
#include <vector>
#include <utility>
#include <map>
#include "../trace-parser/trace-parser.h"
#include <memory>

#include "IBuffer.h"

#include "../hw-parser/hw-parser.h"
#include "RegisterBankAllocator.h"
#include "../trace-driven/entry.h"
#include "../trace-driven/register-set.h"

#ifndef PRIVATESM_H
#define PRIVATESM_H

enum exec_unit_type_t {
  NONE = 0,
  SP = 1,
  SFU = 2,
  LDST = 3,
  DP = 4,
  INT = 5,
  TENSOR = 6,
  SPECIALIZED = 7
};

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

class PrivateSM {
 public:
  PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg);
  ~PrivateSM() {
    delete m_ibuffer;
    delete m_inst_fetch_buffer;
    delete m_reg_bank_allocator;
    /*
    register_set* m_sp_out;// = &m_pipeline_reg[ID_OC_SP];
    register_set* m_dp_out;// = &m_pipeline_reg[ID_OC_DP];
    register_set* m_sfu_out;// = &m_pipeline_reg[ID_OC_SFU];
    register_set* m_int_out;// = &m_pipeline_reg[ID_OC_INT];
    register_set* m_tensor_core_out;// = &m_pipeline_reg[ID_OC_TENSOR_CORE];
    std::vector<register_set*> m_spec_cores_out;// = m_specilized_dispatch_reg;
    register_set* m_mem_out;// = &m_pipeline_reg[ID_OC_MEM];
    */

    m_sp_out->release_register_set();
    m_dp_out->release_register_set();
    m_sfu_out->release_register_set();
    m_int_out->release_register_set();
    m_tensor_core_out->release_register_set();
    m_mem_out->release_register_set();
    for (auto ptr : m_specilized_dispatch_reg) {
      ptr->release_register_set();
    }
    m_specilized_dispatch_reg.clear();
    m_spec_cores_out.clear();
  
    for (auto ptr : m_pipeline_reg) {
      ptr.release_register_set();
    }
    
  };
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

  void issue_warp(register_set &pipe_reg_set, ibuffer_entry entry, unsigned sch_id) {
    // print entry
    std::cout << "issue_warp: " << std::endl;
    std::cout << "    pc: " << entry.pc << ", wid: " << entry.wid 
              << ", kid: " << entry.kid << ", uid: " << entry.uid << std::endl;

    // inst_fetch_buffer_entry **pipe_reg =
    //   pipe_reg_set.get_free(m_hw_cfg->get_sub_core_model(), sch_id);
    // assert(pipe_reg);

    inst_fetch_buffer_entry* tmp = new inst_fetch_buffer_entry();
    tmp->kid = entry.kid;
    tmp->pc = entry.pc;
    tmp->uid = entry.uid;
    tmp->wid = entry.wid;
    tmp->m_valid = true;

    std::cout << "issue_warp: " << std::endl;
    std::cout << "    pc: " << tmp->pc << ", wid: " << tmp->wid 
              << ", kid: " << tmp->kid << ", uid: " << tmp->uid << std::endl;

    pipe_reg_set.move_in(m_hw_cfg->get_sub_core_model(), sch_id, tmp);

    delete tmp;
    tmp = nullptr;  // Optional: set tmp to nullptr to avoid dangling pointer

    // print reigster set
    std::cout << "Now register set: " << std::endl;
    pipe_reg_set.print();

    // Scoreboard: TODO

  }

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
  register_set* m_sp_out;// = &m_pipeline_reg[ID_OC_SP];
  register_set* m_dp_out;// = &m_pipeline_reg[ID_OC_DP];
  register_set* m_sfu_out;// = &m_pipeline_reg[ID_OC_SFU];
  register_set* m_int_out;// = &m_pipeline_reg[ID_OC_INT];
  register_set* m_tensor_core_out;// = &m_pipeline_reg[ID_OC_TENSOR_CORE];
  std::vector<register_set*> m_spec_cores_out;// = m_specilized_dispatch_reg;
  register_set* m_mem_out;// = &m_pipeline_reg[ID_OC_MEM];

  int last_fetch_warp_id;
  int last_issue_sched_id;
  std::vector<int> last_issue_warp_ids;
  RegisterBankAllocator* m_reg_bank_allocator;

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  std::map<curr_instn_id_per_warp_entry, unsigned> curr_instn_id_per_warp;

  std::vector<stage_instns_identifier> fetch_stage_instns;

  std::vector<stage_instns_identifier> writeback_stage_instns;

  std::vector<stage_instns_identifier> warp_exit_stage_instns;

  hw_config* m_hw_cfg;
};

#endif