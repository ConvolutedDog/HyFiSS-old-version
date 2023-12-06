#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include "inst-memadd-info.h"
#include "memory-space.h"
#include "sass-inst.h"
#include "../common/common_def.h"
#include "../ISA-Def/trace_opcode.h"

#ifndef INST_TRACE_H
#define INST_TRACE_H

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

  // unsigned line_num;
  unsigned kernel_id;
  unsigned m_pc;
  unsigned mask;
  unsigned reg_dsts_num;
  unsigned reg_dest[MAX_DST];
  std::string opcode;
  unsigned reg_srcs_num;
  unsigned reg_src[MAX_SRC];
  inst_memadd_info_t *memadd_info;
  std::string instn_str;

  std::vector<std::string> opcode_tokens;

  std::string pred_str = "";

  bool parse_from_string(std::string trace,
                         unsigned kernel_id);

  bool check_opcode_contain(const std::vector<std::string> &opcode,
                            std::string param) const;

  unsigned get_datawidth_from_opcode(
      const std::vector<std::string> &opcode) const;

  std::vector<std::string> get_opcode_tokens() const;

  std::vector<std::string> get_opcode_tokens_directly() const;

  ~_inst_trace_t();  
};

#endif