
#include <vector>
#include <utility>
#include "../trace-parser/trace-parser.h"

#include "IBuffer.h"

#include "../hw-parser/hw-parser.h"
#include "RegisterBankAllocator.h"

#ifndef PRIVATESM_H
#define PRIVATESM_H

struct stage_instns_identifier {
  stage_instns_identifier(unsigned _kid, unsigned _pc, 
                          unsigned _wid, unsigned _uid) {
    kid = _kid;
    pc = _pc;
    wid = _wid;
    uid = _uid;
  };
  unsigned kid;
  unsigned pc;
  unsigned wid;
  unsigned uid;
};


struct inst_fetch_buffer_entry {
  inst_fetch_buffer_entry() {
    m_valid = false;
  };
  inst_fetch_buffer_entry(unsigned _pc, unsigned _wid, unsigned _kid, unsigned _uid) {
    pc = _pc;
    wid = _wid;
    kid = _kid;
    uid = _uid;
    m_valid = true;
  };
  unsigned pc;
  unsigned wid;
  unsigned kid;
  unsigned uid;
  bool m_valid;
};


class PrivateSM {
 public:
  PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg);

  void run();

  bool get_active() { return active; }
  unsigned long long get_cycle() { return m_cycle; }

  bool is_active() { return active; }
  bool check_active();

  unsigned get_num_warps_per_sm(unsigned kernel_id) { return m_num_warps_per_sm[kernel_id]; }

  /*
  在V100配置中，m_num_banks被初始化为16。m_bank_warp_shift被初始化为5。由于在操作数
  收集器的寄存器文件中，warp0的r0寄存器放在0号bank，...，warp0的r15寄存器放在15号bank，
  warp0的r16寄存器放在0号bank，...，warp0的r31寄存器放在15号bank；warp1的r0寄存器放在
  [0+warp_id]号bank，这里以非sub_core_model模式为例：

  这里register_bank函数就是用来计算regnum所在的bank数。

  Bank0   Bank1   Bank2   Bank3                   ......                  Bank15
  w1:r31  w1:r16  w1:r17  w1:r18                  ......                  w1:r30
  w1:r15  w1:r0   w1:r1   w1:r2                   ......                  w1:r14
  w0:r16  w0:r17  w0:r18  w0:r19                  ......                  w0:r31
  w0:r0   w0:r1   w0:r2   w0:r3                   ......                  w0:r15

  在sub_core_model模式中，每个warp调度器可用的bank数量是有限的。在V100配置中，共有4个
  warp调度器，0号warp调度器可用的bank为0-3，1号warp调度器可用的bank为4-7，2号warp调度
  器可用的bank为8-11，3号warp调度器可用的bank为12-15。
  */
  int register_bank(int regnum, int wid, unsigned sched_id);

  void parse_blocks_per_kernel() {
    for (unsigned i = 0; i < kernel_block_pair.size(); i++) {
      if (blocks_per_kernel.find(kernel_block_pair[i].first) == blocks_per_kernel.end()) {
        blocks_per_kernel[kernel_block_pair[i].first] = std::vector<unsigned>(1, kernel_block_pair[i].second);
      } else {
        blocks_per_kernel[kernel_block_pair[i].first].push_back(kernel_block_pair[i].second);
      }
    }
  }
  
  std::vector<unsigned> get_blocks_per_kernel(unsigned kernel_id) {
    return blocks_per_kernel[kernel_id];
  }

  std::map<unsigned, std::vector<unsigned>>* get_blocks_per_kernel() {
    return &blocks_per_kernel;
  }

  unsigned get_inst_fetch_throughput() { return inst_fetch_throughput; }

 private:
  unsigned m_smid;
  unsigned long long m_cycle;
  bool active;

  unsigned num_banks;
  unsigned bank_warp_shift;
  unsigned num_scheds;
  bool sub_core_model;
  unsigned banks_per_sched;
  unsigned inst_fetch_throughput;

  // number of warps per kernel that are allocated to this SM
  std::vector<unsigned> m_num_warps_per_sm;

  // sum of std::vector<unsigned> m_num_warps_per_sm
  unsigned all_warps_num;

  // (kernel_id, block_id) pair that are allocated to this SM
  std::vector<std::pair<int, int>> kernel_block_pair;
  std::map<unsigned, std::vector<unsigned>> blocks_per_kernel;

  trace_parser* tracer;
  issue_config* issuecfg;
  app_config* appcfg;
  instn_config* instncfg;

  IBuffer* m_ibuffer;
  inst_fetch_buffer_entry* m_inst_fetch_buffer;

  unsigned last_fetch_warp_id;
  RegisterBankAllocator* m_reg_bank_allocator;

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  std::vector<unsigned> curr_instn_id_per_warp;

  std::vector<stage_instns_identifier> fetch_stage_instns;

  std::vector<stage_instns_identifier> writeback_stage_instns;

  std::vector<stage_instns_identifier> warp_exit_stage_instns;

};

#endif