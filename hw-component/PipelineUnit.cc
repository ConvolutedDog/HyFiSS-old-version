
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
  return !m_dispatch_reg->m_valid && !occupied.test(latency);
}

void pipelined_simd_unit::cycle() {
  if (m_pipeline_reg[0]->m_valid) {
    m_result_port->move_in(m_pipeline_reg[0]);
    assert(active_insts_in_pipeline > 0);
    active_insts_in_pipeline--;
  }

  if (active_insts_in_pipeline) {
    // for (unsigned stage = 0; stage < m_pipeline_depth - 1; stage++) {
    //   m_pipeline_reg[stage]->move_in(m_pipeline_reg[stage + 1]);
    // }
    std::rotate(m_pipeline_reg.begin(), m_pipeline_reg.begin() + 1, m_pipeline_reg.end());
    m_pipeline_reg[m_pipeline_depth - 1]->m_valid = false;
  }

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

  if (m_dispatch_reg->m_valid) {
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

  occupied >>= 1;
}

void pipelined_simd_unit::issue(register_set &source_reg) {
  bool partition_issue =
    m_hw_cfg->get_sub_core_model() && this->is_issue_partitioned();
  inst_fetch_buffer_entry **ready_reg =
    source_reg.get_ready(partition_issue, m_issue_reg_id);
  
  std::cout << "    partition_issue: " << partition_issue << std::endl
            << "    m_issue_reg_id: " << m_issue_reg_id << std::endl
            << "    ready_reg: " << ready_reg << std::endl
            << "    NULL: " << NULL << std::endl;

  if (ready_reg != NULL) {
    std::cout << "    (*ready_reg): " << (*ready_reg)->kid 
              << " " << (*ready_reg)->wid 
              << " " << (*ready_reg)->uid
              << std::endl;
  
    // simd_function_unit::issue(source_reg);

    //source_reg即为流水线寄存器，目的是找到一个非空的指令，将其移入m_dispatch_reg。
    std::cout << "    source_reg.move_out_to.m_dispatch_reg" << std::endl;

    std::cout << "    Before Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
              << "                         kid: " << m_dispatch_reg->kid << std::endl
              << "                         wid: " << m_dispatch_reg->wid << std::endl 
              << "                         uid: " << m_dispatch_reg->uid << std::endl;
    std::cout << "    source before move: " << (*ready_reg)->m_valid << std::endl;

    source_reg.move_out_to(partition_issue, this->get_issue_reg_id(),
                          m_dispatch_reg);

    std::cout << "    After Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
              << "                        kid: " << m_dispatch_reg->kid << std::endl
              << "                        wid: " << m_dispatch_reg->wid << std::endl 
              << "                        uid: " << m_dispatch_reg->uid << std::endl;

    std::cout << "    source after move: " << (*ready_reg)->m_valid << std::endl;

    //设置m_dispatch_reg的标识占用位图的状态，m_dispatch_reg是warp_inst_t类型，可获取该指令的延迟。
    std::cout << "    occupied.set" << std::endl;
    
    occupied.set(m_dispatch_reg->latency);

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
  
  std::cout << "    partition_issue: " << partition_issue << std::endl
            << "    reg_id: " << reg_id << std::endl
            << "    ready_reg: " << ready_reg << std::endl
            << "    NULL: " << NULL << std::endl;

  if (ready_reg != NULL) {
    std::cout << "    (*ready_reg): " << (*ready_reg)->kid 
              << " " << (*ready_reg)->wid 
              << " " << (*ready_reg)->uid
              << std::endl;
  
    // simd_function_unit::issue(source_reg);

    //source_reg即为流水线寄存器，目的是找到一个非空的指令，将其移入m_dispatch_reg。
    std::cout << "    source_reg.move_out_to.m_dispatch_reg" << std::endl;

    std::cout << "    Before Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
              << "                         kid: " << m_dispatch_reg->kid << std::endl
              << "                         wid: " << m_dispatch_reg->wid << std::endl 
              << "                         uid: " << m_dispatch_reg->uid << std::endl;
    std::cout << "    source before move: " << (*ready_reg)->m_valid << std::endl;

    source_reg.move_out_to(partition_issue, reg_id,
                           m_dispatch_reg);

    std::cout << "    After Move: " << "valid: " << m_dispatch_reg->m_valid << std::endl
              << "                        kid: " << m_dispatch_reg->kid << std::endl
              << "                        wid: " << m_dispatch_reg->wid << std::endl 
              << "                        uid: " << m_dispatch_reg->uid << std::endl
              << "                    latency: " << m_dispatch_reg->latency << std::endl;

    std::cout << "    source after move: " << (*ready_reg)->m_valid << std::endl;

    //设置m_dispatch_reg的标识占用位图的状态，m_dispatch_reg是warp_inst_t类型，可获取该指令的延迟。
    std::cout << "    occupied.set" << std::endl;
    
    occupied.set(m_dispatch_reg->latency);

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