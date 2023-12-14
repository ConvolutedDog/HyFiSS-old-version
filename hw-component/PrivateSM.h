
#include <vector>
#include <utility>
#include <map>
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

struct curr_instn_id_per_warp_entry {
  curr_instn_id_per_warp_entry() {
    kid = 0;
    block_id = 0;
    warp_id = 0;
  };
  curr_instn_id_per_warp_entry(unsigned _kid, unsigned _block_id, unsigned _warp_id) {
    kid = _kid;
    block_id = _block_id;
    warp_id = _warp_id;
  };
  unsigned kid;
  unsigned block_id;
  unsigned warp_id;
};

bool operator<(const curr_instn_id_per_warp_entry& lhs, const curr_instn_id_per_warp_entry& rhs);


// enum pipeline_stage_name_t {
//   ID_OC_SP = 0,
//   ID_OC_DP = 1,
//   ID_OC_INT = 2,
//   ID_OC_SFU = 3,
//   ID_OC_MEM = 4,
//   OC_EX_SP = 5,
//   OC_EX_DP = 6,
//   OC_EX_INT = 7,
//   OC_EX_SFU = 8,
//   OC_EX_MEM = 9,
//   EX_WB = 10,
//   ID_OC_TENSOR_CORE = 11,
//   OC_EX_TENSOR_CORE = 12,
//   N_PIPELINE_STAGES
// };

// const char *const pipeline_stage_name_decode[] = {
//     "ID_OC_SP",          "ID_OC_DP",         "ID_OC_INT", "ID_OC_SFU",
//     "ID_OC_MEM",         "OC_EX_SP",         "OC_EX_DP",  "OC_EX_INT",
//     "OC_EX_SFU",         "OC_EX_MEM",        "EX_WB",     "ID_OC_TENSOR_CORE",
//     "OC_EX_TENSOR_CORE", "N_PIPELINE_STAGES"};

class register_set {
 public:
  //构造函数，用于初始化寄存器集合，寄存器集合中有num个寄存器，每个寄存器含有一条指令。
  register_set(){};
  register_set(unsigned num, std::string name, hw_config* hw_cfg) {
    for (unsigned i = 0; i < num; i++) {
      regs.push_back(new inst_fetch_buffer_entry());
    }
    //m_name是该寄存器集合的名字。
    m_name = name;
    m_hw_cfg = hw_cfg;
  }
  //获取该寄存器集合的名字。
  const std::string get_name() { return m_name; }
  //遍历寄存器集合中的所有寄存器，判断是否有寄存器为空。
  bool has_free() {
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid == false) {
        return true;
      }
    }
    return false;
  }
  //给定一个寄存器id，判断该寄存器是否为空。
  bool has_free(bool sub_core_model, unsigned reg_id) {
    // in subcore model, each sched has a one specific reg to use (based on
    // sched id)
    if (!sub_core_model) return has_free();

    assert(reg_id < regs.size());
    return (regs[reg_id]->m_valid == false);
  }
  //获取一个非空寄存器的id。
  bool has_ready() {
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        return true;
      }
    }
    return false;
  }
  //给定一个寄存器id，判断该寄存器是否非空。
  bool has_ready(bool sub_core_model, unsigned reg_id) {
    if (!sub_core_model) return has_ready();
    assert(reg_id < regs.size());
    return (regs[reg_id]->m_valid);
  }
  //获取一个非空寄存器的id。
  unsigned get_ready_reg_id() {
    // for sub core model we need to figure which reg_id has the ready warp
    // this function should only be called if has_ready() was true
    assert(has_ready());
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    unsigned reg_id;
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        if (ready and (*ready)->uid < regs[i]->uid) {
          // ready is oldest
        } else {
          ready = &regs[i];
          reg_id = i;
        }
      }
    }
    return reg_id;
  }
  unsigned get_schd_id(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return (unsigned)(regs[reg_id]->wid % m_hw_cfg->get_num_sched_per_sm());
  }

  void move_warp(inst_fetch_buffer_entry *&dest, 
                 inst_fetch_buffer_entry *&src) {
    dest->pc = src->pc;
    dest->wid = src->wid;
    dest->kid = src->kid;
    dest->uid = src->uid;
    dest->m_valid = true;
    // src->clear();
  }

  //获取一个非空寄存器，并将一条指令存入。
  void move_in(inst_fetch_buffer_entry *&src) {
    inst_fetch_buffer_entry **free = get_free();
    inst_fetch_buffer_entry* tmp = *free;
    move_warp(tmp, src);
  }
  //获取一个空寄存器，并将一条指令存入。
  void move_in(bool sub_core_model, unsigned reg_id, inst_fetch_buffer_entry *&src) {
    inst_fetch_buffer_entry **free;
    if (!sub_core_model) {
      free = get_free();
    } else {
      assert(reg_id < regs.size());
      free = get_free(sub_core_model, reg_id);
    }
    inst_fetch_buffer_entry* tmp = *free;
    move_warp(tmp, src);
  }
  //获取一个非空寄存器，并将其指令移出到dest。
  void move_out_to(inst_fetch_buffer_entry *&dest) {
    inst_fetch_buffer_entry **ready = get_ready();
    move_warp(dest, *ready);
  }
  //依据寄存器编号reg_id，获取一个非空寄存器，并将其指令移出到dest。
  void move_out_to(bool sub_core_model, unsigned reg_id, inst_fetch_buffer_entry *&dest) {
    if (!sub_core_model) {
      return move_out_to(dest);
    }
    inst_fetch_buffer_entry **ready = get_ready(sub_core_model, reg_id);
    assert(ready != NULL);
    move_warp(dest, *ready);
  }
  //获取一个非空寄存器，将其指令移出，并返回这条指令。
  inst_fetch_buffer_entry **get_ready() {
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        if (ready and (*ready)->uid < regs[i]->uid) {
          // ready is oldest
        } else {
          ready = &regs[i];
        }
      }
    }
    return ready;
  }
  //获取一个非空寄存器，将其指令移出，并返回这条指令。
  inst_fetch_buffer_entry **get_ready(bool sub_core_model, unsigned reg_id) {
    if (!sub_core_model) return get_ready();
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    assert(reg_id < regs.size());
    if (regs[reg_id]->m_valid) ready = &regs[reg_id];
    return ready;
  }
  //打印寄存器集合中的所有寄存器。
  void print() const {
    std::cout << m_name << " : @ " << this << std::endl;
    for (unsigned i = 0; i < regs.size(); i++) {
      std::cout << "     ";
      if (regs[i]->m_valid) {
        std::cout << "    valid: ";
        std::cout << "pc: " << regs[i]->pc << ", wid: " << regs[i]->wid 
                  << ", kid: " << regs[i]->kid << ", uid: " << regs[i]->uid;
      } else {
        std::cout << "    novalid      ";
      }
      std::cout << std::endl;
    }
  }
  //遍历所有寄存器，获取一个空寄存器的id。
  inst_fetch_buffer_entry **get_free() {
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid == false) {
        return &regs[i];
      }
    }
    return NULL;
  }
  //遍历所有寄存器，获取一个空寄存器的地址。
  inst_fetch_buffer_entry **get_free(bool sub_core_model, unsigned reg_id) {
    // in subcore model, each sched has a one specific reg to use (based on
    // sched id)
    if (!sub_core_model) return get_free();

    assert(reg_id < regs.size());
    if (regs[reg_id]->m_valid == false) {
      return &regs[reg_id];
    }
    return NULL;
  }
  //返回寄存器集合的大小。
  unsigned get_size() { return regs.size(); }

 private:
  //将寄存器集合中的所有寄存器用一个向量保存。
  std::vector<inst_fetch_buffer_entry*> regs;
  //该寄存器集合的名字。
  std::string m_name;
  hw_config* m_hw_cfg;
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

  unsigned warps_per_sched;

  // (kernel_id, block_id) pair that are allocated to this SM
  std::vector<std::pair<int, int>> kernel_block_pair;
  std::map<unsigned, std::vector<unsigned>> blocks_per_kernel;

  trace_parser* tracer;
  issue_config* issuecfg;
  app_config* appcfg;
  instn_config* instncfg;

  IBuffer* m_ibuffer;
  inst_fetch_buffer_entry* m_inst_fetch_buffer;

  unsigned total_pipeline_stages;
  std::vector<register_set> m_pipeline_reg;
  std::vector<register_set*> m_specilized_dispatch_reg;

  int last_fetch_warp_id;
  int last_issue_sched_id;
  std::vector<int> last_issue_warp_ids;
  RegisterBankAllocator* m_reg_bank_allocator;

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  std::map<curr_instn_id_per_warp_entry, unsigned> curr_instn_id_per_warp;

  std::vector<stage_instns_identifier> fetch_stage_instns;

  std::vector<stage_instns_identifier> writeback_stage_instns;

  std::vector<stage_instns_identifier> warp_exit_stage_instns;

};

#endif