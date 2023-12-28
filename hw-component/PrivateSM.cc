

#include "PrivateSM.h"


bool operator<(const curr_instn_id_per_warp_entry& lhs, const curr_instn_id_per_warp_entry& rhs) {
  if (lhs.kid != rhs.kid) {
    return lhs.kid < rhs.kid;
  } else {
    if (lhs.block_id != rhs.block_id) {
      return lhs.block_id < rhs.block_id;
    } else {
      return lhs.warp_id < rhs.warp_id;
    }
  }
}

PrivateSM::PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg){

  m_smid = smid;
  m_cycle = 0;
  active = true;

  m_hw_cfg = hw_cfg;

  this->tracer = tracer;
  issuecfg = this->tracer->get_issuecfg();
  appcfg = this->tracer->get_appcfg();  
  instncfg = this->tracer->get_instncfg();

  // (kernel_id, block_id) pair that are allocated to this SM
  kernel_block_pair = issuecfg->get_kernel_block_by_smid(m_smid);

  // number of warps per kernel that are allocated to this SM
  m_num_warps_per_sm.resize(appcfg->get_kernels_num(), 0);

  m_warp_active_status.reserve(kernel_block_pair.size());
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned block_id = it->second;
    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    m_warp_active_status.push_back(std::vector<bool>(_warps_per_block, false));
  }

  // m_num_warps_per_sm[i] stores the i-th kernel's warps number that are allocated to this SM
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    m_num_warps_per_sm[kid] += _warps_per_block;
  }

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned block_id = it->second;
    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    for (unsigned _i = 0; _i < _warps_per_block; _i++) {
      // std::cout << "D: Initial curr_instn_id_per_warp: " << kid << " " << block_id << " " << _i << " to 0." << std::endl;
      curr_instn_id_per_warp_entry _entry = curr_instn_id_per_warp_entry(kid, block_id, _i);
      curr_instn_id_per_warp[_entry] = 0;
    }
  }

  //traverse m_num_warps_per_sm
  for (auto it = m_num_warps_per_sm.begin(); it != m_num_warps_per_sm.end(); it++){
    std::cout << "m_num_warps_per_sm: " << *it << std::endl;
  }

  // sum of std::vector<unsigned> m_num_warps_per_sm
  all_warps_num = std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.end(), 0);

  // when accessing the ibuffer, the index is:
  //     global_all_kernels_warp_id = gwarp_id + sum_{kid = 0,1,...,kernel_id-1}(m_num_warps_per_sm[kid])
  m_ibuffer = new IBuffer(m_smid, all_warps_num);
  last_fetch_warp_id = 0;
  last_issue_sched_id = 0;
  
  m_scoreboard = new Scoreboard(m_smid, all_warps_num);

  m_inst_fetch_buffer = new inst_fetch_buffer_entry();

  
  // std::cout << "D: all_warps_num: " << all_warps_num << std::endl;

  num_banks = hw_cfg->get_num_reg_banks();
  bank_warp_shift = hw_cfg->get_bank_warp_shift();
  num_scheds = hw_cfg->get_num_sched_per_sm();
  sub_core_model = hw_cfg->get_sub_core_model();
  banks_per_sched = (unsigned)(num_banks / num_scheds);
  inst_fetch_throughput = hw_cfg->get_inst_fetch_throughput();
  reg_file_port_throughput = hw_cfg->get_reg_file_port_throughput();

  warps_per_sched = (unsigned)(all_warps_num / num_scheds);

  last_issue_warp_ids.resize(num_scheds, 0);

  m_reg_bank_allocator = new RegisterBankAllocator(m_smid, 
                                                   num_banks, 
                                                   num_scheds, 
                                                   bank_warp_shift, 
                                                   banks_per_sched);

  parse_blocks_per_kernel();

  total_pipeline_stages =
      N_PIPELINE_STAGES + hw_cfg->get_specialized_unit_size() * 2;
  m_pipeline_reg.reserve(total_pipeline_stages);
  
  std::cout << "total_pipeline_stages: " << total_pipeline_stages << std::endl;

  for (unsigned j = 0; j < N_PIPELINE_STAGES; j++) {
    std::cout << ";;;pipeline_width index " << j << " : " 
              << hw_cfg->get_pipe_widths(static_cast<pipeline_stage_name_t>(j)) << " "
              << hw_cfg->get_pipeline_stage_name_decode(
                 static_cast<pipeline_stage_name_t>(j))
              << std::endl;
    m_pipeline_reg.push_back(
      register_set(
        hw_cfg->get_pipe_widths(static_cast<pipeline_stage_name_t>(j)), 
        std::string(
          hw_cfg->get_pipeline_stage_name_decode(
          static_cast<pipeline_stage_name_t>(j))
        ), 
        hw_cfg
      )
    );
  }

  for (unsigned j = 0; j < hw_cfg->get_specialized_unit_size(); j++) {
    std::cout << ";;;pipeline_width index " 
              << j + N_PIPELINE_STAGES << " : " 
              << hw_cfg->get_pipe_widths_ID_OC_spec_unit(j) << " "
              << std::string("ID_OC_") + hw_cfg->get_m_specialized_unit_name(j)
              << std::endl;
    m_pipeline_reg.push_back(
      register_set(
        hw_cfg->get_pipe_widths_ID_OC_spec_unit(j),
        std::string(
          std::string("ID_OC_") + hw_cfg->get_m_specialized_unit_name(j)
        ), 
        hw_cfg
      )
    );
    // m_config->m_specialized_unit[j].ID_OC_SPEC_ID = m_pipeline_reg.size() - 1;
    m_specilized_dispatch_reg.push_back(
        &m_pipeline_reg[m_pipeline_reg.size() - 1]);
  }

  for (unsigned j = 0; j < hw_cfg->get_specialized_unit_size(); j++) {
    std::cout << ";;;pipeline_width index " 
              << j + N_PIPELINE_STAGES + hw_cfg->get_specialized_unit_size() << " : " 
              << hw_cfg->get_pipe_widths_OC_EX_spec_unit(j) << " "
              << std::string("OC_EX_") + hw_cfg->get_m_specialized_unit_name(j)
              << std::endl;
    m_pipeline_reg.push_back(
      register_set(
        hw_cfg->get_pipe_widths_OC_EX_spec_unit(j),
        std::string(
          std::string("OC_EX_") + hw_cfg->get_m_specialized_unit_name(j)
        ), 
        hw_cfg
      )
    );
    // m_config->m_specialized_unit[j].OC_EX_SPEC_ID = m_pipeline_reg.size() - 1;
  }

  m_sp_out = &m_pipeline_reg[ID_OC_SP];
  m_dp_out = &m_pipeline_reg[ID_OC_DP];
  m_sfu_out = &m_pipeline_reg[ID_OC_SFU];
  m_int_out = &m_pipeline_reg[ID_OC_INT];
  m_tensor_core_out = &m_pipeline_reg[ID_OC_TENSOR_CORE];
  // m_spec_cores_out = m_specilized_dispatch_reg;
  for (unsigned j = 0; j < m_specilized_dispatch_reg.size(); j++) {
    m_spec_cores_out.push_back(m_specilized_dispatch_reg[j]);
  }
  m_mem_out = &m_pipeline_reg[ID_OC_MEM];

  // op collector configuration
  enum { SP_CUS, DP_CUS, SFU_CUS, TENSOR_CORE_CUS, INT_CUS, MEM_CUS, GEN_CUS };
  
  opndcoll_rfu_t::port_vector_t in_ports;
  opndcoll_rfu_t::port_vector_t out_ports;
  opndcoll_rfu_t::uint_vector_t cu_sets;

  m_operand_collector = new opndcoll_rfu_t(m_hw_cfg, m_reg_bank_allocator, this->tracer);


  // configure generic collectors
  m_operand_collector->add_cu_set(
      GEN_CUS, hw_cfg->get_operand_collector_num_units_gen(),
      hw_cfg->get_operand_collector_num_out_ports_gen());

  for (unsigned i = 0; i < hw_cfg->get_operand_collector_num_in_ports_gen();
       i++) {
    in_ports.push_back(&m_pipeline_reg[ID_OC_SP]);
    in_ports.push_back(&m_pipeline_reg[ID_OC_SFU]);
    in_ports.push_back(&m_pipeline_reg[ID_OC_MEM]);
    out_ports.push_back(&m_pipeline_reg[OC_EX_SP]);
    out_ports.push_back(&m_pipeline_reg[OC_EX_SFU]);
    out_ports.push_back(&m_pipeline_reg[OC_EX_MEM]);
    if (1) {
      in_ports.push_back(&m_pipeline_reg[ID_OC_TENSOR_CORE]);
      out_ports.push_back(&m_pipeline_reg[OC_EX_TENSOR_CORE]);
    }
    if (hw_cfg->get_num_dp_units() > 0) {
      in_ports.push_back(&m_pipeline_reg[ID_OC_DP]);
      out_ports.push_back(&m_pipeline_reg[OC_EX_DP]);
    }
    if (hw_cfg->get_num_int_units() > 0) {
      in_ports.push_back(&m_pipeline_reg[ID_OC_INT]);
      out_ports.push_back(&m_pipeline_reg[OC_EX_INT]);
    }
    if (hw_cfg->get_specialized_unit_size() > 0) {
      for (unsigned j = 0; j < hw_cfg->get_specialized_unit_size(); ++j) {
        in_ports.push_back(
            &m_pipeline_reg[N_PIPELINE_STAGES + 0 + j]);
        out_ports.push_back(
            &m_pipeline_reg[N_PIPELINE_STAGES + 3 + j]);
      }
    }
    cu_sets.push_back((unsigned)GEN_CUS);
    m_operand_collector->add_port(in_ports, out_ports, cu_sets);
    in_ports.clear(), out_ports.clear(), cu_sets.clear();
  }

  
  m_operand_collector->init(m_hw_cfg, m_reg_bank_allocator, this->tracer);
  
  // m_fu

  for (unsigned k = 0; k < m_hw_cfg->get_num_sp_units(); k++) {
    m_fu.push_back(new sp_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_SP);
    m_issue_port.push_back(OC_EX_SP);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_sfu_units(); k++) {
    m_fu.push_back(new sfu(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_SFU);
    m_issue_port.push_back(OC_EX_SFU);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_int_units(); k++) {
    m_fu.push_back(new int_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_INT);
    m_issue_port.push_back(OC_EX_INT);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_dp_units(); k++) {
    m_fu.push_back(new dp_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_DP);
    m_issue_port.push_back(OC_EX_DP);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_tensor_core_units(); k++) {
    m_fu.push_back(new tensor_core(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_TENSOR_CORE);
    m_issue_port.push_back(OC_EX_TENSOR_CORE);
  }

  m_fu.push_back(new mem_unit(&m_pipeline_reg[EX_WB], 0, m_hw_cfg, this->tracer));
  m_dispatch_port.push_back(ID_OC_MEM);
  m_issue_port.push_back(OC_EX_MEM);

  for (unsigned k = 0; k < m_hw_cfg->get_specialized_unit_size(); k++) {
    m_fu.push_back(new specialized_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer, k));
    m_dispatch_port.push_back(N_PIPELINE_STAGES + 0 + k);
    m_issue_port.push_back(N_PIPELINE_STAGES + 3 + k);
  }

  num_result_bus = hw_cfg->get_pipe_widths(static_cast<pipeline_stage_name_t>(EX_WB));
  for (unsigned _ = 0; _ < num_result_bus; _++) {
    m_result_bus.push_back(new std::bitset<MAX_ALU_LATENCY>());
  }
}

/* Here, wid is local wid. */
int PrivateSM::register_bank(int regnum, int wid, unsigned sched_id) {
  int bank = regnum;
  //warp的bank偏移。
  if (bank_warp_shift) bank += wid;
  //在subcore模式下，每个warp调度器在寄存器集合中有一个具体的寄存器可供使用，这个寄
  //存器由调度器的m_id索引。m_num_banks_per_sched的定义为：
  //    num_banks / shader->get_config()->gpgpu_num_sched_per_core;
  //在V100配置中，共有4个warp调度器，0号warp调度器可用的bank为0-3，1号warp调度器可
  //用的bank为4-7，2号warp调度器可用的bank为8-11，3号warp调度器可用的bank为12-15。
  if (sub_core_model) {
    unsigned bank_num = (bank % banks_per_sched) + (sched_id * banks_per_sched);
    assert(bank_num < num_banks);
    return bank_num;
  } else
    return bank % num_banks;
}

bool PrivateSM::check_active(){
  return false; // TODO
}

PrivateSM::~PrivateSM() {
  delete m_ibuffer;
  delete m_inst_fetch_buffer;
  delete m_scoreboard;
  delete m_reg_bank_allocator;
  delete m_operand_collector;
  /*
  register_set* m_sp_out;// = &m_pipeline_reg[ID_OC_SP];
  register_set* m_dp_out;// = &m_pipeline_reg[ID_OC_DP];
  register_set* m_sfu_out;// = &m_pipeline_reg[ID_OC_SFU];
  register_set* m_int_out;// = &m_pipeline_reg[ID_OC_INT];
  register_set* m_tensor_core_out;// = &m_pipeline_reg[ID_OC_TENSOR_CORE];
  std::vector<register_set*> m_spec_cores_out;// = m_specilized_dispatch_reg;
  register_set* m_mem_out;// = &m_pipeline_reg[ID_OC_MEM];
  */

  m_sp_out->release_register_set();
  m_dp_out->release_register_set();
  m_sfu_out->release_register_set();
  m_int_out->release_register_set();
  m_tensor_core_out->release_register_set();
  m_mem_out->release_register_set();
  for (auto ptr : m_specilized_dispatch_reg) {
    ptr->release_register_set();
  }
  m_specilized_dispatch_reg.clear();
  m_spec_cores_out.clear();
  
  for (auto ptr : m_pipeline_reg) {
    ptr.release_register_set();
  }

  for (auto ptr : m_fu) {
    delete ptr;
  }

  for (auto ptr : m_result_bus) {
    delete ptr;
  }
}

unsigned PrivateSM::get_num_warps_per_sm(unsigned kernel_id) { 
  return m_num_warps_per_sm[kernel_id]; 
}

void PrivateSM::parse_blocks_per_kernel() {
  for (unsigned i = 0; i < kernel_block_pair.size(); i++) {
    if (blocks_per_kernel.find(kernel_block_pair[i].first) == blocks_per_kernel.end()) {
      blocks_per_kernel[kernel_block_pair[i].first] = 
        std::vector<unsigned>(1, kernel_block_pair[i].second);
    } else {
      blocks_per_kernel[kernel_block_pair[i].first].push_back(kernel_block_pair[i].second);
    }
  }
}

std::vector<unsigned> PrivateSM::get_blocks_per_kernel(unsigned kernel_id) {
  return blocks_per_kernel[kernel_id];
}

std::map<unsigned, std::vector<unsigned>>* PrivateSM::get_blocks_per_kernel() {
  return &blocks_per_kernel;
}

unsigned PrivateSM::get_inst_fetch_throughput() { return inst_fetch_throughput; }
unsigned PrivateSM::get_reg_file_port_throughput() { return reg_file_port_throughput; }

void PrivateSM::issue_warp(register_set &pipe_reg_set, 
                           ibuffer_entry entry, 
                           unsigned sch_id) {
  // print entry
  std::cout << "  issue_warp: " << std::endl;
  std::cout << "    pc: " << entry.pc << ", wid: " << entry.wid 
            << ", kid: " << entry.kid << ", uid: " << entry.uid << std::endl;

  // inst_fetch_buffer_entry **pipe_reg =
  //   pipe_reg_set.get_free(m_hw_cfg->get_sub_core_model(), sch_id);
  // assert(pipe_reg);

  inst_fetch_buffer_entry* tmp = new inst_fetch_buffer_entry();
  tmp->kid = entry.kid;
  tmp->pc = entry.pc;
  tmp->uid = entry.uid;
  tmp->wid = entry.wid;
  tmp->m_valid = true;

  std::cout << "  issue_warp: " << std::endl;
  std::cout << "    pc: " << tmp->pc << ", wid: " << tmp->wid 
            << ", kid: " << tmp->kid << ", uid: " << tmp->uid << std::endl;

  pipe_reg_set.move_in(m_hw_cfg->get_sub_core_model(), sch_id, tmp);

  delete tmp;
  tmp = nullptr;  // Optional: set tmp to nullptr to avoid dangling pointer

  // print reigster set
  std::cout << "  Now register set: " << std::endl;
  pipe_reg_set.print();

  // Scoreboard: TODO

}

void PrivateSM::run(){
  m_cycle++;

  std::cout << "# cycle: " << m_cycle << std::endl;

  if (m_cycle >= 100) {
    active = false;
  }

  // for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++){
  
  // TODO: here only first kid-bid pair is considered
  auto it_kernel_block_pair = kernel_block_pair.begin(); 

    unsigned kid = it_kernel_block_pair->first - 1;
    
    unsigned block_id = it_kernel_block_pair->second;
    unsigned warps_per_block = appcfg->get_num_warp_per_block(kid);
    /* Calculate gwarp_id:
     *     gwarp_id_start = block_id * warps_per_block
     *     gwarp_id_end   = (block_id + 1) * warps_per_block */
    // std::cout << "D: warps_per_block, block_id: " << warps_per_block << " " << block_id << std::endl;
    unsigned gwarp_id_start = warps_per_block * block_id;
    unsigned gwarp_id_end = gwarp_id_start + warps_per_block - 1;

    // std::cout << "$ SM-" << m_smid << " Kernel-" << kid << " Block-" << block_id 
    //           << ", gwarp_id_start: " << gwarp_id_start << ", gwarp_id_end: " 
    //           << gwarp_id_end << std::endl;

    for (unsigned _wid_ = 0; _wid_ < warps_per_block; _wid_++) {
      if (m_cycle == 1 && m_warp_active_status[kid][_wid_] == false) {
        m_warp_active_status[kid][_wid_] = true;
        std::cout << "  **First time to activate warp (kid, wid): (" << kid << ", " << _wid_ << ")" << std::endl;
      }
    }


    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                              Write back to register banks.                             ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
/***************************************************************************************************
    std::vector<std::vector<stage_instns_identifier>::iterator> writeback_stage_instns_to_remove;
    // traverse writeback_stage_instns
    for (auto it = writeback_stage_instns.begin(); it != writeback_stage_instns.end(); it++) {
      unsigned _kid = it->kid;
      unsigned _pc = it->pc;
      unsigned _wid = it->wid;
      unsigned _uid = it->uid;

      // get the dst reg id of the instn (_kid, _pc, _wid, _uid)
      auto _trace_warp_inst = tracer->get_one_kernel_one_warp_one_instn(_kid, _wid, _uid)->trace_warp_inst;
      unsigned dst_reg_num = _trace_warp_inst.get_outcount();
      for (unsigned i = 0; i < dst_reg_num; i++){
        int dst_reg_id = _trace_warp_inst.get_arch_reg_dst(i);
        // std::cout << "    dst_reg_id[" << i << "]: " << dst_reg_id << std::endl;
        // Calculate the scheduler id of the dst reg id
        if (dst_reg_id >= 0) {
          auto local_wid = (unsigned)(_wid % warps_per_block);
          auto sched_id = (unsigned)(local_wid % num_scheds);
          // Calculate the bank id of the dst reg id
          auto bank_id = register_bank(dst_reg_id, local_wid, sched_id);
          std::cout << "    kid, wid, uid, dst_reg_num, dst_reg_id, bank_id, sched_id: " << _kid << ", " 
                                                                    << _wid << " (local:" 
                                                                    << local_wid << "), "
                                                                    << _uid << ", " 
                                                                    << dst_reg_num << ", " 
                                                                    << dst_reg_id << ", " 
                                                                    << bank_id << ", " 
                                                                    << sched_id << std::endl;
          // Now we set the bank_id of the dst reg id to be on_write (for dst reg id, it is on_wite).
          if (m_reg_bank_allocator->getBankState(bank_id) == FREE) {
            m_reg_bank_allocator->setBankState(bank_id, ON_WRITING);
            
            _trace_warp_inst.set_arch_reg_dst(i, -1);
            std::cout << "    setBankState(" << bank_id 
                      << ", WRITING)   dst_reg_id : " << _trace_warp_inst.get_arch_reg_dst(i) << std::endl;
          }
        }
      }

      bool all_write_back = true;
      for (unsigned i = 0; i < dst_reg_num; i++){
        if (_trace_warp_inst.get_arch_reg_dst(i) != -1) {
          all_write_back = false;
          break;
        }
      }

      // if all dst reg ids are written back, then we can release the bank states and
      // remove the instn from writeback_stage_instns
      if (all_write_back) {
        // std::cout << "    all_write_back" << std::endl;
        std::cout << "  Write back instn (pc, gwid, kid, fetch_instn_id): (" << _pc << ", " 
                                                                             << _wid << ", " 
                                                                             << _kid << ", " 
                                                                             << _uid << ")" << std::endl;
        
        // This should be done in the read operand stage.
        // for (unsigned i = 0; i < dst_reg_num; i++){
        //   int dst_reg_id = _trace_warp_inst.get_out(i);
        //   if (dst_reg_id >= 0) {
        //     auto local_wid = (unsigned)(_wid % warps_per_block);
        //     auto sched_id = (unsigned)(local_wid % num_scheds);
        //     // Calculate the bank id of the dst reg id
        //     auto bank_id = register_bank(dst_reg_id, local_wid, sched_id);
        //     std::cout << "    releaseBankState(" << bank_id << ")" << std::endl;
        //     m_reg_bank_allocator->releaseBankState(bank_id);
        //   }
        // }

        // remove the instn from writeback_stage_instns
        // writeback_stage_instns.erase(it);
        writeback_stage_instns_to_remove.push_back(it);
      }

    }

    // remove the instns from writeback_stage_instns
    for (auto it = writeback_stage_instns_to_remove.begin(); 
              it != writeback_stage_instns_to_remove.end(); it++){
      writeback_stage_instns.erase(*it);
    }
***************************************************************************************************/
    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                       Transfer instns to writeback_stage_instns.                       ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
/***************************************************************************************************
    // std::cout << "D: kid: " << kid << " m_valid: " << m_inst_fetch_buffer->m_valid << std::endl;
    for (auto gwid = gwarp_id_start; gwid <= gwarp_id_end; gwid++) {
      auto global_all_kernels_warp_id = gwid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
      // check if the ibuffer has free slot
      // std::cout << "D: global_all_kernels_warp_id: " << global_all_kernels_warp_id << std::endl;
      // std::cout << "D: m_ibuffer->is_not_empty(global_all_kernels_warp_id): " << m_ibuffer->is_not_empty(global_all_kernels_warp_id) << std::endl;
      if (m_ibuffer->is_not_empty(global_all_kernels_warp_id)) {
        // pop the instn from ibuffer
        // TODO
        // ibuffer_entry entry = m_ibuffer->pop_front(global_all_kernels_warp_id);
        ibuffer_entry entry = m_ibuffer->front(global_all_kernels_warp_id);
        unsigned _fetch_instn_id = entry.uid;
        unsigned _pc = entry.pc;
        unsigned _gwid = entry.wid;
        unsigned _kid = entry.kid;
        // TODO
        // writeback_stage_instns.push_back(stage_instns_identifier(_kid, _pc, _gwid, _fetch_instn_id));
        std::cout << "  Transfer instn (pc, gwid, kid, fetch_instn_id): (" << _pc << ", " 
                                                                           << _gwid << ", " 
                                                                           << _kid << ", " 
                                                                           << _fetch_instn_id << ")" << std::endl;
        m_ibuffer->print_ibuffer(gwid);
      }
    }

***************************************************************************************************/
    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                              Write back to register banks.                             ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    inst_fetch_buffer_entry **preg = m_pipeline_reg[EX_WB].get_ready();
    inst_fetch_buffer_entry *pipe_reg = (preg == NULL) ? NULL : *preg;
    while (preg and pipe_reg->m_valid) {
      /*
      * Right now, the writeback stage drains all waiting instructions
      * assuming there are enough ports in the register file or the
      * conflicts are resolved at issue.
      */
      /*
      * The operand collector writeback can generally generate a stall
      * However, here, the pipelines should be un-stallable. This is
      * guaranteed because this is the first time the writeback function
      * is called after the operand collector's step function, which
      * resets the allocations. There is one case which could result in
      * the writeback function returning false (stall), which is when
      * an instruction tries to modify two registers (GPR and predicate)
      * To handle this case, we ignore the return value (thus allowing
      * no stalling).
      */

      // m_operand_collector.writeback(*pipe_reg);
      unsigned _kid = pipe_reg->kid;
      unsigned _pc = pipe_reg->pc;
      unsigned _wid = pipe_reg->wid;
      unsigned _uid = pipe_reg->uid;

      auto _compute_instn = tracer->get_one_kernel_one_warp_one_instn(_kid, _wid, _uid);
      auto _trace_warp_inst = 
        _compute_instn->trace_warp_inst;
      unsigned dst_reg_num = _trace_warp_inst.get_outcount();

      std::vector<int> need_write_back_regs_num;

      for (unsigned i = 0; i < dst_reg_num; i++){
        int dst_reg_id = _trace_warp_inst.get_arch_reg_dst(i);
        // std::cout << "    dst_reg_id[" << i << "]: " << dst_reg_id << std::endl;
        // Calculate the scheduler id of the dst reg id
        if (dst_reg_id >= 0) {
          auto local_wid = (unsigned)(_wid % warps_per_block);
          auto sched_id = (unsigned)(local_wid % num_scheds);
          // Calculate the bank id of the dst reg id
          auto bank_id = register_bank(dst_reg_id, local_wid, sched_id);
          std::cout << "    kid, wid, uid, dst_reg_num, "
                       "dst_reg_id, bank_id, sched_id: " 
                    << _kid << ", " 
                    << _wid << " (local:" 
                    << local_wid << "), "
                    << _uid << ", " 
                    << dst_reg_num << ", " 
                    << dst_reg_id << ", " 
                    << bank_id << ", " 
                    << sched_id << std::endl;
          // Now we set the bank_id of the dst reg id to be on_write (for dst reg id, it is on_wite).
          if (m_reg_bank_allocator->getBankState(bank_id) == FREE) {
            m_reg_bank_allocator->setBankState(bank_id, ON_WRITING);
            
            need_write_back_regs_num.push_back(dst_reg_id);
            
            _trace_warp_inst.set_arch_reg_dst(i, -1);
            std::cout << "    setBankState(" << bank_id 
                      << ", WRITING)   dst_reg_id : " << _trace_warp_inst.get_arch_reg_dst(i) << std::endl;
          }
        }
      }

      bool all_write_back = true;
      for (unsigned i = 0; i < dst_reg_num; i++){
        if (_trace_warp_inst.get_arch_reg_dst(i) != -1) {
          all_write_back = false;
          break;
        }
      }

      // if all dst reg ids are written back, then we can release the bank states and
      // remove the instn from writeback_stage_instns
      if (all_write_back) {
        // std::cout << "    all_write_back" << std::endl;
        std::cout << "  **Write back instn (pc, gwid, kid, fetch_instn_id): (" << std::hex 
                                                                               << _pc << ", " << std::dec 
                                                                               << _wid << ", " 
                                                                               << _kid << ", " 
                                                                               << _uid << ")" << std::endl;
        
        

        // This should be done in the read operand stage.
        // for (unsigned i = 0; i < dst_reg_num; i++){
        //   int dst_reg_id = _trace_warp_inst.get_out(i);
        //   if (dst_reg_id >= 0) {
        //     auto local_wid = (unsigned)(_wid % warps_per_block);
        //     auto sched_id = (unsigned)(local_wid % num_scheds);
        //     // Calculate the bank id of the dst reg id
        //     auto bank_id = register_bank(dst_reg_id, local_wid, sched_id);
        //     std::cout << "    releaseBankState(" << bank_id << ")" << std::endl;
        //     m_reg_bank_allocator->releaseBankState(bank_id);
        //   }
        // }

        // remove the instn from writeback_stage_instns
        pipe_reg->m_valid = false;

        /* If the instn string of (_kid, _wid, _pc) is "EXIT", should deactivate this warp. */
        if (_trace_warp_inst.get_opcode() == OP_EXIT) {
          m_warp_active_status[_kid][_wid - gwarp_id_start] = false;
        }

      } else {
        std::cout << "  **Try to but fail to Write back "
                     "instn (pc, gwid, kid, fetch_instn_id): (" << std::hex 
                  << _pc << ", " << std::dec 
                  << _wid << ", " 
                  << _kid << ", " 
                  << _uid << ")" << std::endl;
      }

      if (all_write_back) {
        m_scoreboard->printContents();

        _inst_trace_t* tmp_inst_trace = _compute_instn->inst_trace;
        for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
          need_write_back_regs_num.push_back(tmp_inst_trace->reg_src[i]);
        }
        for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
          need_write_back_regs_num.push_back(tmp_inst_trace->reg_dest[i]);
        }
        auto pred = _trace_warp_inst.get_pred();
        need_write_back_regs_num.push_back((pred < 0) ? pred : pred + PRED_NUM_OFFSET);

        for (auto regnum : need_write_back_regs_num) {
          // void Scoreboard::releaseRegisters(const unsigned wid, std::vector<int> regnums)
          m_scoreboard->releaseRegisters(_wid, regnum);
        }

        m_scoreboard->printContents();
      }
      
      preg = m_pipeline_reg[EX_WB].get_ready();
      pipe_reg = (preg == NULL) ? NULL : *preg;
    }
    


    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                                         Execute.                                       ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    
    for (unsigned i = 0; i < num_result_bus; i++) {
      *(m_result_bus[i]) >>= 1;
    }

    for (unsigned n = 0; n < m_fu.size(); n++) {
      for (auto _ = 0; _ < m_fu[n]->clock_multiplier(); _++) m_fu[n]->cycle();
    }

    std::vector<opndcoll_rfu_t::input_port_t>* m_in_ports_ptr = m_operand_collector->get_m_in_ports();
    /* m_in_ports:
     *   m_in_ports[0]: GEN_CUS, &m_pipeline_reg[ID_OC_SP], &m_pipeline_reg[ID_OC_DP], ... => m_in
     *                           &m_pipeline_reg[OC_EX_SP], &m_pipeline_reg[OC_EX_DP], ... => m_out
     *   m_in_ports[1~7]: ... */
    for (unsigned p = 0; p < m_in_ports_ptr->size(); p++) {
      /* inp => m_in_ports[p]:
       *   GEN_CUS, &m_pipeline_reg[ID_OC_SP], &m_pipeline_reg[ID_OC_DP], ... => m_in
       *            &m_pipeline_reg[OC_EX_SP], &m_pipeline_reg[OC_EX_DP], ... => m_out 
       * We have put the instns that have completed the read operands process into 
       * m_pipeline_reg[OC_EX_SP] or m_pipeline_reg[OC_EX_DP] or ..., so here we will 
       * traverse all the inp.m_out[.] to find ready instns and send them to the execute
       * process. */
      opndcoll_rfu_t::input_port_t &inp = (*m_in_ports_ptr)[p];
      std::cout << "  **Execute: " << "port_idx: " << p << std::endl;
      /* inp.m_out: 
       *   &m_pipeline_reg[OC_EX_SP], 
       *   &m_pipeline_reg[OC_EX_DP], ... */
      for (unsigned i = 0; i < inp.m_out.size(); i++) {
        if ((*inp.m_out[i]).has_ready()) {
          // print the ready instn
          std::cout << "  Ready instn in inp.m_out[" << i << "]: " << std::endl;
          // (*inp.m_out[i]).print();
          std::vector<unsigned> ready_reg_ids = (*inp.m_out[i]).get_ready_reg_ids();
          for (unsigned j = 0; j < ready_reg_ids.size(); j++) {
            unsigned reg_id = ready_reg_ids[j];
            unsigned _kid = (*inp.m_out[i]).get_kid(reg_id);
            unsigned _pc = (*inp.m_out[i]).get_pc(reg_id);
            unsigned _wid = (*inp.m_out[i]).get_wid(reg_id);
            unsigned _uid = (*inp.m_out[i]).get_uid(reg_id);
            std::cout << "    Ready instn "
                         "(pc, gwid, kid, fetch_instn_id): (" << _pc << ", " 
                                                              << _wid << ", " 
                                                              << _kid << ", " 
                                                              << _uid << ")" 
                                                              << std::endl;
            // remove the instn from (*inp.m_out[i]) and add it to execute stages
            compute_instn* tmp = 
              tracer->get_one_kernel_one_warp_one_instn(_kid, _wid, _uid);
            _inst_trace_t* tmp_inst_trace = tmp->inst_trace;

            std::cout << "tmp_inst_trace->get_func_unit(): " 
                      << tmp_inst_trace->get_func_unit() << std::endl;
            
            unsigned offset_fu = 0;
            bool schedule_wb_now = false;
            int resbus = -1;

            switch (tmp_inst_trace->get_func_unit()) {
              // In PrivateSM.cc, m_fu[SP_UNIT-1] => SP_UNIT_PIPELINE.
              case SP_UNIT:
                std::cout << "    SP unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    SP unit initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = 0;
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_sp_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "num_result_bus: " << num_result_bus << std::endl;
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl
                              << "resbus: " << resbus << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_sp_units() << std::endl;
                  }
                }
                break;
              case DP_UNIT:
                std::cout << "    DP unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    DP unit initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_dp_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_dp_units() << std::endl;
                  }
                }
                break;
              case SFU_UNIT:
                std::cout << "    SFU unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    SFU unit initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_sfu_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_sfu_units() << std::endl;
                  }
                }
                break;
              case TENSOR_CORE_UNIT:
                std::cout << "    TENSOR_CORE unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    TENSOR_CORE unit initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_tensor_core_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_tensor_core_units() << std::endl;
                  }
                }
                break;
              case INT_UNIT:
                std::cout << "    INT unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    INT unit initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_int_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_int_units() << std::endl;
                  }
                }
                break;
              case LDST_UNIT:
                std::cout << "    LDST unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    LDST unit initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                // TODO: get latency from memory_model from reuse distance.
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units();
                for (unsigned _ = 0; _ < 1; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  }
                }

                break;
              case SPEC_UNIT_1:
                std::cout << "    SPEC_UNIT_1 unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    SPEC_UNIT_1 initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units()
                            + 1;
                for (unsigned _ = 0; _ < 1; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  }
                }
                break;
              case SPEC_UNIT_2:
                std::cout << "    SPEC_UNIT_2 unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    SPEC_UNIT_2 initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units()
                            + 1;
                for (unsigned _ = 1; _ < 2; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  }
                }
                break;
              case SPEC_UNIT_3:
                std::cout << "    SPEC_UNIT_3 unit latency: " 
                          << tmp_inst_trace->get_latency() 
                          << std::endl;
                std::cout << "    SPEC_UNIT_3 initiation interval: " 
                         << tmp_inst_trace->get_initiation_interval() 
                         << std::endl;
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units()
                            + 1;
                for (unsigned _ = 2; _ < 3; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]));
                      std::cout << "    executing instn: " 
                                << "pc-" << _pc 
                                << ", wid-" << _wid 
                                << ", kid-" << _kid 
                                << ", uid-" << _uid << std::endl;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  }
                }
                break;
              default:
                std::cout << "Error: tmp_inst_trace->get_func_unit(): " 
                          << tmp_inst_trace->get_func_unit() << std::endl;
                assert(false);
            }
          }
        }
      }
    }
    

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                                      Read Operands.                                    ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    for (unsigned _iter = 0; _iter < get_reg_file_port_throughput(); _iter++) {
      std::cout << "  **Read Operands: " << "reg_file_port_idx: " << _iter << std::endl;
      m_operand_collector->step();
    }


    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                               Issue intns to issue_port.                               ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    

    unsigned max_issue_per_warp = m_hw_cfg->get_max_insn_issue_per_warp();
    std::cout << "  **ISSUE stage: " << std::endl;
    for (unsigned _sched_id = 0; _sched_id < num_scheds; _sched_id++) {
      auto sched_id = (last_issue_sched_id + _sched_id) % num_scheds;
      std::cout << "    D: issue sched_id: " << sched_id << std::endl;
      
      
      // LRR warp sheduling
      for (auto gwid = gwarp_id_start; 
           (gwid <= gwarp_id_end) && 
           (gwid % num_scheds == sched_id); 
           gwid++) {
        // std::cout << last_issue_warp_ids.size() << std::endl;
        // for (auto it = last_issue_warp_ids.begin(); it != last_issue_warp_ids.end(); it++) {
        //   std::cout << *it << std::endl;
        // }
        auto wid = (last_issue_warp_ids[sched_id] + gwid) % warps_per_block + gwarp_id_start;
        auto global_all_kernels_warp_id = wid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
        // std::cout << "D: global_all_kernels_warp_id: " << global_all_kernels_warp_id << std::endl;
        // std::cout << "D: m_ibuffer->is_not_empty(global_all_kernels_warp_id): " << m_ibuffer->is_not_empty(global_all_kernels_warp_id) << std::endl;
        
        unsigned issued_num = 0;
        unsigned checked_num = 0;

        exec_unit_type_t previous_issued_inst_exec_type = exec_unit_type_t::NONE;

        while (
          (issued_num < max_issue_per_warp) &&
          (checked_num <= issued_num)
        ) {
          bool warp_inst_issued = false;

          std::vector<int> regnums;
          int pred;
          int ar1;
          int ar2;

          if (m_ibuffer->is_not_empty(global_all_kernels_warp_id)) {
            
            std::cout << "  Before pop,m_ibuffer:" << std::endl;
            m_ibuffer->print_ibuffer(wid);
            // pop the instn from ibuffer
            ibuffer_entry entry = m_ibuffer->front(global_all_kernels_warp_id);
            std::cout << "  pop_front(uid,pc,wid,kid): " 
                                        << entry.uid << " " 
                                        << entry.pc << " " 
                                        << entry.wid << " " 
                                        << entry.kid << std::endl;
            std::cout << "  After pop,m_ibuffer:" << std::endl;
            m_ibuffer->print_ibuffer(entry.wid);
            unsigned _fetch_instn_id = entry.uid;
            unsigned _pc = entry.pc;
            unsigned _gwid = entry.wid;
            unsigned _kid = entry.kid;
            std::cout << "  try to issue: " << _fetch_instn_id << " " 
                                            << _pc << " " 
                                            << _gwid << " " 
                                            << _kid << std::endl;
            
            
            
            // get instn by entry
            compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
            _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
            trace_warp_inst_t* tmp_trace_warp_inst = &(tmp->trace_warp_inst);
            std::cout << "    instn: " << tmp_inst_trace->instn_str << std::endl;
            
            std::cout << "    opcode: " << tmp_trace_warp_inst->get_opcode() << " " 
                      << tmp_trace_warp_inst->get_op() << std::endl;
            std::cout << "            " << OP_SHFL << " " << ALU_OP << std::endl;
            auto latency = tmp_inst_trace->get_latency();
            std::cout << "    latency: " << latency << std::endl;
            auto init_latency = tmp_inst_trace->get_initiation_interval();
            std::cout << "    init_latency: " << init_latency << std::endl;


            std::cout << "  @ pred: " << tmp_trace_warp_inst->get_pred() << " " << std::endl;
            std::cout << "  @ ar1: " << tmp_trace_warp_inst->get_ar1() << " " << std::endl;
            std::cout << "  @ ar2: " << tmp_trace_warp_inst->get_ar2() << " " << std::endl;
            std::cout << "  @ srcs: ";
            for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
              std::cout << tmp_inst_trace->reg_src[i] << " ";
            }
            std::cout << std::endl;
            std::cout << "  @ dsts: ";
            for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
              std::cout << tmp_inst_trace->reg_dest[i] << " ";
            }
            std::cout << std::endl;

            
            pred = tmp_trace_warp_inst->get_pred();
            ar1 = tmp_trace_warp_inst->get_ar1();
            ar2 = tmp_trace_warp_inst->get_ar2();
            
            
            for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
              regnums.push_back(tmp_inst_trace->reg_src[i]);
            }
            for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
              regnums.push_back(tmp_inst_trace->reg_dest[i]);
            }

            bool check_is_scoreboard_collision = false;
            check_is_scoreboard_collision = 
              m_scoreboard->checkCollision(global_all_kernels_warp_id, 
                                           regnums, 
                                           /* set PRED+NUM_OFFSET to avoid collision with regnums */
                                           (pred < 0) ? pred : pred + PRED_NUM_OFFSET, 
                                           ar1,
                                           ar2);
            std::cout << "  check_is_scoreboard_collision: " 
                      << check_is_scoreboard_collision << std::endl;

            if (tmp_trace_warp_inst->get_opcode() == OP_EXIT && 
                m_scoreboard->regs_size(global_all_kernels_warp_id) > 0) {
              check_is_scoreboard_collision = true;
            }
            
            if (check_is_scoreboard_collision) {
              checked_num++;
              continue;
            }
            
            // get the function unit of the instn
            auto fu = tmp_inst_trace->get_func_unit();
            std::cout << "  Execute on FU: ";
            
            /* Identify the availability of function units. */
            bool sp_pipe_avail = false;
            bool sfu_pipe_avail = false;
            bool int_pipe_avail = false;
            bool dp_pipe_avail = false;
            bool tensor_core_pipe_avail = false;
            bool ldst_pipe_avail = false;
            bool spec_1_pipe_avail = false;
            bool spec_2_pipe_avail = false;
            bool spec_3_pipe_avail = false;
            /*
            enum exec_unit_type_t {
              NONE = 0,
              SP = 1,
              SFU = 2,
              LDST = 3,
              DP = 4,
              INT = 5,
              TENSOR = 6,
              SPECIALIZED = 7
            };
            */
            switch (fu) {
              case NON_UNIT:
                std::cout << "NON_UNIT" << std::endl;
                assert(0);
                break;
              case SP_UNIT:
                std::cout << "SP_UNIT" << std::endl;
                sp_pipe_avail = 
                  (m_hw_cfg->get_num_sp_units() > 0) &&
                  m_sp_out->has_free(m_hw_cfg->get_sub_core_model(), 
                                    sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::SP);
                std::cout << "sp_pipe_avail: " << sp_pipe_avail << std::endl;
                if (sp_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_sp_out, entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::SP;
                  
                  m_sp_out->print();
                }
                break;
              case SFU_UNIT:
                std::cout << "SFU_UNIT" << std::endl;
                sfu_pipe_avail = 
                  (m_hw_cfg->get_num_sfu_units() > 0) &&
                  m_sfu_out->has_free(m_hw_cfg->get_sub_core_model(), 
                                      sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::SFU);
                std::cout << "sfu_pipe_avail: " << sfu_pipe_avail << std::endl;
                if (sfu_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_sfu_out, entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::SFU;
                  
                  m_sfu_out->print();
                }
                break;
              case INT_UNIT:
                std::cout << "INT_UNIT" << std::endl;
                int_pipe_avail = 
                  (m_hw_cfg->get_num_int_units() > 0) &&
                  m_int_out->has_free(m_hw_cfg->get_sub_core_model(), 
                                      sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() || 
                  previous_issued_inst_exec_type != exec_unit_type_t::INT);
                std::cout << "  int_pipe_avail: " << int_pipe_avail << std::endl;
                if (int_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_int_out, entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::INT;
                  
                  m_int_out->print();
                }
                break;
              case DP_UNIT:
                std::cout << "DP_UNIT" << std::endl;
                dp_pipe_avail = 
                  (m_hw_cfg->get_num_dp_units() > 0) &&
                  m_dp_out->has_free(m_hw_cfg->get_sub_core_model(), 
                                    sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::DP);
                std::cout << "  dp_pipe_avail: " << dp_pipe_avail << std::endl;
                if (dp_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_dp_out, entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::DP;
                  
                  m_dp_out->print();
                }
                break;
              case TENSOR_CORE_UNIT:
                std::cout << "TENSOR_CORE_UNIT" << std::endl;
                tensor_core_pipe_avail = 
                  (m_hw_cfg->get_num_tensor_core_units() > 0) &&
                  m_tensor_core_out->has_free(m_hw_cfg->get_sub_core_model(), 
                                              sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::TENSOR);
                std::cout << "  tensor_core_pipe_avail: " << tensor_core_pipe_avail << std::endl;
                if (tensor_core_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_tensor_core_out, entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::TENSOR;
                  
                  m_tensor_core_out->print();
                }
                break;
              case LDST_UNIT:
                std::cout << "LDST_UNIT" << std::endl;
                ldst_pipe_avail = 
                  m_mem_out->has_free(m_hw_cfg->get_sub_core_model(), 
                                      sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() || 
                  previous_issued_inst_exec_type != exec_unit_type_t::LDST);
                std::cout << "  ldst_pipe_avail: " << ldst_pipe_avail << std::endl;
                if (ldst_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_mem_out, entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::LDST;
                  
                  m_mem_out->print();
                }
                break;
              case SPEC_UNIT_1:
                std::cout << "SPEC_UNIT_1" << std::endl;
                spec_1_pipe_avail = 
                  (m_hw_cfg->get_specialized_unit_1_enabled()) &&
                  m_spec_cores_out[0]->has_free(m_hw_cfg->get_sub_core_model(), 
                                                sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::SPECIALIZED);
                std::cout << "  spec_1_pipe_avail: " << spec_1_pipe_avail << std::endl;
                if (spec_1_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_spec_cores_out[0], entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::SPECIALIZED;
                  
                  m_spec_cores_out[0]->print();
                }
                break;
              case SPEC_UNIT_2:
                std::cout << "SPEC_UNIT_2" << std::endl;
                spec_2_pipe_avail = 
                  (m_hw_cfg->get_specialized_unit_2_enabled()) &&
                  m_spec_cores_out[1]->has_free(m_hw_cfg->get_sub_core_model(), 
                                                sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::SPECIALIZED);
                std::cout << "  spec_2_pipe_avail: " << spec_2_pipe_avail << std::endl;
                if (spec_2_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_spec_cores_out[1], entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::SPECIALIZED;
                  
                  m_spec_cores_out[1]->print();
                }
                break;
              case SPEC_UNIT_3:
                std::cout << "SPEC_UNIT_3" << std::endl;
                spec_3_pipe_avail = 
                  (m_hw_cfg->get_specialized_unit_3_enabled()) &&
                  m_spec_cores_out[2]->has_free(m_hw_cfg->get_sub_core_model(), 
                                                sched_id) &&
                  (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                  previous_issued_inst_exec_type != exec_unit_type_t::SPECIALIZED);
                std::cout << "  spec_3_pipe_avail: " << spec_3_pipe_avail << std::endl;
                if (spec_3_pipe_avail) {
                  // issue
                  warp_inst_issued = true;
                  issued_num++;
                  issue_warp(*m_spec_cores_out[2], entry, sched_id);
                  previous_issued_inst_exec_type = exec_unit_type_t::SPECIALIZED;
                  
                  m_spec_cores_out[2]->print();
                }
                break;
              default:
                std::cout << "default UNIT" << std::endl;
                assert(0);
            }



            // traverse m_pipeline_reg
            if (false) {
              std::cout << "==================================================" << std::endl;
              for (auto it = m_pipeline_reg.begin(); it != m_pipeline_reg.end(); it++) {
                std::cout << "  ||name: " << it->get_name() << std::endl;
                std::cout << "    has_ready: " << it->has_ready() << std::endl;
                std::cout << "    has_free: " << it->has_free() << std::endl;
              }


              std::cout << "  ||name: " << m_sp_out->get_name() << std::endl;
              std::cout << "    has_ready: " << m_sp_out->has_ready() << std::endl;
              std::cout << "    has_free: " << m_sp_out->has_free() << std::endl;
              // m_sp_out->print();
              std::cout << "  ||name: " << m_dp_out->get_name() << std::endl;
              std::cout << "    has_ready: " << m_dp_out->has_ready() << std::endl;
              std::cout << "    has_free: " << m_dp_out->has_free() << std::endl;
              // m_dp_out->print();
              std::cout << "  ||name: " << m_sfu_out->get_name() << std::endl;
              std::cout << "    has_ready: " << m_sfu_out->has_ready() << std::endl;
              std::cout << "    has_free: " << m_sfu_out->has_free() << std::endl;
              // m_sfu_out->print();
              std::cout << "  ||name: " << m_int_out->get_name() << std::endl;
              std::cout << "    has_ready: " << m_int_out->has_ready() << std::endl;
              std::cout << "    has_free: " << m_int_out->has_free() << std::endl;
              // m_int_out->print();
              std::cout << "  ||name: " << m_tensor_core_out->get_name() << std::endl;
              std::cout << "    has_ready: " << m_tensor_core_out->has_ready() << std::endl;
              std::cout << "    has_free: " << m_tensor_core_out->has_free() << std::endl;
              // m_tensor_core_out->print();
              std::cout << "  ||name: " << m_mem_out->get_name() << std::endl;
              std::cout << "    has_ready: " << m_mem_out->has_ready() << std::endl;
              std::cout << "    has_free: " << m_mem_out->has_free() << std::endl;
              // m_mem_out->print();
              std::cout << "  ||name: " << m_spec_cores_out[0]->get_name() << std::endl;
              std::cout << "    has_ready: " << m_spec_cores_out[0]->has_ready() << std::endl;
              std::cout << "    has_free: " << m_spec_cores_out[0]->has_free() << std::endl;
              // m_spec_cores_out[0]->print();
              std::cout << "  ||name: " << m_spec_cores_out[1]->get_name() << std::endl;
              std::cout << "    has_ready: " << m_spec_cores_out[1]->has_ready() << std::endl;
              std::cout << "    has_free: " << m_spec_cores_out[1]->has_free() << std::endl;
              // m_spec_cores_out[1]->print();
              std::cout << "  ||name: " << m_spec_cores_out[2]->get_name() << std::endl;
              std::cout << "    has_ready: " << m_spec_cores_out[2]->has_ready() << std::endl;
              std::cout << "    has_free: " << m_spec_cores_out[2]->has_free() << std::endl;
              // m_spec_cores_out[2]->print();
              std::cout << "==================================================" << std::endl;
            }
            
          }

          if (warp_inst_issued) {
            m_ibuffer->pop_front(global_all_kernels_warp_id);
            // check_is_scoreboard_collision = 
            //   m_scoreboard->checkCollision(global_all_kernels_warp_id, 
            //                                regnums, 
            //                                /* set PRED+NUM_OFFSET to avoid collision with regnums */
            //                                pred + PRED_NUM_OFFSET, 
            //                                ar1,
            //                                ar2);

            // reserveRegisters(const unsigned wid, std::vector<int> regnums, bool is_load)
            
            regnums.push_back((pred < 0) ? pred : pred + PRED_NUM_OFFSET); 
            // BUG: need to change pred < 0 to "P?"
            regnums.push_back(ar1);
            regnums.push_back(ar2);
            m_scoreboard->reserveRegisters(global_all_kernels_warp_id, regnums, false);
            m_scoreboard->printContents();
          }

          checked_num++;
        }
      }


    }

    last_issue_sched_id = (last_issue_sched_id + 1) % num_scheds;

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                           Fetch and Decode instns to Fbuffer.                          ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    for (unsigned _ = 0; _ < get_inst_fetch_throughput(); _++) {
      // DECODE
      
      if (m_inst_fetch_buffer->m_valid) {
        auto _pc = m_inst_fetch_buffer->pc;
        auto _wid = m_inst_fetch_buffer->wid;
        auto _kid = m_inst_fetch_buffer->kid;
        auto _uid = m_inst_fetch_buffer->uid;

        auto global_all_kernels_warp_id = _wid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + _kid, 0);
        
        if (m_ibuffer->has_free_slot(global_all_kernels_warp_id)) {
          auto _entry = ibuffer_entry(_pc, _wid, _kid, _uid);
          m_ibuffer->push_back(global_all_kernels_warp_id, _entry);
          
          m_inst_fetch_buffer->m_valid = false;
          std::cout << "  **DECODE: ";
          m_ibuffer->print_ibuffer(_wid);
        } else {
          std::cout << "  **No DECODE cause m_ibuffer->has_free_slot() == false" << std::endl;
        }
      } else {
        std::cout << "  **No DECODE cause m_inst_fetch_buffer->m_valid == false" << std::endl;
      }
      
      // std::cout << "D: gwarp_id_start, gwarp_id_end: " << gwarp_id_start << " " << gwarp_id_end << std::endl;

      // FETCH
      for (auto gwid = gwarp_id_start; gwid <= gwarp_id_end; gwid++) {
        // Round-robin issue
        auto wid = (last_fetch_warp_id + gwid) % warps_per_block + gwarp_id_start;

        // auto curr_warp_id = wid + gwarp_id_start;
        // std::cout << "D: last_fetch_warp_id, gwid, wid: " << last_fetch_warp_id << " " << gwid << " " << wid << std::endl;
        // access ibuffer and access curr_instn_id_per_warp
        // auto global_all_kernels_warp_id = gwid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
        auto global_all_kernels_warp_id = wid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
        
        
        // std::cout << "D: kid, global_all_kernels_warp_id: " << kid << ", " << global_all_kernels_warp_id << std::endl;
        // check if the ibuffer has free slot
        bool fetch_instn = false;
        while (!m_inst_fetch_buffer->m_valid) {
          // curr_instn_id_per_warp stores the current instn id of each warp
          // std::cout << "D: Access curr_instn_id_per_warp: " << kid << " " <<  block_id << " " << gwid - gwarp_id_start << std::endl;
          curr_instn_id_per_warp_entry _entry = curr_instn_id_per_warp_entry(kid, block_id, gwid - gwarp_id_start);
          unsigned fetch_instn_id = curr_instn_id_per_warp[_entry];
          // std::cout << "D: fetch_instn_id: " << fetch_instn_id << std::endl;
          
          if (tracer->get_one_kernel_one_warp_instn_size(kid, wid) <= fetch_instn_id) break;

          compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(kid, wid, fetch_instn_id);
          // std::cout << "D: @@@ " << tmp << std::endl;
          _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
          // m_inst_fetch_buffer = inst_fetch_buffer_entry(tmp_inst_trace->m_pc, wid, kid, fetch_instn_id);
          // if (tmp_inst_trace != nullptr) {
          
            if (!tmp_inst_trace->m_valid) {
              
              // std::cout << "D: tmp_inst_trace == NULL" << std::endl;
              break;
            }

            // std::cout << "D: @@@ "  << tmp_inst_trace->m_pc << std::endl;
          
            m_inst_fetch_buffer->pc = tmp_inst_trace->m_pc;
            m_inst_fetch_buffer->wid = wid;
            m_inst_fetch_buffer->kid = kid;
            m_inst_fetch_buffer->uid = fetch_instn_id;
            m_inst_fetch_buffer->m_valid = true;
            // std::cout << "D: @@@" << std::endl;
            
            std::cout << "  **Fetch instn (pc, gwid, kid, fetch_instn_id): " << "(" << tmp_inst_trace->m_pc << ", " 
                                                                              << wid << ", " 
                                                                              << kid << ", " 
                                                                              << fetch_instn_id << ")" << std::endl;
            fetch_instn = true;
            curr_instn_id_per_warp[_entry]++;
          // }
        }
        
        if (fetch_instn) break;
        else {
          std::cout << "  **No FETCH" << std::endl;
        }

      }

      last_fetch_warp_id = (last_fetch_warp_id + 1) % warps_per_block;

      // for (auto gwid = gwarp_id_start; gwid <= gwarp_id_end; gwid++) {
      //   // Round-robin issue
      //   auto wid = (last_fetch_warp_id + gwid + 1) % warps_per_block + gwarp_id_start;

      //   // auto curr_warp_id = wid + gwarp_id_start;

      //   // access ibuffer and access curr_instn_id_per_warp
      //   // auto global_all_kernels_warp_id = gwid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
      //   auto global_all_kernels_warp_id = wid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
      //   // check if the ibuffer has free slot
      //   bool fetch_instn = false;
      //   while (m_ibuffer->has_free_slot(global_all_kernels_warp_id)) {
      //     // curr_instn_id_per_warp stores the current instn id of each warp
      //     unsigned fetch_instn_id = curr_instn_id_per_warp[global_all_kernels_warp_id];
      //     // compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(kid, gwid, fetch_instn_id);
      //     compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(kid, wid, fetch_instn_id);
      //     _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
      //     trace_warp_inst_t* tmp_trace_warp_inst = &(tmp->trace_warp_inst);
      //     // auto _entry = ibuffer_entry(tmp_inst_trace->m_pc, gwid, kid, fetch_instn_id);
      //     auto _entry = ibuffer_entry(tmp_inst_trace->m_pc, wid, kid, fetch_instn_id);
      //     m_ibuffer->push_back(global_all_kernels_warp_id, _entry);

          
      //     std::cout << "  Fetch instn (pc, gwid, kid, fetch_instn_id): " << "(" << tmp_inst_trace->m_pc << ", " 
      //                                                                       << wid << ", " 
      //                                                                       << kid << ", " 
      //                                                                       << fetch_instn_id << ")" << std::endl;
      //     fetch_instn = true;
      //     curr_instn_id_per_warp[global_all_kernels_warp_id]++;
      //     last_fetch_warp_id = wid;
      //   }

      //   // m_ibuffer->print_ibuffer(gwid);
      //   m_ibuffer->print_ibuffer(wid);

        
      //   if (fetch_instn) break;

      // }
    }
    
    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                            Release all register bank state.                            ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    for (unsigned i = 0; i < num_banks; i++) {
      if (m_reg_bank_allocator->getBankState(i) == ON_WRITING || 
          m_reg_bank_allocator->getBankState(i) == ON_READING) {
        m_reg_bank_allocator->releaseBankState(i);
      }
    }

  // std::cout << "D: !!!!" << std::endl;
  // }

  // std::cout << "$ SM-" << m_smid << " Kernel NUM: " 
  //           << tracer->get_appcfg()->get_kernels_num() << std::endl;

  for (unsigned _w_id_ = 0; _w_id_ <= gwarp_id_end - gwarp_id_start; _w_id_++) {
    std::cout << "D: SM-" << m_smid << " Kernel-" << kid << " Warp-" << _w_id_ << " active status: " << m_warp_active_status[kid][_w_id_] << std::endl;
    if (m_warp_active_status[kid][_w_id_]) break;
    else if (!m_warp_active_status[kid][_w_id_] && _w_id_ == gwarp_id_end - gwarp_id_start) {
      active = false;
      std::cout << "D: SM-" << m_smid << " Kernel-" << kid << " is finished." << std::endl;
    }
  }

}