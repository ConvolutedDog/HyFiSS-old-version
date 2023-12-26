
#include "OperandCollector.h"

unsigned register_bank_opcoll(unsigned regnum, 
                              unsigned wid, 
                              unsigned num_banks,
                              bool bank_warp_shift, 
                              bool sub_core_model,
                              unsigned banks_per_sched,
                              unsigned sched_id) {
  unsigned bank = regnum;
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

void opndcoll_rfu_t::add_cu_set(unsigned set_id, unsigned num_cu,
                                unsigned num_dispatch) {
  m_cus[set_id].reserve(num_cu);  // this is necessary to stop pointers in m_cu
                                  // from being invalid do to a resize;
  for (unsigned i = 0; i < num_cu; i++) {
    // m_cus[set_id].push_back(collector_unit_t());
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
    // m_cus[set_id].push_back(*(new collector_unit_t(m_hw_cfg, m_tracer)));
    m_cus[set_id].emplace_back(m_hw_cfg, m_tracer);
    m_cu.push_back(&m_cus[set_id].back());
    std::cout << "add_cu_set: " << i << " num_cu: " << num_cu << std::endl;
  }
  // for now each collector set gets dedicated dispatch units.
  for (unsigned i = 0; i < num_dispatch; i++) {
    m_dispatch_units.push_back(dispatch_unit_t(&m_cus[set_id]));
  }
}

void opndcoll_rfu_t::add_port(port_vector_t &input, port_vector_t &output,
                              const uint_vector_t cu_sets) {
  // m_num_ports++;
  // m_num_collectors += num_collector_units;
  // m_input.resize(m_num_ports);
  // m_output.resize(m_num_ports);
  // m_num_collector_units.resize(m_num_ports);
  // m_input[m_num_ports-1]=input_port;
  // m_output[m_num_ports-1]=output_port;
  // m_num_collector_units[m_num_ports-1]=num_collector_units;
  m_in_ports.push_back(input_port_t(input, output, cu_sets));
}

void opndcoll_rfu_t::init(hw_config* hw_cfg, 
                          RegisterBankAllocator* reg_bank_allocator,
                          trace_parser* tracer) {

  // m_reg_bank_allocator = reg_bank_allocator;

  // m_hw_cfg = hw_cfg;
  // m_tracer = tracer;
  

  unsigned num_banks = m_hw_cfg->get_num_reg_banks();

  m_arbiter = arbiter_t(m_reg_bank_allocator);

  std::cout << "777" << std::endl;
  m_arbiter.init(m_cu.size(), num_banks);
  std::cout << "444" << std::endl;
  // for( unsigned n=0; n<m_num_ports;n++ )
  //    m_dispatch_units[m_output[n]].init( m_num_collector_units[n] );
  m_num_banks = num_banks;
  m_bank_warp_shift = 0;
  m_warp_size = m_hw_cfg->get_warp_size();
  m_bank_warp_shift = (unsigned)(int)(log(m_warp_size + 0.5) / log(2.0));
  assert((m_bank_warp_shift == 5) || (m_warp_size != 32));

  sub_core_model = m_hw_cfg->get_sub_core_model();
  m_num_warp_scheds = m_hw_cfg->get_num_sched_per_sm();
  unsigned reg_id = -1;
  if (sub_core_model) {
    assert(num_banks % m_num_warp_scheds == 0);
    assert(m_num_warp_scheds <= m_cu.size() &&
           m_cu.size() % m_num_warp_scheds == 0);
  }
  m_num_banks_per_sched =
      num_banks / m_num_warp_scheds;

  for (unsigned j = 0; j < m_cu.size(); j++) {
    if (sub_core_model) {
      unsigned cusPerSched = m_cu.size() / m_num_warp_scheds;
      reg_id = j / cusPerSched;
    }
    m_cu[j]->init(j, num_banks, m_bank_warp_shift, m_hw_cfg, this,
                  sub_core_model, reg_id, m_num_banks_per_sched);
  }
  
  std::cout << "555" << std::endl;
  for (unsigned j = 0; j < m_dispatch_units.size(); j++) {
    m_dispatch_units[j].init(sub_core_model, m_num_warp_scheds);
  }
  
  std::cout << "666" << std::endl;
  m_initialized = true;
}

void opndcoll_rfu_t::dispatch_ready_cu() {
  // std::cout << "m_dispatch_units.size(): " << m_dispatch_units.size() << std::endl;
  for (unsigned p = 0; p < m_dispatch_units.size(); ++p) {
    dispatch_unit_t &du = m_dispatch_units[p];
    collector_unit_t *cu = du.find_ready();
    if (cu) {
      cu->dispatch();
    } else {
      std::cout << "    cu is NULL " << cu << std::endl;
    }
  }
}

void opndcoll_rfu_t::allocate_cu(unsigned port_num) {
  input_port_t &inp = m_in_ports[port_num];
  std::cout << "    for allocate_cu port_num: " << port_num << std::endl;
  for (unsigned i = 0; i < inp.m_in.size(); i++) {
    // std::cout << "    i: " << i << " inp.m_in.size(): " << inp.m_in.size() << std::endl;
    if ((*inp.m_in[i]).has_ready()) {
      std::cout << "      has_ready_pipeline_reg_idx: " << i << std::endl;
      // find a free cu
      for (unsigned j = 0; j < inp.m_cu_sets.size(); j++) {
        std::vector<collector_unit_t> &cu_set = m_cus[inp.m_cu_sets[j]];
        bool allocated = false;
        unsigned cuLowerBound = 0;
        unsigned cuUpperBound = cu_set.size();
        unsigned schd_id;
        if (sub_core_model) {
          // Sub core model only allocates on the subset of CUs assigned to the
          // scheduler that issued
          unsigned reg_id = (*inp.m_in[i]).get_ready_reg_id();
          std::cout << "      get_ready_regset_slot: " << reg_id << std::endl;
          schd_id = (*inp.m_in[i]).get_schd_id(reg_id);
          assert(cu_set.size() % m_num_warp_scheds == 0 &&
                 cu_set.size() >= m_num_warp_scheds);
          unsigned cusPerSched = cu_set.size() / m_num_warp_scheds;
          cuLowerBound = schd_id * cusPerSched;
          cuUpperBound = cuLowerBound + cusPerSched;
          assert(0 <= cuLowerBound && cuUpperBound <= cu_set.size());
          std::cout << "      schd_id: " << schd_id << std::endl;
          std::cout << "      cuLowerBound: " << cuLowerBound << std::endl;
          std::cout << "      cuUpperBound: " << cuUpperBound << std::endl;
        }
        for (unsigned k = cuLowerBound; k < cuUpperBound; k++) {
          if (cu_set[k].is_free()) {
            std::cout << "      cu_set[" << k << "] is free" << std::endl;
            collector_unit_t *cu = &cu_set[k];
            allocated = cu->allocate(inp.m_in[i], inp.m_out[i]);
            std::cout << "      allocated: " << allocated << std::endl;
            m_arbiter.add_read_requests(cu);
            break;
          }
        }
        if (allocated) break;  // cu has been allocated, no need to search more.
      }
      // break;  // can only service a single input, if it failed it will fail
      // for
      // others.
    }
  }
}


void opndcoll_rfu_t::allocate_reads() {
  // process read requests that do not have conflicts
  std::list<op_t> allocated = m_arbiter.allocate_reads();
  std::map<unsigned, op_t> read_ops;
  for (std::list<op_t>::iterator r = allocated.begin(); r != allocated.end();
       r++) {
    const op_t &rr = *r;
    unsigned reg = rr.get_reg();
    unsigned wid = rr.get_wid();
    unsigned bank =
        register_bank_opcoll(reg, wid, m_num_banks, m_bank_warp_shift, sub_core_model,
                      m_num_banks_per_sched, rr.get_sid());
    std::cout << "    read_ops: reg: " << reg << " wid: " << wid << " bank: " << bank << std::endl;
    m_arbiter.allocate_for_read(bank, rr);
    read_ops[bank] = rr;
  }
  std::map<unsigned, op_t>::iterator r;
  for (r = read_ops.begin(); r != read_ops.end(); ++r) {
    op_t &op = r->second;
    unsigned cu = op.get_oc_id();
    unsigned operand = op.get_operand();
    m_cu[cu]->collect_operand(operand);
    std::cout << "    m_cu[cu]->get_num_operands(): " << m_cu[cu]->get_num_operands() << std::endl;
    std::cout << "    m_cu[" << cu << "]->m_not_ready: " << m_cu[cu]->get_not_ready() << std::endl;
  }
}

bool opndcoll_rfu_t::collector_unit_t::ready() const {
  // std::cout << "    !m_free: " << !m_free << std::endl;
  // std::cout << "    m_not_ready.none(): " << m_not_ready.none() << std::endl;
  // if (m_output_register != NULL)
  //   std::cout << "    (*m_output_register).has_free(m_sub_core_model, m_reg_id): " 
  //             << (*m_output_register).has_free(m_sub_core_model, m_reg_id) << std::endl;
  if (m_output_register != NULL)
    return (!m_free) && m_not_ready.none() &&
           (*m_output_register).has_free(m_sub_core_model, m_reg_id);
  else
    return false;
}

// void opndcoll_rfu_t::collector_unit_t::dump(
//     FILE *fp, const PrivateSM *shader) const {
//   if (m_free) {
//     fprintf(fp, "    <free>\n");
//   } else {
//     // m_warp->print(fp);
//     std::cout << "    pc: " << m_warp->pc << std::endl;
//     std::cout << "    wid: " << m_warp->wid << std::endl;
//     std::cout << "    kid: " << m_warp->kid << std::endl;
//     std::cout << "    uid: " << m_warp->uid << std::endl;
//     for (unsigned i = 0; i < MAX_REG_OPERANDS * 2; i++) {
//       if (m_not_ready.test(i)) {
//         std::string r = m_src_op[i].get_reg_string();
//         fprintf(fp, "    '%s' not ready\n", r.c_str());
//       }
//     }
//   }
// }

void opndcoll_rfu_t::collector_unit_t::init(
    unsigned n, unsigned num_banks, unsigned log2_warp_size, 
    const hw_config *config, opndcoll_rfu_t *rfu, bool sub_core_model, 
    unsigned reg_id, unsigned num_banks_per_sched) {
  m_rfu = rfu;
  m_cuid = n;
  m_num_banks = num_banks;
  assert(m_warp == NULL);
  m_warp = new inst_fetch_buffer_entry();
  m_bank_warp_shift = log2_warp_size;
  m_sub_core_model = sub_core_model;
  m_reg_id = reg_id;
  m_num_banks_per_sched = num_banks_per_sched;
  m_hw_cfg = config;
}

bool opndcoll_rfu_t::collector_unit_t::allocate(register_set *pipeline_reg_set,
                                                register_set *output_reg_set) {
  assert(m_free);
  assert(m_not_ready.none());
  m_free = false;
  m_output_register = output_reg_set;
  inst_fetch_buffer_entry **pipeline_reg = pipeline_reg_set->get_ready();
  // std::cout << "  pipeline_reg: " << pipeline_reg << std::endl;
  // std::cout << "  *pipeline_reg: " << *pipeline_reg << std::endl;
  // std::cout << "  (*pipeline_reg)->m_valid: " << (*pipeline_reg)->m_valid << std::endl;
  // std::cout << "  (*pipeline_reg)->wid: " << (*pipeline_reg)->wid << std::endl;
  // std::cout << "  (*pipeline_reg)->kid: " << (*pipeline_reg)->kid << std::endl;
  // std::cout << "  (*pipeline_reg)->uid: " << (*pipeline_reg)->uid << std::endl;
  // std::cout << "  m_tracer: " << m_tracer << std::endl;
  // std::cout << "  m_tracer->get_m_valid(): " << m_tracer->get_m_valid() << std::endl;
  if ((pipeline_reg) and ((*pipeline_reg)->m_valid)) {
    m_warp_id = (*pipeline_reg)->wid;
    compute_instn* tmp = 
      m_tracer->get_one_kernel_one_warp_one_instn((*pipeline_reg)->kid, 
                                                  (*pipeline_reg)->wid, 
                                                  (*pipeline_reg)->uid);
    // trace_warp_inst_t* tmp_trace_warp_inst = &(tmp->trace_warp_inst);
    _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
    std::vector<int> prev_regs; // remove duplicate regs within same instr
    std::cout << "      reg_srcs_num: " << tmp_inst_trace->reg_srcs_num << std::endl;
    for (unsigned op = 0; op < tmp_inst_trace->reg_srcs_num; op++) {
      int reg_num = tmp_inst_trace->reg_src[op];
      std::cout << "        reg_num: " << reg_num << std::endl;
      bool new_reg = true;
      for (auto r : prev_regs) {
        if (r == reg_num)
          new_reg = false;
      }
      if (reg_num >= 0 && new_reg) {          // valid register
        prev_regs.push_back(reg_num);
        // get schd_id 
        auto sched_id = (unsigned)(m_warp_id % m_hw_cfg->get_num_sched_per_sm());
        m_src_op[op] = op_t(this, op, reg_num, m_num_banks, m_bank_warp_shift,
                            m_sub_core_model, m_num_banks_per_sched,
                            sched_id, m_tracer);
        m_not_ready.set(op);
      } else 
        m_src_op[op] = op_t();
    }
    for (auto x : prev_regs)
      std::cout << "      prev_regs: " << x << std::endl;
    // move_warp(m_warp,*pipeline_reg) && set (*pipeline_reg)->m_valid = false;
    pipeline_reg_set->move_out_to(m_warp);
    
    std::cout << "      Status of m_warp : " << std::endl;
    std::cout << "        m_warp->pc: " << m_warp->pc << std::endl;
    std::cout << "        m_warp->wid: " << m_warp->wid << std::endl;
    std::cout << "        m_warp->kid: " << m_warp->kid << std::endl;
    std::cout << "        m_warp->uid: " << m_warp->uid << std::endl;
    std::cout << "        m_warp->m_valid: " << m_warp->m_valid << std::endl;

    return true;
  }
  return false;
}

void opndcoll_rfu_t::collector_unit_t::dispatch() {
  assert(m_not_ready.none());
  std::cout << "    dispatch: " << std::endl;
  std::cout << "      m_sub_core_model: " << m_sub_core_model << std::endl;
  std::cout << "      m_reg_id: " << m_reg_id << std::endl;
  std::cout << "      m_warp->pc: " << m_warp->pc << std::endl;
  std::cout << "      m_warp->wid: " << m_warp->wid << std::endl;
  std::cout << "      m_warp->kid: " << m_warp->kid << std::endl;
  std::cout << "      m_warp->uid: " << m_warp->uid << std::endl;
  std::cout << "      m_warp->m_valid: " << m_warp->m_valid << std::endl;

  m_output_register->move_in(m_sub_core_model, m_reg_id, m_warp);
  m_warp->m_valid = false;

  std::cout << "    after dispatch: " << std::endl;
  std::cout << "      m_warp->m_valid: " << m_warp->m_valid << std::endl;
  m_output_register->print_regs(m_reg_id);

  m_free = true;
  // m_output_register = NULL; // may not be necessary
  for (unsigned i = 0; i < MAX_REG_OPERANDS * 2; i++) m_src_op[i].reset();
}

std::list<opndcoll_rfu_t::op_t> opndcoll_rfu_t::arbiter_t::allocate_reads() {
  std::list<op_t>
      result;  // a list of registers that (a) are in different register banks,
               // (b) do not go to the same operand collector

  int input;
  int output;
  int _inputs = m_num_banks;
  int _outputs = m_num_collectors;
  int _square = (_inputs > _outputs) ? _inputs : _outputs;
  assert(_square > 0);
  int _pri = (int)m_last_cu;

  // Clear matching
  for (int i = 0; i < _inputs; ++i) _inmatch[i] = -1;
  for (int j = 0; j < _outputs; ++j) _outmatch[j] = -1;

  for (unsigned i = 0; i < m_num_banks; i++) {
    for (unsigned j = 0; j < m_num_collectors; j++) {
      assert(i < (unsigned)_inputs);
      assert(j < (unsigned)_outputs);
      /* In current cycle, if there is request from the j-th collector to the i-th bank,
       * we set _request[i][j] = 1. */
      _request[i][j] = 0;
    }
    /* m_queue = new std::list<op_t>[num_banks], and the idxes of m_queue is bank_id,
     * and items of m_queue[bank_id] is the quest reg_nums that are allocated to bank_id.*/
    if (!m_queue[i].empty()) {
      const op_t &op = m_queue[i].front();
      int oc_id = op.get_oc_id();
      assert(i < (unsigned)_inputs);
      assert(oc_id < _outputs);
      _request[i][oc_id] = 1;
    }
    if (m_allocated_bank[i].is_write()) {
      /* If the i-th bank is allocated for writing during the write-back process, we set
       * _inmatch[i] = 0, cause the writing behaviour has the highest priority. */
      assert(i < (unsigned)_inputs);
      _inmatch[i] = 0;  // write gets priority
    }
  }

  ///// wavefront allocator from booksim... --->

  // Loop through diagonals of request matrix
  // printf("####\n");

  for (int p = 0; p < _square; ++p) {
    output = (_pri + p) % _outputs;

    // Step through the current diagonal
    // int _inputs = m_num_banks;
    // int _outputs = m_num_collectors;
    for (input = 0; input < _inputs; ++input) {
      assert(input < _inputs);
      assert(output < _outputs);
      if ((output < _outputs) && (_inmatch[input] == -1) &&
          //( _outmatch[output] == -1 ) &&   //allow OC to read multiple reg
          // banks at the same cycle
          (_request[input][output] /*.label != -1*/)) {
        // Grant!
        _inmatch[input] = output;
        _outmatch[output] = input;
        // printf("Register File: granting bank %d to OC %d, schedid %d, warpid
        // %d, Regid %d\n", input, output, (m_queue[input].front()).get_sid(),
        // (m_queue[input].front()).get_wid(),
        // (m_queue[input].front()).get_reg());
      }

      output = (output + 1) % _outputs;
    }
  }

  // Round-robin the priority diagonal
  _pri = (_pri + 1) % _outputs;

  /// <--- end code from booksim

  m_last_cu = _pri;
  for (unsigned i = 0; i < m_num_banks; i++) {
    if (_inmatch[i] != -1) {
      if (!m_allocated_bank[i].is_write()) {
        unsigned bank = (unsigned)i;
        op_t &op = m_queue[bank].front();
        result.push_back(op);
        m_queue[bank].pop_front();
      }
    }
  }

  // print result
  if (result.size() > 0)
    std::cout << "    allocate_reads result: " << std::endl;
  for (auto x : result) {
    std::cout << "      get_wid(): " << x.get_wid() << std::endl;
    std::cout << "      get_reg(): " << x.get_reg() << std::endl;
    std::cout << "      get_sid(): " << x.get_sid() << std::endl;
    std::cout << "      get_active_count(): " << x.get_active_count() << std::endl;
    std::cout << "      get_active_mask(): " << x.get_active_mask() << std::endl; 
    std::cout << "      get_oc_id(): " << x.get_oc_id() << std::endl;
    std::cout << "      get_bank(): " << x.get_bank() << std::endl;
    std::cout << "      get_operand(): " << x.get_operand() << std::endl;
    std::cout << "      get_reg_string(): " << x.get_reg_string() << std::endl;
  }

  return result;
}
