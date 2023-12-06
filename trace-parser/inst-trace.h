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
 public:
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

  ~_inst_trace_t();
  /*********************************************************************/ // TODO
 private:
  unsigned m_opcode;
  unsigned m_uid;
  bool m_empty;
  bool m_isatomic;

  bool m_decoded = false;
  address_type pc = (address_type)-1;
  unsigned isize;   // size of instruction in bytes

  //记录了当前指令的所有目的操作数寄存器ID。
  unsigned out[8];
  //记录了当前指令的所有目的操作数寄存器总数。
  unsigned outcount;
  //记录了当前指令的所有源操作数寄存器ID。
  unsigned in[24];
  //记录了当前指令的所有源操作数寄存器总数。
  unsigned incount;

  unsigned char is_vectorin;
  unsigned char is_vectorout;

  int pred;  // predicate register number
  int ar1, ar2;
  // register number for bank conflict evaluation
  struct {
    int dst[MAX_REG_OPERANDS];
    int src[MAX_REG_OPERANDS];
  } arch_reg;

  _memory_op_t memory_op;      // memory_op used by ptxplus

  unsigned num_operands;
  unsigned num_regs;  // count vector operand as one register operand

  unsigned data_size;  // what is the size of the word being operated on?

  op_type op;       // opcode (uarch visible)
  special_ops
      sp_op;  // code (uarch visible) identify if int_alu, fp_alu, int_mul ....
  mem_operation mem_op;        // code (uarch visible) identify memory type

  bool const_cache_operand;   // has a load from constant memory as an operand

  types_of_operands oprnd_type;  // code (uarch visible) identify if the
                                 // operation is an interger or a floating point

  bool should_do_atomic;
  bool m_is_printf;
  unsigned m_warp_id;
  unsigned m_dynamic_warp_id;
  // const core_config *m_config;
  active_mask_t m_warp_active_mask;  // dynamic active mask for timing model
                                     // (after predication)
  memory_space_t space;
  cache_operator_type cache_op;
  /*********************************************************************/
  
};

#endif