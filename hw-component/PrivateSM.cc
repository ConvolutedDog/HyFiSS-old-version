

#include "PrivateSM.h"


PrivateSM::PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg){
  m_smid = smid;
  m_cycle = 0;
  active = true;

  this->tracer = tracer;
  issuecfg = this->tracer->get_issuecfg();
  appcfg = this->tracer->get_appcfg();  
  instncfg = this->tracer->get_instncfg();

  // (kernel_id, block_id) pair that are allocated to this SM
  kernel_block_pair = issuecfg->get_kernel_block_by_smid(m_smid);

  // number of warps per kernel that are allocated to this SM
  m_num_warps_per_sm.resize(appcfg->get_kernels_num(), 0);

  // m_num_warps_per_sm[i] stores the i-th kernel's warps number that are allocated to this SM
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++){
    unsigned kid = it->first - 1;
    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    m_num_warps_per_sm[kid] += _warps_per_block;
  }

  // sum of std::vector<unsigned> m_num_warps_per_sm
  all_warps_num = std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.end(), 0);

  // when accessing the ibuffer, the index is:
  //     global_all_kernels_warp_id = gwarp_id + sum_{kid = 0,1,...,kernel_id-1}(m_num_warps_per_sm[kid])
  m_ibuffer = new IBuffer(m_smid, all_warps_num);
  last_fetch_warp_id = -1;

  m_inst_fetch_buffer = new inst_fetch_buffer_entry();

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  curr_instn_id_per_warp.resize(all_warps_num, 0);

  num_banks = hw_cfg->get_num_reg_banks();
  bank_warp_shift = hw_cfg->get_bank_warp_shift();
  num_scheds = hw_cfg->get_num_sched_per_sm();
  sub_core_model = hw_cfg->get_sub_core_model();
  banks_per_sched = (unsigned)(num_banks / num_scheds);
  inst_fetch_throughput = hw_cfg->get_inst_fetch_throughput();

  m_reg_bank_allocator = new RegisterBankAllocator(m_smid, 
                                                   num_banks, 
                                                   num_scheds, 
                                                   bank_warp_shift, 
                                                   banks_per_sched);

  parse_blocks_per_kernel();

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

void PrivateSM::run(){
  m_cycle++;

  std::cout << "# cycle: " << m_cycle << std::endl;

  if (m_cycle >= 10) {
    active = false;
  }

  // for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++){
  auto it_kernel_block_pair = kernel_block_pair.begin();
    unsigned kid = it_kernel_block_pair->first - 1;
    
    unsigned block_id = it_kernel_block_pair->second;
    unsigned warps_per_block = appcfg->get_num_warp_per_block(kid);
    /* Calculate gwarp_id:
     *     gwarp_id_start = block_id * warps_per_block
     *     gwarp_id_end   = (block_id + 1) * warps_per_block */
    unsigned gwarp_id_start = warps_per_block * block_id;
    unsigned gwarp_id_end = gwarp_id_start + warps_per_block - 1;

    // std::cout << "$ SM-" << m_smid << " Kernel-" << kid << " Block-" << block_id 
    //           << ", gwarp_id_start: " << gwarp_id_start << ", gwarp_id_end: " 
    //           << gwarp_id_end << std::endl;

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                              Write back to register banks.                             ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    std::vector<std::vector<stage_instns_identifier>::iterator> writeback_stage_instns_to_remove;
    // traverse writeback_stage_instns
    for (auto it = writeback_stage_instns.begin(); it != writeback_stage_instns.end(); it++){
      unsigned _kid = it->kid;
      unsigned _pc = it->pc;
      unsigned _wid = it->wid;
      unsigned _uid = it->uid;
      /*
      // get the dst reg id of the instn (_kid, _pc, _wid, _uid)
      auto _inst_trace = tracer->get_one_kernel_one_warp_one_instn(_kid, _wid, _uid)->inst_trace;
      unsigned dst_reg_num = _inst_trace->reg_dsts_num;
      for (unsigned i = 0; i < dst_reg_num; i++){
        int dst_reg_id = _inst_trace->reg_dest[i];
        std::cout << "    dst_reg_id[" << i << "]: " << dst_reg_id << std::endl;
        // Calculate the scheduler id of the dst reg id
        if (dst_reg_id >= 0) {
          auto local_wid = (unsigned)(_wid % warps_per_block);
          auto sched_id = (unsigned)(local_wid % num_scheds);
          // Calculate the bank id of the dst reg id
          auto bank_id = register_bank(dst_reg_id, local_wid, sched_id);
          std::cout << "_kid, _wid, _uid, dst_reg_num, dst_reg_id, bank_id, sched_id: " << _kid << ", " 
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
            std::cout << "    setBankState(" << bank_id << ", ON_WRITING)" << std::endl;
            // _inst_trace->reg_dest[i] = -1;
          }
          std::cout << "                               dst_reg_id : " << _inst_trace->reg_dest[i] << std::endl;
        }
      }
      */

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

    

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                       Transfer instns to writeback_stage_instns.                       ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    for (auto gwid = gwarp_id_start; gwid <= gwarp_id_end; gwid++) {
      auto global_all_kernels_warp_id = gwid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
      // check if the ibuffer has free slot
      if (m_ibuffer->is_not_empty(global_all_kernels_warp_id)) {
        // pop the instn from ibuffer
        ibuffer_entry entry = m_ibuffer->pop_front(global_all_kernels_warp_id);
        unsigned _fetch_instn_id = entry.uid;
        unsigned _pc = entry.pc;
        unsigned _gwid = entry.wid;
        unsigned _kid = entry.kid;
        writeback_stage_instns.push_back(stage_instns_identifier(_kid, _pc, _gwid, _fetch_instn_id));
        std::cout << "  Transfer instn (pc, gwid, kid, fetch_instn_id): (" << _pc << ", " 
                                                                           << _gwid << ", " 
                                                                           << _kid << ", " 
                                                                           << _fetch_instn_id << ")" << std::endl;
        m_ibuffer->print_ibuffer(gwid);
      }
    }
        
    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                                Decode instns to Ibuffer.                               ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                                Fetch instns to Fbuffer.                                ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    for (auto _ = 0; _ < get_inst_fetch_throughput(); _++) {
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
          std::cout << "    DECODE: ";
          m_ibuffer->print_ibuffer(_wid);
        }
      }
      


      // FETCH
      for (auto gwid = gwarp_id_start; gwid <= gwarp_id_end; gwid++) {
        // Round-robin issue
        auto wid = (last_fetch_warp_id + gwid + 1) % warps_per_block + gwarp_id_start;

        // auto curr_warp_id = wid + gwarp_id_start;

        // access ibuffer and access curr_instn_id_per_warp
        // auto global_all_kernels_warp_id = gwid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
        auto global_all_kernels_warp_id = wid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + kid, 0);
        // check if the ibuffer has free slot
        bool fetch_instn = false;
        while (!m_inst_fetch_buffer->m_valid) {
          // curr_instn_id_per_warp stores the current instn id of each warp
          unsigned fetch_instn_id = curr_instn_id_per_warp[global_all_kernels_warp_id];
          compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(kid, wid, fetch_instn_id);
          _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
          // m_inst_fetch_buffer = inst_fetch_buffer_entry(tmp_inst_trace->m_pc, wid, kid, fetch_instn_id);
          m_inst_fetch_buffer->pc = tmp_inst_trace->m_pc;
          m_inst_fetch_buffer->wid = wid;
          m_inst_fetch_buffer->kid = kid;
          m_inst_fetch_buffer->uid = fetch_instn_id;
          m_inst_fetch_buffer->m_valid = true;
          
          std::cout << "  Fetch instn (pc, gwid, kid, fetch_instn_id): " << "(" << tmp_inst_trace->m_pc << ", " 
                                                                            << wid << ", " 
                                                                            << kid << ", " 
                                                                            << fetch_instn_id << ")" << std::endl;
          fetch_instn = true;
          curr_instn_id_per_warp[global_all_kernels_warp_id]++;
          last_fetch_warp_id = wid;
        }

        if (fetch_instn) break;

      }

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
    


  // }

  // std::cout << "$ SM-" << m_smid << " Kernel NUM: " 
  //           << tracer->get_appcfg()->get_kernels_num() << std::endl;
}