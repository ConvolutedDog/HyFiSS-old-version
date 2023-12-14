#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include "inst-memadd-info.h"
#include "memory-space.h"
#include "sass-inst.h"
#include "../common/common_def.h"
#include "../ISA-Def/trace_opcode.h"
#include "../hw-parser/hw-parser.h"
#include "../ISA-Def/volta_opcode.h"

#ifndef INST_TRACE_H
#define INST_TRACE_H

enum FUNC_UNITS_NAME {
  NON_UNIT = 0,
  SP_UNIT,
  SFU_UNIT,
  INT_UNIT,
  DP_UNIT,
  TENSOR_CORE_UNIT,
  LDST_UNIT,
  SPEC_UNIT_1,
  SPEC_UNIT_2,
  SPEC_UNIT_3,
  NUM_FUNC_UNITS
};

struct inst_trace_t {
  inst_trace_t();
  inst_trace_t(const inst_trace_t &b);

  unsigned line_num;
  unsigned m_pc;
  unsigned mask;
  unsigned reg_dsts_num;
  unsigned reg_dest[MAX_DST];
  std::string opcode;
  unsigned reg_srcs_num;
  unsigned reg_src[MAX_SRC];
  inst_memadd_info_t *memadd_info;

  bool parse_from_string(std::string trace, unsigned tracer_version,
                         unsigned enable_lineinfo,
                         std::string kernel_name,
                         unsigned kernel_id);

  bool check_opcode_contain(const std::vector<std::string> &opcode,
                            std::string param) const;

  unsigned get_datawidth_from_opcode(
      const std::vector<std::string> &opcode) const;

  std::vector<std::string> get_opcode_tokens() const;

  ~inst_trace_t();
};


struct _inst_trace_t {
  _inst_trace_t();
  _inst_trace_t(const _inst_trace_t &b);
  _inst_trace_t(unsigned _kernel_id, unsigned _pc, std::string _instn_str) {
    kernel_id = _kernel_id;
    m_pc = _pc;
    instn_str = _instn_str;
    memadd_info = NULL;
    parse_from_string(_instn_str, _kernel_id);

    /* print out the instn_str */
    // std::cout << kernel_id << " " << m_pc << " " << instn_str << std::endl;
    // if (pred_str != "") std::cout << pred_str << " ";
    // std::cout << opcode << " " << reg_dsts_num << " ";
    // for (unsigned i = 0; i < reg_dsts_num; i++) {
    //   std::cout << "R" << reg_dest[i] << " ";
    // }
    // std::cout << reg_srcs_num << " ";
    // for (unsigned i = 0; i < reg_srcs_num; i++) {
    //   std::cout << "R" << reg_src[i] << " ";
    // }
    // std::cout << std::endl;

    opcode_tokens = get_opcode_tokens();
    memadd_info->width = get_datawidth_from_opcode(opcode_tokens);
  };

  _inst_trace_t(unsigned _kernel_id, unsigned _pc, std::string _instn_str, hw_config* hw_cfg) {
    kernel_id = _kernel_id;
    m_pc = _pc;
    instn_str = _instn_str;
    memadd_info = NULL;
    parse_from_string(_instn_str, _kernel_id);

    /* print out the instn_str */
    // std::cout << kernel_id << " " << m_pc << " " << instn_str << std::endl;
    // if (pred_str != "") std::cout << pred_str << " ";
    // std::cout << opcode << " " << reg_dsts_num << " ";
    // for (unsigned i = 0; i < reg_dsts_num; i++) {
    //   std::cout << "R" << reg_dest[i] << " ";
    // }
    // std::cout << reg_srcs_num << " ";
    // for (unsigned i = 0; i < reg_srcs_num; i++) {
    //   std::cout << "R" << reg_src[i] << " ";
    // }
    // std::cout << std::endl;

    opcode_tokens = get_opcode_tokens();
    memadd_info->width = get_datawidth_from_opcode(opcode_tokens);
    this->hw_cfg = hw_cfg;

    // std::cout << "``````|||" << this->hw_cfg->get_opcode_latency_initiation_int(0) << std::endl;
    // std::cout << "``````|||" << this->hw_cfg->get_opcode_latency_initiation_int(0) << std::endl;

    parse_opcode_latency_info();
  };

  // unsigned line_num;
  unsigned kernel_id;
  unsigned m_pc;
  unsigned mask; // invalid
  unsigned reg_dsts_num;
  int reg_dest[MAX_DST];
  std::string opcode;
  bool read_or_wirte;
  unsigned reg_srcs_num;
  int reg_src[MAX_SRC];
  inst_memadd_info_t *memadd_info;
  std::string instn_str;

  std::vector<std::string> opcode_tokens;

  std::string pred_str = "";

  unsigned initiation_interval;
  unsigned latency;
  enum FUNC_UNITS_NAME func_unit;
  hw_config* hw_cfg;

  bool parse_from_string(std::string trace,
                         unsigned kernel_id);

  bool check_opcode_contain(const std::vector<std::string> &opcode,
                            std::string param) const;

  unsigned get_datawidth_from_opcode(
      const std::vector<std::string> &opcode) const;

  std::vector<std::string> get_opcode_tokens() const;

  inline std::vector<std::string> get_opcode_tokens_directly() const {
    return opcode_tokens;
  }

  void parse_opcode_latency_info();

  unsigned get_latency() const;
  unsigned get_initiation_interval() const;
  enum FUNC_UNITS_NAME get_func_unit() const;

  ~_inst_trace_t();  
};

#endif