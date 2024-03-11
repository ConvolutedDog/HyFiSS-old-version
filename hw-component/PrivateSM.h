
#include <vector>
#include <utility>
#include <map>
#include "../trace-parser/trace-parser.h"
#include <memory>

#include "IBuffer.h"

#include "../hw-parser/hw-parser.h"
#include "RegisterBankAllocator.h"
#include "Scoreboard.h"
#include "../trace-driven/entry.h"
#include "../trace-driven/register-set.h"

#include "OperandCollector.h"

#include "PipelineUnit.h"

#ifndef PRIVATESM_H
#define PRIVATESM_H

#define PRED_NUM_OFFSET 65536
#define MAX_ALU_LATENCY 512

// #define KERNEL_EVALUATION 0


#define _EXECUTE_DEBUG_LOG_ 0

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

#include <fstream>
#include <string>



class stat_collector {
 public:
  stat_collector(unsigned num_sm, unsigned kernel_id);
  void set_Unified_L1_cache_hit_rate(float value, unsigned smid) {
    Unified_L1_cache_hit_rate[smid] = value;
  }
  float get_Unified_L1_cache_hit_rate(unsigned smid) {
    return Unified_L1_cache_hit_rate[smid];
  }
  void set_L2_cache_hit_rate(float value) {
    L2_cache_hit_rate = value;
  }
  void set_L2_cache_requests(unsigned value) {
    L2_cache_requests = value;
  }

  void set_Unified_L1_cache_requests(unsigned value, unsigned smid) {
    Unified_L1_cache_requests[smid] = value;
  }

  void print_Unified_L1_cache_hit_rate() {
    for (unsigned i = 0; i < Unified_L1_cache_hit_rate.size(); i++) {
      std::cout << "SM[" << i << "] = " << Unified_L1_cache_hit_rate[i] << std::endl;
    }
  }
  /*
  unsigned m_num_sm; // TODO: SM70
  unsigned max_block_size; // TODO: SM70
  unsigned warp_size; // TODO: SM70
  unsigned smem_allocation_size; // TODO: SM70
  unsigned max_registers_per_SM; // TODO: SM70
  unsigned max_registers_per_block; // TODO: SM70
  unsigned max_registers_per_thread; // TODO: SM70
  unsigned register_allocation_size; // TODO: SM70
  unsigned max_active_blocks_per_SM; // TODO: SM70
  unsigned max_active_threads_per_SM; // TODO: SM70
  shared_mem_size = 96*1024; // TODO: V100
  num_warp_schedulers_per_SM = 4; // TODO: V100

  total_num_workloads
  active_SMs
  allocated_active_warps_per_block
  */
  unsigned get_max_block_size() { return max_block_size; }
  unsigned get_warp_size() { return warp_size; }
  unsigned get_smem_allocation_size() { return smem_allocation_size; }
  unsigned get_max_registers_per_SM() { return max_registers_per_SM; }
  unsigned get_max_registers_per_block() { return max_registers_per_block; }
  unsigned get_max_registers_per_thread() { return max_registers_per_thread; }
  unsigned get_register_allocation_size() { return register_allocation_size; }
  unsigned get_max_active_blocks_per_SM() { return max_active_blocks_per_SM; }
  unsigned get_max_active_threads_per_SM() { return max_active_threads_per_SM; }
  unsigned get_shared_mem_size() { return shared_mem_size; }
  unsigned get_total_num_workloads() { return total_num_workloads; }
  unsigned get_active_SMs() { return active_SMs; }
  unsigned get_m_num_sm() { return m_num_sm; }
  unsigned get_allocated_active_warps_per_block() { return allocated_active_warps_per_block; }

  void set_max_block_size(unsigned value) { max_block_size = value; }
  void set_warp_size(unsigned value) { warp_size = value; }
  void set_smem_allocation_size(unsigned value) { smem_allocation_size = value; }
  void set_max_registers_per_SM(unsigned value) { max_registers_per_SM = value; }
  void set_max_registers_per_block(unsigned value) { max_registers_per_block = value; }
  void set_max_registers_per_thread(unsigned value) { max_registers_per_thread = value; }
  void set_register_allocation_size(unsigned value) { register_allocation_size = value; }
  void set_max_active_blocks_per_SM(unsigned value) { max_active_blocks_per_SM = value; }
  void set_max_active_threads_per_SM(unsigned value) { max_active_threads_per_SM = value; }
  void set_shared_mem_size(unsigned value) { shared_mem_size = value; }
  void set_total_num_workloads(unsigned value) { total_num_workloads = value; }
  void set_active_SMs(unsigned value) { active_SMs = value; }
  void set_m_num_sm(unsigned value) { m_num_sm = value; }
  void set_allocated_active_warps_per_block(unsigned value) { allocated_active_warps_per_block = value; }

  unsigned get_Thread_block_limit_SM() { return Thread_block_limit_SM; }
  unsigned get_Thread_block_limit_registers() { return Thread_block_limit_registers; }
  unsigned get_Thread_block_limit_shared_memory() { return Thread_block_limit_shared_memory; }
  unsigned get_Thread_block_limit_warps() { return Thread_block_limit_warps; }
  unsigned get_Theoretical_max_active_warps_per_SM() { return Theoretical_max_active_warps_per_SM; }
  float get_Theoretical_occupancy() { return Theoretical_occupancy; }

  void set_Thread_block_limit_SM(unsigned value) { Thread_block_limit_SM = value; }
  void set_Thread_block_limit_registers(unsigned value) { Thread_block_limit_registers = value; }
  void set_Thread_block_limit_shared_memory(unsigned value) { Thread_block_limit_shared_memory = value; }
  void set_Thread_block_limit_warps(unsigned value) { Thread_block_limit_warps = value; }
  void set_Theoretical_max_active_warps_per_SM(unsigned value) { Theoretical_max_active_warps_per_SM = value; }
  void set_Theoretical_occupancy(float value) { Theoretical_occupancy = value; }

  unsigned get_allocated_active_blocks_per_SM() { return allocated_active_blocks_per_SM; }
  unsigned set_allocated_active_blocks_per_SM(unsigned value) { allocated_active_blocks_per_SM = value; }

  /*
  std::vector<unsigned> GMEM_read_requests;
  std::vector<unsigned> GMEM_write_requests;
  std::vector<unsigned> GMEM_total_requests;
  std::vector<unsigned> GMEM_read_transactions;
  std::vector<unsigned> GMEM_write_transactions;
  std::vector<unsigned> GMEM_total_transactions;
  std::vector<float> Number_of_read_transactions_per_read_requests;
  std::vector<float> Number_of_write_transactions_per_write_requests;
  */

  void set_GEMM_read_requests(unsigned value, unsigned smid) { GMEM_read_requests[smid] = value; }
  unsigned get_GEMM_read_requests(unsigned smid) { return GMEM_read_requests[smid]; }
  void set_GEMM_write_requests(unsigned value, unsigned smid) { GMEM_write_requests[smid] = value; }
  unsigned get_GEMM_write_requests(unsigned smid) { return GMEM_write_requests[smid]; }
  void set_GEMM_total_requests(unsigned value, unsigned smid) { GMEM_total_requests[smid] = value; }
  unsigned get_GEMM_total_requests(unsigned smid) { return GMEM_total_requests[smid]; }
  void set_GEMM_read_transactions(unsigned value, unsigned smid) { GMEM_read_transactions[smid] = value; }
  unsigned get_GEMM_read_transactions(unsigned smid) { return GMEM_read_transactions[smid]; }
  void set_GEMM_write_transactions(unsigned value, unsigned smid) { GMEM_write_transactions[smid] = value; }
  unsigned get_GEMM_write_transactions(unsigned smid) { return GMEM_write_transactions[smid]; }
  void set_GEMM_total_transactions(unsigned value, unsigned smid) { GMEM_total_transactions[smid] = value; }
  unsigned get_GEMM_total_transactions(unsigned smid) { return GMEM_total_transactions[smid]; }
  void set_Number_of_read_transactions_per_read_requests(float value, unsigned smid) { Number_of_read_transactions_per_read_requests[smid] = value; }
  float get_Number_of_read_transactions_per_read_requests(unsigned smid) { return Number_of_read_transactions_per_read_requests[smid]; }
  void set_Number_of_write_transactions_per_write_requests(float value, unsigned smid) { Number_of_write_transactions_per_write_requests[smid] = value; }
  float get_Number_of_write_transactions_per_write_requests(unsigned smid) { return Number_of_write_transactions_per_write_requests[smid]; }

  /*
  std::vector<unsigned> Total_number_of_global_atomic_requests;
  std::vector<unsigned> Total_number_of_global_reduction_requests;
  std::vector<unsigned> Global_memory_atomic_and_reduction_transactions;
  */
  void set_Total_number_of_global_atomic_requests(unsigned value, unsigned smid) { Total_number_of_global_atomic_requests[smid] = value; }
  unsigned get_Total_number_of_global_atomic_requests(unsigned smid) { return Total_number_of_global_atomic_requests[smid]; }
  void set_Total_number_of_global_reduction_requests(unsigned value, unsigned smid) { Total_number_of_global_reduction_requests[smid] = value; }
  unsigned get_Total_number_of_global_reduction_requests(unsigned smid) { return Total_number_of_global_reduction_requests[smid]; }
  void set_Global_memory_atomic_and_reduction_transactions(unsigned value, unsigned smid) { Global_memory_atomic_and_reduction_transactions[smid] = value; }
  unsigned get_Global_memory_atomic_and_reduction_transactions(unsigned smid) { return Global_memory_atomic_and_reduction_transactions[smid]; }

  /*
  unsigned L2_read_transactions;
  unsigned L2_write_transactions;
  unsigned L2_total_transactions;
  unsigned DRAM_total_transactions;
  */
  void set_L2_read_transactions(unsigned value, unsigned smid) { L2_read_transactions[smid] = value; }
  unsigned get_L2_read_transactions(unsigned smid) { return L2_read_transactions[smid]; }
  void set_L2_write_transactions(unsigned value, unsigned smid) { L2_write_transactions[smid] = value; }
  unsigned get_L2_write_transactions(unsigned smid) { return L2_write_transactions[smid]; }
  void set_L2_total_transactions(unsigned value, unsigned smid) { L2_total_transactions[smid] = value; }
  unsigned get_L2_total_transactions(unsigned smid) { return L2_total_transactions[smid]; }

  void set_DRAM_total_transactions(unsigned value) { DRAM_total_transactions = value; }
  unsigned get_DRAM_total_transactions() { return DRAM_total_transactions; }

  /*
  std::vector<unsigned> GPU_active_cycles;
  std::vector<unsigned> SM_active_cycles;
  */
  void set_GPU_active_cycles(unsigned value, unsigned smid) { GPU_active_cycles[smid] = value; }
  unsigned get_GPU_active_cycles(unsigned smid) { return GPU_active_cycles[smid]; }
  void set_SM_active_cycles(unsigned value, unsigned smid) { SM_active_cycles[smid] = value; }
  unsigned get_SM_active_cycles(unsigned smid) { return SM_active_cycles[smid]; }

  /*
  std::vector<unsigned> Warp_instructions_executed;
  std::vector<float> Instructions_executed_per_clock_cycle_IPC;
  std::vector<float> Total_instructions_executed_per_seconds;
  */
  void set_Warp_instructions_executed(unsigned value, unsigned smid) { Warp_instructions_executed[smid] = value; }
  unsigned get_Warp_instructions_executed(unsigned smid) { return Warp_instructions_executed[smid]; }
  void set_Instructions_executed_per_clock_cycle_IPC(float value, unsigned smid) { Instructions_executed_per_clock_cycle_IPC[smid] = value; }
  float get_Instructions_executed_per_clock_cycle_IPC(unsigned smid) { return Instructions_executed_per_clock_cycle_IPC[smid]; }
  void set_Total_instructions_executed_per_seconds(float value, unsigned smid) { Total_instructions_executed_per_seconds[smid] = value; }
  float get_Total_instructions_executed_per_seconds(unsigned smid) { return Total_instructions_executed_per_seconds[smid]; }

  /*
  std::vector<unsigned> Kernel_execution_time;
  */
  void set_Kernel_execution_time(unsigned value, unsigned smid) { Kernel_execution_time[smid] = value; }
  unsigned get_Kernel_execution_time(unsigned smid) { return Kernel_execution_time[smid]; }

  /*
  std::vector<float> Simulation_time_memory_model;
  std::vector<float> Simulation_time_compute_model;
  */
  void set_Simulation_time_memory_model(float value, unsigned smid) { Simulation_time_memory_model[smid] = value; }
  float get_Simulation_time_memory_model(unsigned smid) { return Simulation_time_memory_model[smid]; }
  void set_Simulation_time_compute_model(float value, unsigned smid) { Simulation_time_compute_model[smid] = value; }
  float get_Simulation_time_compute_model(unsigned smid) { return Simulation_time_compute_model[smid]; }

  /*
  unsigned kernel_id;
  */
  unsigned get_kernel_id() { return kernel_id; }
  void set_kernel_id(unsigned value) { kernel_id = value; }

  /*
  std::vector<float> Achieved_active_warps_per_SM;
  std::vector<float> Achieved_occupancy;
  */
  void set_Achieved_active_warps_per_SM(float value, unsigned smid) { Achieved_active_warps_per_SM[smid] = value; }
  float get_Achieved_active_warps_per_SM(unsigned smid) { return Achieved_active_warps_per_SM[smid]; }
  void set_Achieved_occupancy(float value, unsigned smid) { Achieved_occupancy[smid] = value; }
  float get_Achieved_occupancy(unsigned smid) { return Achieved_occupancy[smid]; }

  void dump_output(const std::string& path, unsigned rank);



 private:
  unsigned kernel_id;

  unsigned Thread_block_limit_SM;
  unsigned Thread_block_limit_registers;
  unsigned Thread_block_limit_shared_memory;
  unsigned Thread_block_limit_warps;            //v
  unsigned Theoretical_max_active_warps_per_SM;
  float Theoretical_occupancy;

  std::vector<float> Achieved_active_warps_per_SM;
  std::vector<float> Achieved_occupancy;
  std::vector<float> Unified_L1_cache_hit_rate;
  std::vector<unsigned> Unified_L1_cache_requests;
  std::vector<float> Unified_L1_cache_hit_rate_for_read_transactions;// (global memory accesses)	
  float L2_cache_hit_rate;
  unsigned L2_cache_requests;
  std::vector<unsigned> GMEM_read_requests;
  std::vector<unsigned> GMEM_write_requests;
  std::vector<unsigned> GMEM_total_requests;
  std::vector<unsigned> GMEM_read_transactions;
  std::vector<unsigned> GMEM_write_transactions;
  std::vector<unsigned> GMEM_total_transactions;
  std::vector<float> Number_of_read_transactions_per_read_requests;
  std::vector<float> Number_of_write_transactions_per_write_requests;

  

  std::vector<unsigned> L2_read_transactions;
  std::vector<unsigned> L2_write_transactions;
  std::vector<unsigned> L2_total_transactions;
  unsigned DRAM_total_transactions;

  std::vector<unsigned> Total_number_of_global_atomic_requests;
  std::vector<unsigned> Total_number_of_global_reduction_requests;
  std::vector<unsigned> Global_memory_atomic_and_reduction_transactions;

  std::vector<unsigned> GPU_active_cycles;
  std::vector<unsigned> SM_active_cycles;
  std::vector<unsigned> Warp_instructions_executed;
  std::vector<float> Instructions_executed_per_clock_cycle_IPC;
  std::vector<float> Total_instructions_executed_per_seconds;
  std::vector<unsigned> Kernel_execution_time;
  
  std::vector<float> Simulation_time_memory_model;
  std::vector<float> Simulation_time_compute_model;

  unsigned m_num_sm; // TODO: SM70
  unsigned max_block_size; // TODO: SM70
  unsigned warp_size; // TODO: SM70
  unsigned smem_allocation_size; // TODO: SM70
  unsigned max_registers_per_SM; // TODO: SM70
  unsigned max_registers_per_block; // TODO: SM70
  unsigned max_registers_per_thread; // TODO: SM70
  unsigned register_allocation_size; // TODO: SM70
  unsigned max_active_blocks_per_SM; // TODO: SM70
  unsigned max_active_threads_per_SM; // TODO: SM70

  unsigned shared_mem_size; // TODO: V100

  unsigned total_num_workloads;
  unsigned active_SMs;
  unsigned allocated_active_warps_per_block;
  unsigned allocated_active_blocks_per_SM;
};

class PrivateSM {
 public:
  PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg);
  ~PrivateSM();
  void run(unsigned KERNEL_EVALUATION, unsigned MEM_ACCESS_LATENCY);

  bool get_active() { return active; }
  unsigned long long get_cycle() { return m_cycle; }

  bool is_active() { return active; }
  bool check_active();

  unsigned get_num_warps_per_sm(unsigned kernel_id);

  unsigned get_num_warp_instns_executed() { return num_warp_instns_executed; }

  hw_config* get_hw_cfg() { return m_hw_cfg; }
  trace_parser* get_tracer() { return tracer; }
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

  void parse_blocks_per_kernel();
  
  std::vector<unsigned> get_blocks_per_kernel(unsigned kernel_id);

  std::map<unsigned, std::vector<unsigned>>* get_blocks_per_kernel();

  unsigned get_inst_fetch_throughput();
  unsigned get_reg_file_port_throughput();

  void issue_warp(register_set &pipe_reg_set, ibuffer_entry entry, unsigned sch_id);

  RegisterBankAllocator* get_reg_bank_allocator() { return m_reg_bank_allocator; }

  unsigned test_result_bus(unsigned latency) {
    for (unsigned i = 0; i < num_result_bus; i++) {
      if (!m_result_bus[i]->test(latency)) {
        return i;
      }
    }
    return -1;
  }

  unsigned get_active_cycles() { return active_cycles; }
  unsigned long long get_active_warps_id_size_sum() { return active_warps_id_size_sum; }
  /*
  unsigned m_active_warps;
  unsigned max_warps_init;
  */
  unsigned get_active_warps() { return m_active_warps; }
  unsigned get_max_warps_init() { return max_warps_init; }

  

 private:
  unsigned m_smid;
  unsigned long long m_cycle;
  bool active;

  unsigned m_active_warps;
  unsigned max_warps_init;

  unsigned long long active_cycles;
  unsigned long long active_warps_id_size_sum;

  unsigned num_banks;
  unsigned bank_warp_shift;
  unsigned num_scheds;
  bool sub_core_model;
  unsigned banks_per_sched;
  unsigned inst_fetch_throughput;
  unsigned reg_file_port_throughput;

  unsigned num_warp_instns_executed;

  // number of warps per kernel that are allocated to this SM
  std::vector<unsigned> m_num_warps_per_sm;

  std::vector<unsigned> m_num_blocks_per_kernel;

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
  inst_fetch_buffer_entry* m_inst_fetch_buffer_copy;               // yangjianchao16 add 20240131

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

  Scoreboard* m_scoreboard;

  /* key - shed_id : value - kernel_id */

  // int last_fetch_warp_id;
  /* key - kid : value - wid */
  std::map<int, int> last_fetch_warp_id;
  int distance_last_fetch_kid;
  int last_issue_sched_id;
  // std::vector<int> last_issue_warp_ids;
  /* Key: <kid, block_id>, value: warp_id. */
  std::map<std::pair<int, int>, int> last_issue_warp_ids;

  std::vector<int> last_issue_block_index_per_sched;

  RegisterBankAllocator* m_reg_bank_allocator;

  opndcoll_rfu_t* m_operand_collector;

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  std::map<curr_instn_id_per_warp_entry, unsigned> curr_instn_id_per_warp;

  std::vector<stage_instns_identifier> fetch_stage_instns;

  std::vector<stage_instns_identifier> writeback_stage_instns;

  std::vector<stage_instns_identifier> warp_exit_stage_instns;

  hw_config* m_hw_cfg;

  std::vector<pipelined_simd_unit*> m_fu;
  std::vector<unsigned> m_dispatch_port;
  std::vector<unsigned> m_issue_port;

  unsigned num_result_bus;
  std::vector<std::bitset<MAX_ALU_LATENCY>*> m_result_bus;

  std::vector<std::vector<bool>> m_warp_active_status;
};

#endif