
#include <list>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <string.h>
#include <regex>

#include "../common/vector_types.h"
#include "../common/common_def.h"
#include "../ISA-Def/trace_opcode.h"
#include "../trace-parser/inst-trace.h"
#include "../ISA-Def/accelwattch_component_mapping.h"

#include "mem-access.h"
#include "kernel-trace.h"

#ifndef TRACE_WARP_INST_H
#define TRACE_WARP_INST_H

types_of_operands get_oprnd_type(op_type op, special_ops sp_op);

class trace_warp_inst_t {
 public:
  trace_warp_inst_t() {
    m_opcode = 0;
    m_uid = 0;
    m_empty = true;
    m_isatomic = false;

    m_decoded = false;
    pc = (address_type)-1;
    isize = 0;

    num_operands = 0;
    num_regs = 0;

    memset(out, 0, sizeof(unsigned));
    outcount = 0;
    memset(in, 0, sizeof(unsigned));
    incount = 0;

    is_vectorin = false;
    is_vectorout = false;

    pred = -1;
    ar1 = -1;
    ar2 = -1;

    for (unsigned i = 0; i < MAX_REG_OPERANDS; i++) {
      arch_reg.src[i] = -1;
      arch_reg.dst[i] = -1;
    }

    memory_op = no_memory_op;
    data_size = 0;

    op = NO_OP;
    sp_op = OTHER_OP;
    mem_op = NOT_TEX;

    const_cache_operand = 0;

    oprnd_type = UN_OP;

    // m_per_scalar_thread_valid = false;
    // m_mem_accesses_created = false;
    m_is_printf = false;
    should_do_atomic = false;

    m_gwarp_id = 0;
    m_warp_id = 0;
    m_dynamic_warp_id = 0;

    space = memory_space_t();
    cache_op = CACHE_UNDEFINED;

    // active_mask_t m_warp_active_mask(0);
  }

  // trace_warp_inst_t(const class core_config *config) : warp_inst_t(config) {
  //   m_opcode = 0;
  //   should_do_atomic = false;
  // }

  // bool parse_from_trace_struct(
  //     const inst_trace_t &trace,
  //     const std::unordered_map<std::string, OpcodeChar> *OpcodeMap,
  //   //   const class trace_config *tconfig,
  //     const class kernel_trace_t *kernel_trace_info);
  
  // bool parse_from_trace_struct(
  //     const _inst_trace_t* trace,
  //     const std::unordered_map<std::string, OpcodeChar> *OpcodeMap);
  
  bool parse_from_trace_struct(
      const _inst_trace_t* trace,
      const std::unordered_map<std::string, OpcodeChar> *OpcodeMap,
      unsigned gwarp_id);

  inline void set_active(const active_mask_t &active);

  // void set_addr(unsigned n, new_addr_type addr) {
  //   if (!m_per_scalar_thread_valid) {
  //     m_per_scalar_thread.resize(MAX_WARP_SIZE);
  //     m_per_scalar_thread_valid = true;
  //   }
  //   m_per_scalar_thread[n].memreqaddr[0] = addr;
  // }

  // void set_addr(unsigned n, new_addr_type *addr, unsigned num_addrs) {
  //   if (!m_per_scalar_thread_valid) {
  //     m_per_scalar_thread.resize(MAX_WARP_SIZE);
  //     m_per_scalar_thread_valid = true;
  //   }
  //   assert(num_addrs <= MAX_ACCESSES_PER_INSN_PER_THREAD);
  //   for (unsigned i = 0; i < num_addrs; i++)
  //     m_per_scalar_thread[n].memreqaddr[i] = addr[i];
  // }

  /***************************************************************************/
  unsigned get_opcode() const { return m_opcode; }
  unsigned get_uid() const { return m_uid; }
  bool isempty() const { return m_empty; }
  bool isatomic() const { return m_isatomic; }
  bool isdecoded() const { return m_decoded; }
  address_type get_pc() const { return pc; }
  unsigned get_isize() const { return isize; }
  unsigned get_outcount() const { return outcount; }
  unsigned get_incount() const { return incount; }
  unsigned get_in(unsigned i) const {
    assert(i < incount);
    return in[i];
  }
  unsigned get_out(unsigned i) const {
    assert(i < outcount);
    return out[i];
  }
  bool get_is_vectorin() const { return is_vectorin; }
  bool get_is_vectorout() const { return is_vectorout; }
  int get_pred() const { return pred; }
  int get_ar1() const { return ar1; }
  int get_ar2() const { return ar2; }
  int get_arch_reg_dst(unsigned i) const {
    assert(i < outcount);
    return arch_reg.dst[i];
  }
  int get_arch_reg_src(unsigned i) const {
    assert(i < incount);
    return arch_reg.src[i];
  }
  void set_arch_reg_dst(unsigned i, int reg) {
    assert(i < outcount);
    arch_reg.dst[i] = reg;
  }
  void set_arch_reg_src(unsigned i, int reg) {
    assert(i < incount);
    arch_reg.src[i] = reg;
  }
  _memory_op_t get_memory_op() const { return memory_op; }
  unsigned get_num_operands() const { return num_operands; }
  unsigned get_num_regs() const { return num_regs; }
  unsigned get_data_size() const { return data_size; }
  op_type get_op() const { return op; }
  special_ops get_sp_op() const { return sp_op; }
  mem_operation get_mem_op() const { return mem_op; }
  bool get_const_cache_operand() const { return const_cache_operand; }
  types_of_operands get_oprnd_type_() const { return oprnd_type; }
  bool get_should_do_atomic() const { return should_do_atomic; }
  bool get_is_printf() const { return m_is_printf; }
  unsigned get_gwarp_id() const { return m_gwarp_id; }
  unsigned get_warp_id() const { return m_warp_id; }
  unsigned get_dynamic_warp_id() const { return m_dynamic_warp_id; }
  active_mask_t get_active_mask() const { return m_warp_active_mask; }
  active_mask_t& get_active_mask_ref() { return m_warp_active_mask; }
  unsigned get_activate_count() const { return m_warp_active_mask.count(); }
  /***************************************************************************/

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

  bool is_vectorin;
  bool is_vectorout;

  // TODO
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
  // TODO
  unsigned m_gwarp_id;
  unsigned m_warp_id;
  // TODO
  unsigned m_dynamic_warp_id;
  // const core_config *m_config;
  active_mask_t m_warp_active_mask;  // dynamic active mask for timing model
                                     // (after predication)
  
  // struct per_thread_info {
  //   per_thread_info() {
  //     for (unsigned i = 0; i < MAX_ACCESSES_PER_INSN_PER_THREAD; i++)
  //       memreqaddr[i] = 0;
  //   }
  //   //MAX_ACCESSES_PER_INSN_PER_THREAD为单个线程中允许的最大访存次数。设置为8。
  //   //memreqaddr[]存储了单条指令的所有访存地址。
  //   new_addr_type
  //       memreqaddr[MAX_ACCESSES_PER_INSN_PER_THREAD];  // effective address,
  //                                                      // upto 8 different
  //                                                      // requests (to support
  //                                                      // 32B access in 8 chunks
  //                                                      // of 4B each)
  // };
  // bool m_per_scalar_thread_valid;
  //m_per_scalar_thread是线程信息的向量，每个warp有一个m_per_scalar_thread。
  // std::vector<per_thread_info> m_per_scalar_thread;
  // bool m_mem_accesses_created;
  //当前指令的访存操作的列表。
  // std::list<mem_access_t> m_accessq;

  memory_space_t space;
  cache_operator_type cache_op;
};

#endif