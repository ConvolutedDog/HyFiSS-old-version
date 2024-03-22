
#include "PipelineUnit.h"

pipelined_simd_unit::pipelined_simd_unit(register_set *result_port,
                                         unsigned max_latency,
                                         unsigned issue_reg_id, 
                                         hw_config* hw_cfg, trace_parser* tracer) {
  m_pipeline_depth = max_latency;
  m_pipeline_reg.reserve(m_pipeline_depth);
  for (unsigned i = 0; i < m_pipeline_depth; i++) {
    m_pipeline_reg.push_back(new inst_fetch_buffer_entry());
  }
  m_result_port = result_port;
  m_issue_reg_id = issue_reg_id;
  m_dispatch_reg = new inst_fetch_buffer_entry();
  m_hw_cfg = hw_cfg;
  m_tracer = tracer;
  occupied.reset();
  active_insts_in_pipeline = 0;
}

pipelined_simd_unit::~pipelined_simd_unit() {
  for (unsigned i = 0; i < m_pipeline_depth; i++) {
    delete m_pipeline_reg[i];
  }
  delete m_dispatch_reg;
}

bool pipelined_simd_unit::can_issue(unsigned latency) const {
  // std::cout << "      can_issue: " << !m_dispatch_reg->m_valid << " " 
  //           << !occupied.test(latency) << std::endl;
  // std::cout << "      occupied.to_string(): ";
  // for (int i = 0; i < 32; ++i) {
  //   std::cout << occupied[i];
  // }
  // std::cout << '\n';
  return !m_dispatch_reg->m_valid && !occupied.test(latency);
}

std::vector<unsigned> pipelined_simd_unit::cycle(trace_parser* tracer, 
                                                 Scoreboard* m_scoreboard, app_config* appcfg,
                                                 std::vector<std::pair<int, int>>* kernel_block_pair,
                                                 std::vector<unsigned>* m_num_warps_per_sm,
                                                 unsigned KERNEL_EVALUATION,
                                                 unsigned num_scheds,
                                                 RegisterBankAllocator* m_reg_bank_allocator,
                                                 bool* flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle,
                                                 std::map<std::tuple<unsigned, unsigned, unsigned>, 
                                                   std::tuple<unsigned, unsigned, unsigned, unsigned, 
                                                              unsigned, unsigned>>* clk_record,
                                                 unsigned m_cycle) {

  std::vector<unsigned> need_to_return_wids;

  bool active_during_this_cycle = false;
  // print m_pipeline_reg
  // for (int s = m_pipeline_depth - 1; s >= 0; s--) {
  //   if (m_pipeline_reg[s]->m_valid) {
  //     std::cout << "      " << m_name.c_str() << "[" << s << "] ";
  //     std::cout << "m_pipeline_reg[" << s << "]: (pc,wid,kid,uid) "
  //               << m_pipeline_reg[s]->pc << " " << m_pipeline_reg[s]->wid
  //               << " " << m_pipeline_reg[s]->kid << " "
  //               << m_pipeline_reg[s]->uid << std::endl;
  //   }
  // }

  // if (m_pipeline_reg[0]->m_valid) {
  bool should_shift_pipeline = false;
  // m_result_port->print();
  if (m_pipeline_reg[0]->m_valid && (m_result_port->get_free() != NULL)) {
    if (_CALIBRATION_LOG_) {
      std::cout << "    Execute: ("
                << m_pipeline_reg[0]->kid << ", "
                << m_pipeline_reg[0]->wid << ", "
                << m_pipeline_reg[0]->uid << ", "
                << m_pipeline_reg[0]->pc << ")" << std::endl;
    }
    set_clk_record<4>(clk_record, m_pipeline_reg[0]->kid, m_pipeline_reg[0]->wid, m_pipeline_reg[0]->uid, m_cycle);

    // std::cout << "@" << (m_result_port->get_free() != NULL) << std::endl;
    if (m_result_port->get_free() != NULL) {
      active_during_this_cycle = true;
      need_to_return_wids.push_back(m_pipeline_reg[0]->wid);

      m_result_port->move_in(m_pipeline_reg[0]);
      assert(active_insts_in_pipeline > 0);
      active_insts_in_pipeline--;
      should_shift_pipeline = true;
    }
    // m_result_port->move_in(m_pipeline_reg[0]);
    // assert(active_insts_in_pipeline > 0);
    // active_insts_in_pipeline--;
  }

/*
  // if (active_insts_in_pipeline) {
  if (should_shift_pipeline && active_insts_in_pipeline) {
    // print m_pipeline_reg
    std::cout << "    bef " << m_name << "-pipe: ";
    for (int s = 0; s <= 32 - 1; s++) {
      if (m_pipeline_reg[s]->m_valid) {
        std::cout << "1";
      }
      else {
        std::cout << "0";
      }
    }
    std::cout << std::endl;
    // for (unsigned stage = 0; stage < m_pipeline_depth - 1; stage++) {
    //   m_pipeline_reg[stage]->move_in(m_pipeline_reg[stage + 1]);
    // }
    std::rotate(m_pipeline_reg.begin(), m_pipeline_reg.begin() + 1, m_pipeline_reg.end());
    m_pipeline_reg[m_pipeline_depth - 1]->m_valid = false;

    // print m_pipeline_reg
    std::cout << "    aft " << m_name << "-pipe: ";
    for (int s = 0; s <= 32 - 1; s++) {
      if (m_pipeline_reg[s]->m_valid) {
        std::cout << "1";
      }
      else {
        std::cout << "0";
      }
    }
    std::cout << std::endl;
  }
*/




/*
  if (active_insts_in_pipeline) {
    if (!m_pipeline_reg[0]->m_valid) {
      for (unsigned stage = 0; stage < m_pipeline_depth - 1; stage++) {
        // m_pipeline_reg[stage]->move_in(m_pipeline_reg[stage + 1]);
        m_pipeline_reg[stage]->m_valid = m_pipeline_reg[stage + 1]->m_valid;
        m_pipeline_reg[stage]->pc = m_pipeline_reg[stage + 1]->pc;
        m_pipeline_reg[stage]->wid = m_pipeline_reg[stage + 1]->wid;
        m_pipeline_reg[stage]->kid = m_pipeline_reg[stage + 1]->kid;
        m_pipeline_reg[stage]->uid = m_pipeline_reg[stage + 1]->uid;
        m_pipeline_reg[stage]->latency = m_pipeline_reg[stage + 1]->latency;
        m_pipeline_reg[stage]->initial_interval = m_pipeline_reg[stage + 1]->initial_interval;
        m_pipeline_reg[stage + 1]->m_valid = false;
      }
    }
  }
*/
  if (active_insts_in_pipeline) {
    active_during_this_cycle = true;
    for (unsigned stage = 0; stage < m_pipeline_depth - 1; stage++) {
      if (!m_pipeline_reg[stage]->m_valid) {
        need_to_return_wids.push_back(m_pipeline_reg[stage + 1]->wid);
        m_pipeline_reg[stage]->m_valid = m_pipeline_reg[stage + 1]->m_valid;
        m_pipeline_reg[stage]->pc = m_pipeline_reg[stage + 1]->pc;
        m_pipeline_reg[stage]->wid = m_pipeline_reg[stage + 1]->wid;
        m_pipeline_reg[stage]->kid = m_pipeline_reg[stage + 1]->kid;
        m_pipeline_reg[stage]->uid = m_pipeline_reg[stage + 1]->uid;
        m_pipeline_reg[stage]->latency = m_pipeline_reg[stage + 1]->latency;
        m_pipeline_reg[stage]->initial_interval = m_pipeline_reg[stage + 1]->initial_interval;
        m_pipeline_reg[stage + 1]->m_valid = false;
      }
    }
  }







  if (_DEBUG_LOG_) {
    bool print_status = false;
    for (auto &reg : m_pipeline_reg) {
      if (reg->m_valid) {
        print_status = true;
        break;
      }
    }
    if (print_status) {
      std::cout << "    UNIT " << m_name << " 's m_pipeline_reg status:" << std::endl;
      for (auto &reg : m_pipeline_reg) {
        std::cout << "    reg: valid-" << reg->m_valid 
                  << ",pc-" << reg->pc 
                  << ",kid-" << reg->kid 
                  << ",wid-" << reg->wid 
                  << ",uid-" << reg->uid << std::endl;
      }
    }
  }

/*
  if (m_dispatch_reg->m_valid) {
    std::cout << "    m_dispatch_reg latency: " << m_dispatch_reg->latency 
              << " initial_interval: " << m_dispatch_reg->initial_interval 
              << " dec_counter: " << m_dispatch_reg->initial_interval_dec_counter << std::endl;
    if (m_dispatch_reg->latency == 0) {
      // m_pipeline_reg[0]->move_in(m_dispatch_reg);
      int start_stage =
          m_dispatch_reg->latency - m_dispatch_reg->initial_interval;
      assert(start_stage >= 0 && start_stage < m_pipeline_depth);
      // m_pipeline_reg[start_stage]->move_in(m_dispatch_reg);
      if (m_pipeline_reg[start_stage]->m_valid == false) {
        m_dispatch_reg->m_valid = false;
        active_insts_in_pipeline++;
        m_pipeline_reg[start_stage]->m_valid = true;
        m_pipeline_reg[start_stage]->pc = m_dispatch_reg->pc;
        m_pipeline_reg[start_stage]->wid = m_dispatch_reg->wid;
        m_pipeline_reg[start_stage]->kid = m_dispatch_reg->kid;
        m_pipeline_reg[start_stage]->uid = m_dispatch_reg->uid;
        m_pipeline_reg[start_stage]->latency = m_dispatch_reg->latency;
        m_pipeline_reg[start_stage]->initial_interval = m_dispatch_reg->initial_interval;
      }
    } else {
      m_dispatch_reg->latency--;
    }
  }
*/

  // std::cout << "    m_dispatch_reg->m_valid: " 
  //           << m_name << ": " << m_dispatch_reg->m_valid << std::endl;
  if (m_dispatch_reg->m_valid) {
    // std::cout << "    m_dispatch_reg latency: " << m_dispatch_reg->latency 
    //           << " initial_interval: " << m_dispatch_reg->initial_interval 
    //           << " dec_counter: " << m_dispatch_reg->initial_interval_dec_counter << std::endl;
    if (m_dispatch_reg->initial_interval_dec_counter == 1) {
      int start_stage = m_dispatch_reg->latency - m_dispatch_reg->initial_interval /*+ 1*/;
      if (start_stage < 0) start_stage = 0;
      if (start_stage >= m_pipeline_depth) start_stage = m_pipeline_depth - 1;
      
      // std::cout << "    start_stage: " << start_stage 
      //           << " m_pipeline_depth: " << m_pipeline_depth 
      //           << " ini_intval counter: " 
      //           << m_dispatch_reg->initial_interval_dec_counter
      //           << " latency: " << m_dispatch_reg->latency
      //           << " initial_interval: " << m_dispatch_reg->initial_interval
      //           << std::endl;
      // std::cout << "start_stage: " << start_stage << " " << "m_pipeline_depth: " << m_pipeline_depth << std::endl;
      assert(start_stage >= 0 && start_stage < m_pipeline_depth);
      // m_pipeline_reg[start_stage]->move_in(m_dispatch_reg);
      if (m_pipeline_reg[start_stage]->m_valid == false) {
        active_during_this_cycle = true;
        need_to_return_wids.push_back(m_dispatch_reg->wid);

        m_dispatch_reg->m_valid = false;
        active_insts_in_pipeline++;
        m_pipeline_reg[start_stage]->m_valid = true;
        m_pipeline_reg[start_stage]->pc = m_dispatch_reg->pc;
        m_pipeline_reg[start_stage]->wid = m_dispatch_reg->wid;
        m_pipeline_reg[start_stage]->kid = m_dispatch_reg->kid;
        m_pipeline_reg[start_stage]->uid = m_dispatch_reg->uid;
        m_pipeline_reg[start_stage]->latency = m_dispatch_reg->latency;
        m_pipeline_reg[start_stage]->initial_interval = m_dispatch_reg->initial_interval;
      }
    } else {
      m_dispatch_reg->initial_interval_dec_counter--;
    }
  }

  occupied >>= 1;

  return need_to_return_wids;
}

void pipelined_simd_unit::issue(register_set &source_reg) {
  bool partition_issue =
    m_hw_cfg->get_sub_core_model() && this->is_issue_partitioned();
  inst_fetch_buffer_entry **ready_reg =
    source_reg.get_ready(partition_issue, m_issue_reg_id);
  if (_DEBUG_LOG_)
    std::cout << "    partition_issue: " << partition_issue << std::endl
              << "    m_issue_reg_id: " << m_issue_reg_id << std::endl
              << "    ready_reg: " << ready_reg << std::endl
              << "    NULL: " << NULL << std::endl;

  if (ready_reg != NULL) {
    if (_DEBUG_LOG_)
      std::cout << "    (*ready_reg): " << (*ready_reg)->kid 
                << " " << (*ready_reg)->wid 
                << " " << (*ready_reg)->uid
                << std::endl;
  
    // simd_function_unit::issue(source_reg);

    //source_reg即为流水线寄存器，目的是找到一个非空的指令，将其移入m_dispatch_reg。
    if (_DEBUG_LOG_)
      std::cout << "    source_reg.move_out_to.m_dispatch_reg" << std::endl;

    if (_DEBUG_LOG_) {
      std::cout << "    Before Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
                << "                         kid: " << m_dispatch_reg->kid << std::endl
                << "                         wid: " << m_dispatch_reg->wid << std::endl 
                << "                         uid: " << m_dispatch_reg->uid << std::endl;
      std::cout << "    source before move: " << (*ready_reg)->m_valid << std::endl;
    }

    source_reg.move_out_to(partition_issue, this->get_issue_reg_id(),
                          m_dispatch_reg);

    if (_DEBUG_LOG_) {
      std::cout << "    After Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
                << "                        kid: " << m_dispatch_reg->kid << std::endl
                << "                        wid: " << m_dispatch_reg->wid << std::endl 
                << "                        uid: " << m_dispatch_reg->uid << std::endl;

      std::cout << "    source after move: " << (*ready_reg)->m_valid << std::endl;
    }

    //设置m_dispatch_reg的标识占用位图的状态，m_dispatch_reg是warp_inst_t类型，可获取该指令的延迟。
    if (_DEBUG_LOG_) std::cout << "    occupied.set" << std::endl;
    
    occupied.set(m_dispatch_reg->latency);

    if (_DEBUG_LOG_)
      std::cout << "  ### pipelined_simd_unit::issue() end:" << std::endl
                << "    Instn[pc:" << m_dispatch_reg->pc << ","
                << "kid:" << m_dispatch_reg->kid << ","
                << "wid:" << m_dispatch_reg->wid << ","
                << "uid:" << m_dispatch_reg->uid << "] has been issued to UNIT " 
                << m_name << std::endl;

  }
}

void pipelined_simd_unit::issue(register_set &source_reg, unsigned reg_id) {
  bool partition_issue =
    m_hw_cfg->get_sub_core_model() && this->is_issue_partitioned();
  inst_fetch_buffer_entry **ready_reg =
    source_reg.get_ready(partition_issue, reg_id);
  
  if (_DEBUG_LOG_)
    std::cout << "    partition_issue: " << partition_issue << std::endl
              << "    reg_id: " << reg_id << std::endl
              << "    ready_reg: " << ready_reg << std::endl
              << "    NULL: " << NULL << std::endl;

  if (ready_reg != NULL) {
    if (_DEBUG_LOG_)
      std::cout << "    (*ready_reg): " << (*ready_reg)->kid 
                << " " << (*ready_reg)->wid 
                << " " << (*ready_reg)->uid
                << std::endl;
  
    // simd_function_unit::issue(source_reg);

    //source_reg即为流水线寄存器，目的是找到一个非空的指令，将其移入m_dispatch_reg。
    if (_DEBUG_LOG_) {
      std::cout << "    source_reg.move_out_to.m_dispatch_reg" << std::endl;

      std::cout << "    Before Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
                << "                         kid: " << m_dispatch_reg->kid << std::endl
                << "                         wid: " << m_dispatch_reg->wid << std::endl 
                << "                         uid: " << m_dispatch_reg->uid << std::endl;
      std::cout << "    source before move: " << (*ready_reg)->m_valid << std::endl;
    }

    source_reg.move_out_to(partition_issue, reg_id,
                           m_dispatch_reg);

    if (_DEBUG_LOG_) {
      std::cout << "    After Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
                << "                        kid: " << m_dispatch_reg->kid << std::endl
                << "                        wid: " << m_dispatch_reg->wid << std::endl 
                << "                        uid: " << m_dispatch_reg->uid << std::endl
                << "                    latency: " << m_dispatch_reg->latency << std::endl;

      std::cout << "    source after move: " << (*ready_reg)->m_valid << std::endl;
    }

    //设置m_dispatch_reg的标识占用位图的状态，m_dispatch_reg是warp_inst_t类型，可获取该指令的延迟。
    if (_DEBUG_LOG_) std::cout << "    occupied.set" << std::endl;
    
    occupied.set(m_dispatch_reg->latency);

    if (_DEBUG_LOG_)
      std::cout << "  ### pipelined_simd_unit::issue() end:" << std::endl
                << "    Instn[pc:" << m_dispatch_reg->pc << ","
                << "kid:" << m_dispatch_reg->kid << ","
                << "wid:" << m_dispatch_reg->wid << ","
                << "uid:" << m_dispatch_reg->uid << "] has been issued to UNIT " 
                << m_name << std::endl;

  }
}

bool sfu::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
  if (tmp_inst_trace->get_func_unit() == SFU_UNIT) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void sfu::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = SFU__OP;
  pipelined_simd_unit::issue(source_reg);
}

void sfu::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

bool dp_unit::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
  if (tmp_inst_trace->get_func_unit() == DP_UNIT) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void dp_unit::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = DP___OP;
  pipelined_simd_unit::issue(source_reg);
}

void dp_unit::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

bool sp_unit::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
  if (tmp_inst_trace->get_func_unit() == SP_UNIT) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void sp_unit::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = SP___OP;
  pipelined_simd_unit::issue(source_reg);
}

void sp_unit::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

bool tensor_core::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
  if (tmp_inst_trace->get_func_unit() == TENSOR_CORE_UNIT) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void tensor_core::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = TENSOR_OP;
  pipelined_simd_unit::issue(source_reg);
}

void tensor_core::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

bool int_unit::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
  if (tmp_inst_trace->get_func_unit() == INT_UNIT) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void int_unit::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = INT__OP;
  pipelined_simd_unit::issue(source_reg);
}

void int_unit::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

bool specialized_unit::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;

  bool op_condition = false;

  switch (m_index) {
    case 0:
      op_condition = (tmp_inst_trace->get_func_unit() == SPEC_UNIT_1);
      break;
    case 1:
      op_condition = (tmp_inst_trace->get_func_unit() == SPEC_UNIT_2);
      break;
    case 2:
      op_condition = (tmp_inst_trace->get_func_unit() == SPEC_UNIT_3);
      break;
    default:
      return false;
      break;
  }

  if (op_condition) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void specialized_unit::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = SPECIALIZED_UNIT_1_OP + m_index;
  pipelined_simd_unit::issue(source_reg);
}

void specialized_unit::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

bool mem_unit::can_issue(const inst_fetch_buffer_entry &inst) const {
  unsigned _fetch_instn_id = inst.uid;
  unsigned _gwid = inst.wid;
  unsigned _kid = inst.kid;
  compute_instn* tmp = 
    m_tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
  _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
  if (tmp_inst_trace->get_func_unit() == LDST_UNIT) {
    unsigned latency = tmp_inst_trace->get_latency();
    return pipelined_simd_unit::can_issue(latency);
  } else {
    return false;
  }
}

void mem_unit::issue(register_set &source_reg) {
  // inst_fetch_buffer_entry **ready_reg =
  //     source_reg.get_ready(m_hw_cfg->get_sub_core_model(), m_issue_reg_id);
  // (*ready_reg)->op_pipe = MEM__OP;
  pipelined_simd_unit::issue(source_reg);
}

void mem_unit::issue(register_set &source_reg, unsigned reg_id) {
  pipelined_simd_unit::issue(source_reg, reg_id);
}

std::vector<unsigned> mem_unit::cycle(trace_parser* tracer, Scoreboard* m_scoreboard, app_config* appcfg,
                                      std::vector<std::pair<int, int>>* kernel_block_pair,
                                      std::vector<unsigned>* m_num_warps_per_sm,
                                      unsigned KERNEL_EVALUATION,
                                      unsigned num_scheds,
                                      RegisterBankAllocator* m_reg_bank_allocator,
                                      bool* flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle,
                                      std::map<std::tuple<unsigned, unsigned, unsigned>, 
                                        std::tuple<unsigned, unsigned, unsigned, unsigned, 
                                                   unsigned, unsigned>>* clk_record,
                                      unsigned m_cycle) {
  std::vector<unsigned> need_to_return_wids;
  bool active_during_this_cycle = false;
  
  if (m_pipeline_reg[0]->m_valid) {
    if (_CALIBRATION_LOG_) {
      std::cout << "    Execute: ("
                << m_pipeline_reg[0]->kid << ", "
                << m_pipeline_reg[0]->wid << ", "
                << m_pipeline_reg[0]->uid << ", "
                << m_pipeline_reg[0]->pc << ")" << std::endl;
    }
    set_clk_record<4>(clk_record, m_pipeline_reg[0]->kid, m_pipeline_reg[0]->wid, m_pipeline_reg[0]->uid, m_cycle);
    // m_pipeline_reg[0]->m_valid = false;
    // active_insts_in_pipeline--;

    auto _compute_instn = tracer->get_one_kernel_one_warp_one_instn(
      m_pipeline_reg[0]->kid, 
      m_pipeline_reg[0]->wid, 
      m_pipeline_reg[0]->uid);
    auto _trace_warp_inst = 
        _compute_instn->trace_warp_inst;
    std::vector<int> need_write_back_regs_num;

      unsigned _warps_per_block = appcfg->get_num_warp_per_block(m_pipeline_reg[0]->kid);
      unsigned dst_reg_num = _trace_warp_inst.get_outcount();
      for (unsigned i = 0; i < dst_reg_num; i++){
        int dst_reg_id = _trace_warp_inst.get_arch_reg_dst(i);
        // std::cout << "    dst_reg_id[" << i << "]: " << dst_reg_id << std::endl;
        // Calculate the scheduler id of the dst reg id
        if (dst_reg_id >= 0) {
          auto local_wid = (unsigned)(m_pipeline_reg[0]->wid % _warps_per_block);
          auto sched_id = (unsigned)(local_wid % num_scheds);
          // Calculate the bank id of the dst reg id
          auto bank_id = m_reg_bank_allocator->register_bank(dst_reg_id, local_wid, sched_id);
          
          // Now we set the bank_id of the dst reg id to be on_write (for dst reg id, it is on_wite).
          if (m_reg_bank_allocator->getBankState(bank_id) == FREE) {
            // active_during_this_cycle = true;

            m_reg_bank_allocator->setBankState(bank_id, ON_WRITING);
            
            _trace_warp_inst.set_arch_reg_dst(i, -1);
            if (_DEBUG_LOG_)
              std::cout << "    setBankState(" << bank_id 
                        << ", WRITING)   dst_reg_id : " 
                        << _trace_warp_inst.get_arch_reg_dst(i) 
                        << std::endl;
          } else {
            if (_DEBUG_LOG_)
              std::cout << "    cannot setBankState "
                           "bank_id-" << bank_id << std::endl;
            *flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle = true;
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

    if (all_write_back) {
      m_pipeline_reg[0]->m_valid = false;
      active_insts_in_pipeline--;

      _inst_trace_t* tmp_inst_trace = _compute_instn->inst_trace;
      for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
        need_write_back_regs_num.push_back(tmp_inst_trace->reg_src[i]);
        // std::cout << "        R" << tmp_inst_trace->reg_src[i] << std::endl;
      }
      for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
        if (tmp_inst_trace->reg_dest_is_pred[i]) {
          need_write_back_regs_num.push_back(tmp_inst_trace->reg_dest[i] + PRED_NUM_OFFSET);
        } else {
          need_write_back_regs_num.push_back(tmp_inst_trace->reg_dest[i]);
        }
      }
      auto pred = _trace_warp_inst.get_pred();
      need_write_back_regs_num.push_back((pred < 0) ? pred : pred + PRED_NUM_OFFSET);

      //m_num_warps_per_sm, KERNEL_EVALUATION, kernel_block_pair
      // unsigned KERNEL_EVALUATION = 0;
      unsigned _warps_per_block = appcfg->get_num_warp_per_block(m_pipeline_reg[0]->kid);

      unsigned _block_id = (unsigned)(m_pipeline_reg[0]->wid / _warps_per_block);
      unsigned _kid_block_id_count = 0;
        for (auto _it_kernel_block_pair = kernel_block_pair->begin(); 
                  _it_kernel_block_pair != kernel_block_pair->end();
                  _it_kernel_block_pair++) {
          if (_it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
          if (_it_kernel_block_pair->first - 1 == m_pipeline_reg[0]->kid) {
            if (_it_kernel_block_pair->second < _block_id) {
              _kid_block_id_count++;
            }
          }
        }
      // std::cout << " #_block_id: " << m_pipeline_reg[0]->kid << " " << _kid_block_id_count << std::endl;

      /*
      unsigned _kid_block_id_count = 0;
            for (auto _it_kernel_block_pair = kernel_block_pair.begin(); 
                      _it_kernel_block_pair != kernel_block_pair.end();
                      _it_kernel_block_pair++) {
              if (_it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
              if (_it_kernel_block_pair->first - 1 == _kid) {
                if (_it_kernel_block_pair->second < _block_id) {
                  _kid_block_id_count++;
                }
              }
            }
      
      auto global_all_kernels_warp_id = 
              (unsigned)(wid % _warps_per_block) + 
              _kid_block_id_count * _warps_per_block +
              std::accumulate(m_num_warps_per_sm.begin(), 
                              m_num_warps_per_sm.begin() + _kid, 0);
      */
      auto global_all_kernels_warp_id = 
          (unsigned)(m_pipeline_reg[0]->wid % _warps_per_block) + 
          _kid_block_id_count * _warps_per_block +
          std::accumulate(m_num_warps_per_sm->begin(), 
                          m_num_warps_per_sm->begin() + m_pipeline_reg[0]->kid, 0);
      
      // std::cout << "global_all_kernels_warp_id: " << m_pipeline_reg[0]->wid << " " << global_all_kernels_warp_id << std::endl;

      for (auto regnum : need_write_back_regs_num) {
        // void Scoreboard::releaseRegisters(const unsigned wid, std::vector<int> regnums)
        // m_scoreboard->releaseRegisters(_wid, regnum);
        // std::cout << "    release register: " << regnum << std::endl;
        // m_scoreboard->printContents();
        // std::cout << "  releaseRegisters: " << global_all_kernels_warp_id << " " << regnum << std::endl;
        m_scoreboard->releaseRegisters(global_all_kernels_warp_id, regnum);
        // m_scoreboard->printContents();
      }
    }
  }

  if (active_insts_in_pipeline) {
    active_during_this_cycle = true;
    for (unsigned stage = 0; stage < m_pipeline_depth - 1; stage++) {
      if (!m_pipeline_reg[stage]->m_valid) {
        m_pipeline_reg[stage]->m_valid = m_pipeline_reg[stage + 1]->m_valid;
        m_pipeline_reg[stage]->pc = m_pipeline_reg[stage + 1]->pc;
        m_pipeline_reg[stage]->wid = m_pipeline_reg[stage + 1]->wid;
        m_pipeline_reg[stage]->kid = m_pipeline_reg[stage + 1]->kid;
        m_pipeline_reg[stage]->uid = m_pipeline_reg[stage + 1]->uid;
        m_pipeline_reg[stage]->latency = m_pipeline_reg[stage + 1]->latency;
        m_pipeline_reg[stage]->initial_interval = m_pipeline_reg[stage + 1]->initial_interval;
        m_pipeline_reg[stage + 1]->m_valid = false;
      }
    }
  }

  if (m_dispatch_reg->m_valid) {
    // std::cout << "    m_dispatch_reg latency: " << m_dispatch_reg->latency 
    //           << " initial_interval: " << m_dispatch_reg->initial_interval 
    //           << " dec_counter: " << m_dispatch_reg->initial_interval_dec_counter << std::endl;
    if (m_dispatch_reg->initial_interval_dec_counter == 1) {
      int start_stage = m_dispatch_reg->latency - m_dispatch_reg->initial_interval /*+ 1*/;
      if (start_stage < 0) start_stage = 0;
      
      // std::cout << "    start_stage: " << start_stage 
      //           << " m_pipeline_depth: " << m_pipeline_depth 
      //           << " ini_intval counter: " 
      //           << m_dispatch_reg->initial_interval_dec_counter
      //           << " latency: " << m_dispatch_reg->latency
      //           << " initial_interval: " << m_dispatch_reg->initial_interval
      //           << std::endl;
      // std::cout << "start_stage: " << start_stage << " " << "m_pipeline_depth: " << m_pipeline_depth << std::endl;
      assert(start_stage >= 0 && start_stage < m_pipeline_depth);
      // m_pipeline_reg[start_stage]->move_in(m_dispatch_reg);
      if (m_pipeline_reg[start_stage]->m_valid == false) {
        active_during_this_cycle = true;

        m_dispatch_reg->m_valid = false;
        active_insts_in_pipeline++;
        m_pipeline_reg[start_stage]->m_valid = true;
        m_pipeline_reg[start_stage]->pc = m_dispatch_reg->pc;
        m_pipeline_reg[start_stage]->wid = m_dispatch_reg->wid;
        m_pipeline_reg[start_stage]->kid = m_dispatch_reg->kid;
        m_pipeline_reg[start_stage]->uid = m_dispatch_reg->uid;
        m_pipeline_reg[start_stage]->latency = m_dispatch_reg->latency;
        m_pipeline_reg[start_stage]->initial_interval = m_dispatch_reg->initial_interval;
      }
    } else {
      m_dispatch_reg->initial_interval_dec_counter--;
    }
  }

  occupied >>= 1;

  return need_to_return_wids;
}