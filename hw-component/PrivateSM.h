
#include <vector>
#include <utility>
#include <map>
#include <tuple>
#include "../trace-parser/trace-parser.h"
#include <memory>

#include <sys/stat.h>
#include <unistd.h>
#include <chrono>

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
#define MAX_ALU_LATENCY 1024

#define PRINT_STALLS_DISTRIBUTION 0

// #define KERNEL_EVALUATION 0


// #define START_TIMER(no) auto start##no = std::chrono::system_clock::now();

// #define STOP_AND_REPORT_TIMER_rank(no) \
//     auto end##no = std::chrono::system_clock::now(); \
//     auto duration##no = std::chrono::duration_cast<std::chrono::microseconds>(end##no - start##no); \
//     auto cost##no = double(duration##no.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den; \
//     std::cout << "Cost " << no << "-" << cost##no << " seconds." << std::endl;

#define START_TIMER(no) ;
#define STOP_AND_REPORT_TIMER_rank(no) ;

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
  float get_L2_cache_hit_rate() {
    return L2_cache_hit_rate;
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

  
  /*
  Stalls
  */
  void set_Compute_Structural_Stall(unsigned value, unsigned smid) { Compute_Structural_Stall[smid] = value; }
  void increment_Compute_Structural_Stall(unsigned smid) { Compute_Structural_Stall[smid]++; }
  unsigned get_Compute_Structural_Stall(unsigned smid) { return Compute_Structural_Stall[smid]; }
  void set_Compute_Data_Stall(unsigned value, unsigned smid) { Compute_Data_Stall[smid] = value; }
  void increment_Compute_Data_Stall(unsigned smid) { Compute_Data_Stall[smid]++; }
  unsigned get_Compute_Data_Stall(unsigned smid) { return Compute_Data_Stall[smid]; }
  void set_Memory_Structural_Stall(unsigned value, unsigned smid) { Memory_Structural_Stall[smid] = value; }
  void increment_Memory_Structural_Stall(unsigned smid) { Memory_Structural_Stall[smid]++; }
  unsigned get_Memory_Structural_Stall(unsigned smid) { return Memory_Structural_Stall[smid]; }
  void set_Memory_Data_Stall(unsigned value, unsigned smid) { Memory_Data_Stall[smid] = value; }
  void increment_Memory_Data_Stall(unsigned smid) { Memory_Data_Stall[smid]++; }
  unsigned get_Memory_Data_Stall(unsigned smid) { return Memory_Data_Stall[smid]; }
  void set_Synchronization_Stall(unsigned value, unsigned smid) { Synchronization_Stall[smid] = value; }
  void increment_Synchronization_Stall(unsigned smid) { Synchronization_Stall[smid]++; }
  unsigned get_Synchronization_Stall(unsigned smid) { return Synchronization_Stall[smid]; }
  void set_Control_Stall(unsigned value, unsigned smid) { Control_Stall[smid] = value; }
  void increment_Control_Stall(unsigned smid) { Control_Stall[smid]++; }
  unsigned get_Control_Stall(unsigned smid) { return Control_Stall[smid]; }
  void set_Idle_Stall(unsigned value, unsigned smid) { Idle_Stall[smid] = value; }
  void increment_Idle_Stall(unsigned smid) { Idle_Stall[smid]++; }
  unsigned get_Idle_Stall(unsigned smid) { return Idle_Stall[smid]; }
  void set_No_Stall(unsigned value, unsigned smid) { No_Stall[smid] = value; }
  void increment_No_Stall(unsigned smid) { No_Stall[smid]++; }
  unsigned get_No_Stall(unsigned smid) { return No_Stall[smid]; }
  void set_Other_Stall(unsigned value, unsigned smid) { Other_Stall[smid] = value; }
  void increment_Other_Stall(unsigned smid) { Other_Stall[smid]++; }
  unsigned get_Other_Stall(unsigned smid) { return Other_Stall[smid]; }

  /*
  bool At_least_four_instns_issued;
  bool At_least_one_Compute_Structural_Stall_found;
  bool At_least_one_Compute_Data_Stall_found;
  bool At_least_one_Memory_Structural_Stall_found;
  bool At_least_one_Memory_Data_Stall_found;
  bool At_least_one_Synchronization_Stall_found;
  bool At_least_one_Control_Stall_found;
  bool At_least_one_Idle_Stall_found;
  bool At_least_one_No_Stall_found;
  */
  void set_At_least_four_instns_issued(bool value) { At_least_four_instns_issued = value; }
  bool get_At_least_four_instns_issued() { return At_least_four_instns_issued; }
  void set_At_least_one_Compute_Structural_Stall_found(bool value) { At_least_one_Compute_Structural_Stall_found = value; }
  bool get_At_least_one_Compute_Structural_Stall_found() { return At_least_one_Compute_Structural_Stall_found; }
  void set_At_least_one_Compute_Data_Stall_found(bool value) { At_least_one_Compute_Data_Stall_found = value; }
  bool get_At_least_one_Compute_Data_Stall_found() { return At_least_one_Compute_Data_Stall_found; }
  void set_At_least_one_Memory_Structural_Stall_found(bool value) { At_least_one_Memory_Structural_Stall_found = value; }
  bool get_At_least_one_Memory_Structural_Stall_found() { return At_least_one_Memory_Structural_Stall_found; }
  void set_At_least_one_Memory_Data_Stall_found(bool value) { At_least_one_Memory_Data_Stall_found = value; }
  bool get_At_least_one_Memory_Data_Stall_found() { return At_least_one_Memory_Data_Stall_found; }
  void set_At_least_one_Synchronization_Stall_found(bool value) { At_least_one_Synchronization_Stall_found = value; }
  bool get_At_least_one_Synchronization_Stall_found() { return At_least_one_Synchronization_Stall_found; }
  void set_At_least_one_Control_Stall_found(bool value) { At_least_one_Control_Stall_found = value; }
  bool get_At_least_one_Control_Stall_found() { return At_least_one_Control_Stall_found; }
  void set_At_least_one_Idle_Stall_found(bool value) { At_least_one_Idle_Stall_found = value; }
  bool get_At_least_one_Idle_Stall_found() { return At_least_one_Idle_Stall_found; }
  void set_At_least_one_No_Stall_found(bool value) { At_least_one_No_Stall_found = value; }
  bool get_At_least_one_No_Stall_found() { return At_least_one_No_Stall_found; }
  
  /*
  std::vector<unsigned> num_Issue_Compute_Structural_out_has_no_free_slot;
  std::vector<unsigned> num_Issue_Memory_Structural_out_has_no_free_slot;
  std::vector<unsigned> num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute;
  std::vector<unsigned> num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory;
  std::vector<unsigned> num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency;
  std::vector<unsigned> num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency;
  std::vector<unsigned> num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty;
  std::vector<unsigned> num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty;
  std::vector<unsigned> num_Writeback_Compute_Structural_bank_of_reg_is_not_idle;
  std::vector<unsigned> num_Writeback_Memory_Structural_bank_of_reg_is_not_idle;
  std::vector<unsigned> num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated;
  std::vector<unsigned> num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated;
  std::vector<unsigned> num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu;
  std::vector<unsigned> num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu;
  std::vector<unsigned> num_Execute_Memory_Structural_icnt_injection_buffer_is_full;
  std::vector<unsigned> num_Issue_Compute_Data_scoreboard;
  std::vector<unsigned> num_Issue_Memory_Data_scoreboard;
  std::vector<unsigned> num_Execute_Memory_Data_L1;
  std::vector<unsigned> num_Execute_Memory_Data_L2;
  std::vector<unsigned> num_Execute_Memory_Data_Main_Memory;
  */
  
  void set_num_Issue_Compute_Structural_out_has_no_free_slot(unsigned value, unsigned smid) { num_Issue_Compute_Structural_out_has_no_free_slot[smid] = value; }
  void increment_num_Issue_Compute_Structural_out_has_no_free_slot(unsigned smid) { num_Issue_Compute_Structural_out_has_no_free_slot[smid]++; }
  void increment_num_Issue_Compute_Structural_out_has_no_free_slot(unsigned value, unsigned smid) { num_Issue_Compute_Structural_out_has_no_free_slot[smid]+=value; }
  unsigned get_num_Issue_Compute_Structural_out_has_no_free_slot(unsigned smid) { return num_Issue_Compute_Structural_out_has_no_free_slot[smid]; }
  void set_num_Issue_Memory_Structural_out_has_no_free_slot(unsigned value, unsigned smid) { num_Issue_Memory_Structural_out_has_no_free_slot[smid] = value; }
  void increment_num_Issue_Memory_Structural_out_has_no_free_slot(unsigned smid) { num_Issue_Memory_Structural_out_has_no_free_slot[smid]++; }
  void increment_num_Issue_Memory_Structural_out_has_no_free_slot(unsigned value, unsigned smid) { num_Issue_Memory_Structural_out_has_no_free_slot[smid]+=value; }
  unsigned get_num_Issue_Memory_Structural_out_has_no_free_slot(unsigned smid) { return num_Issue_Memory_Structural_out_has_no_free_slot[smid]; }
  void set_num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute(unsigned value, unsigned smid) { num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute[smid] = value; }
  void increment_num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute(unsigned smid) { num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute[smid]++; }
  void increment_num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute(unsigned value, unsigned smid) { num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute[smid]+=value; }
  unsigned get_num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute(unsigned smid) { return num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute[smid]; }
  void set_num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory(unsigned value, unsigned smid) { num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory[smid] = value; }
  void increment_num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory(unsigned smid) { num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory[smid]++; }
  void increment_num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory(unsigned value, unsigned smid) { num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory[smid]+=value; }
  unsigned get_num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory(unsigned smid) { return num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory[smid]; }
  void set_num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency(unsigned value, unsigned smid) { num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency[smid] = value; }
  void increment_num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency(unsigned smid) { num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency[smid]++; }
  void increment_num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency(unsigned value, unsigned smid) { num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency[smid]+=value; }
  unsigned get_num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency(unsigned smid) { return num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency[smid]; }
  void set_num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency(unsigned value, unsigned smid) { num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency[smid] = value; }
  void increment_num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency(unsigned smid) { num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency[smid]++; }
  void increment_num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency(unsigned value, unsigned smid) { num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency[smid]+=value; }
  unsigned get_num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency(unsigned smid) { return num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency[smid]; }
  void set_num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned value, unsigned smid) { num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty[smid] = value; }
  void increment_num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned smid) { num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty[smid]++; }
  void increment_num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned value, unsigned smid) { num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty[smid]+=value; }
  unsigned get_num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned smid) { return num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty[smid]; }
  void set_num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned value, unsigned smid) { num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty[smid] = value; }
  void increment_num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned smid) { num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty[smid]++; }
  void increment_num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned value, unsigned smid) { num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty[smid]+=value; }
  unsigned get_num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty(unsigned smid) { return num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty[smid]; }
  void set_num_Writeback_Compute_Structural_bank_of_reg_is_not_idle(unsigned value, unsigned smid) { num_Writeback_Compute_Structural_bank_of_reg_is_not_idle[smid] = value; }
  void increment_num_Writeback_Compute_Structural_bank_of_reg_is_not_idle(unsigned smid) { num_Writeback_Compute_Structural_bank_of_reg_is_not_idle[smid]++; }
  void increment_num_Writeback_Compute_Structural_bank_of_reg_is_not_idle(unsigned value, unsigned smid) { num_Writeback_Compute_Structural_bank_of_reg_is_not_idle[smid]+=value; }
  unsigned get_num_Writeback_Compute_Structural_bank_of_reg_is_not_idle(unsigned smid) { return num_Writeback_Compute_Structural_bank_of_reg_is_not_idle[smid]; }
  void set_num_Writeback_Memory_Structural_bank_of_reg_is_not_idle(unsigned value, unsigned smid) { num_Writeback_Memory_Structural_bank_of_reg_is_not_idle[smid] = value; }
  void increment_num_Writeback_Memory_Structural_bank_of_reg_is_not_idle(unsigned smid) { num_Writeback_Memory_Structural_bank_of_reg_is_not_idle[smid]++; }
  void increment_num_Writeback_Memory_Structural_bank_of_reg_is_not_idle(unsigned value, unsigned smid) { num_Writeback_Memory_Structural_bank_of_reg_is_not_idle[smid]+=value; }
  unsigned get_num_Writeback_Memory_Structural_bank_of_reg_is_not_idle(unsigned smid) { return num_Writeback_Memory_Structural_bank_of_reg_is_not_idle[smid]; }
  void set_num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated(unsigned value, unsigned smid) { num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated[smid] = value; }
  void increment_num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated(unsigned smid) { num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated[smid]++; }
  void increment_num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated(unsigned value, unsigned smid) { num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated[smid]+=value; }
  unsigned get_num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated(unsigned smid) { return num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated[smid]; }
  void set_num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated(unsigned value, unsigned smid) { num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated[smid] = value; }
  void increment_num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated(unsigned smid) { num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated[smid]++; }
  void increment_num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated(unsigned value, unsigned smid) { num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated[smid]+=value; }
  unsigned get_num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated(unsigned smid) { return num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated[smid]; }
  void set_num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned value, unsigned smid) { num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid] = value; }
  void increment_num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned smid) { num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid]++; }
  void increment_num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned value, unsigned smid) { num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid]+=value; }
  unsigned get_num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned smid) { return num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid]; }
  void set_num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned value, unsigned smid) { num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid] = value; }
  void increment_num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned smid) { num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid]++; }
  void increment_num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned value, unsigned smid) { num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid]+=value; }
  unsigned get_num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(unsigned smid) { return num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[smid]; }
  void set_num_Execute_Memory_Structural_icnt_injection_buffer_is_full(unsigned value, unsigned smid) { num_Execute_Memory_Structural_icnt_injection_buffer_is_full[smid] = value; }
  void increment_num_Execute_Memory_Structural_icnt_injection_buffer_is_full(unsigned smid) { num_Execute_Memory_Structural_icnt_injection_buffer_is_full[smid]++; }
  void increment_num_Execute_Memory_Structural_icnt_injection_buffer_is_full(unsigned value, unsigned smid) { num_Execute_Memory_Structural_icnt_injection_buffer_is_full[smid]+=value; }
  unsigned get_num_Execute_Memory_Structural_icnt_injection_buffer_is_full(unsigned smid) { return num_Execute_Memory_Structural_icnt_injection_buffer_is_full[smid]; }
  void set_num_Issue_Compute_Data_scoreboard(unsigned value, unsigned smid) { num_Issue_Compute_Data_scoreboard[smid] = value; }
  void increment_num_Issue_Compute_Data_scoreboard(unsigned smid) { num_Issue_Compute_Data_scoreboard[smid]++; }
  void increment_num_Issue_Compute_Data_scoreboard(unsigned value, unsigned smid) { num_Issue_Compute_Data_scoreboard[smid]+=value; }
  unsigned get_num_Issue_Compute_Data_scoreboard(unsigned smid) { return num_Issue_Compute_Data_scoreboard[smid]; }
  void set_num_Issue_Memory_Data_scoreboard(unsigned value, unsigned smid) { num_Issue_Memory_Data_scoreboard[smid] = value; }
  void increment_num_Issue_Memory_Data_scoreboard(unsigned smid) { num_Issue_Memory_Data_scoreboard[smid]++; }
  void increment_num_Issue_Memory_Data_scoreboard(unsigned value, unsigned smid) { num_Issue_Memory_Data_scoreboard[smid]+=value; }
  unsigned get_num_Issue_Memory_Data_scoreboard(unsigned smid) { return num_Issue_Memory_Data_scoreboard[smid]; }
  void set_num_Execute_Memory_Data_L1(unsigned value, unsigned smid) { num_Execute_Memory_Data_L1[smid] = value; }
  void increment_num_Execute_Memory_Data_L1(unsigned smid) { num_Execute_Memory_Data_L1[smid]++; }
  void increment_num_Execute_Memory_Data_L1(unsigned value, unsigned smid) { num_Execute_Memory_Data_L1[smid]+=value; }
  unsigned get_num_Execute_Memory_Data_L1(unsigned smid) { return num_Execute_Memory_Data_L1[smid]; }
  void set_num_Execute_Memory_Data_L2(unsigned value, unsigned smid) { num_Execute_Memory_Data_L2[smid] = value; }
  void increment_num_Execute_Memory_Data_L2(unsigned smid) { num_Execute_Memory_Data_L2[smid]++; }
  void increment_num_Execute_Memory_Data_L2(unsigned value, unsigned smid) { num_Execute_Memory_Data_L2[smid]+=value; }
  unsigned get_num_Execute_Memory_Data_L2(unsigned smid) { return num_Execute_Memory_Data_L2[smid]; }
  void set_num_Execute_Memory_Data_Main_Memory(unsigned value, unsigned smid) { num_Execute_Memory_Data_Main_Memory[smid] = value; }
  void increment_num_Execute_Memory_Data_Main_Memory(unsigned smid) { num_Execute_Memory_Data_Main_Memory[smid]++; }
  void increment_num_Execute_Memory_Data_Main_Memory(unsigned value, unsigned smid) { num_Execute_Memory_Data_Main_Memory[smid]+=value; }
  unsigned get_num_Execute_Memory_Data_Main_Memory(unsigned smid) { return num_Execute_Memory_Data_Main_Memory[smid]; }
  
  /*
  std::vector<unsigned> SP_UNIT_execute_clks_sum;
  std::vector<unsigned> SFU_UNIT_execute_clks_sum;
  std::vector<unsigned> INT_UNIT_execute_clks_sum;
  std::vector<unsigned> DP_UNIT_execute_clks_sum;
  std::vector<unsigned> TENSOR_CORE_UNIT_execute_clks_sum;
  std::vector<unsigned> LDST_UNIT_execute_clks_sum;
  std::vector<unsigned> SPEC_UNIT_1_execute_clks_sum;
  std::vector<unsigned> SPEC_UNIT_2_execute_clks_sum;
  std::vector<unsigned> SPEC_UNIT_3_execute_clks_sum;
  std::vector<unsigned> Other_UNIT_execute_clks_sum;
  */
  void increment_SP_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { SP_UNIT_execute_clks_sum[smid] += value; }
  void increment_SFU_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { SFU_UNIT_execute_clks_sum[smid] += value; }
  void increment_INT_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { INT_UNIT_execute_clks_sum[smid] += value; }
  void increment_DP_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { DP_UNIT_execute_clks_sum[smid] += value; }
  void increment_TENSOR_CORE_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { TENSOR_CORE_UNIT_execute_clks_sum[smid] += value; }
  void increment_LDST_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { LDST_UNIT_execute_clks_sum[smid] += value; }
  void increment_SPEC_UNIT_1_execute_clks_sum(unsigned smid, unsigned long long value) { SPEC_UNIT_1_execute_clks_sum[smid] += value; }
  void increment_SPEC_UNIT_2_execute_clks_sum(unsigned smid, unsigned long long value) { SPEC_UNIT_2_execute_clks_sum[smid] += value; }
  void increment_SPEC_UNIT_3_execute_clks_sum(unsigned smid, unsigned long long value) { SPEC_UNIT_3_execute_clks_sum[smid] += value; }
  void increment_Other_UNIT_execute_clks_sum(unsigned smid, unsigned long long value) { Other_UNIT_execute_clks_sum[smid] += value; }

  /*
  std::vector<unsigned long long> SP_UNIT_Instns_num;
  std::vector<unsigned long long> SFU_UNIT_Instns_num;
  std::vector<unsigned long long> INT_UNIT_Instns_num;
  std::vector<unsigned long long> DP_UNIT_Instns_num;
  std::vector<unsigned long long> TENSOR_CORE_UNIT_Instns_num;
  std::vector<unsigned long long> LDST_UNIT_Instns_num;
  std::vector<unsigned long long> SPEC_UNIT_1_Instns_num;
  std::vector<unsigned long long> SPEC_UNIT_2_Instns_num;
  std::vector<unsigned long long> SPEC_UNIT_3_Instns_num;
  std::vector<unsigned long long> Other_UNIT_Instns_num;
  */
  void increment_SP_UNIT_Instns_num(unsigned smid) { SP_UNIT_Instns_num[smid]++; }
  void increment_SFU_UNIT_Instns_num(unsigned smid) { SFU_UNIT_Instns_num[smid]++; }
  void increment_INT_UNIT_Instns_num(unsigned smid) { INT_UNIT_Instns_num[smid]++; }
  void increment_DP_UNIT_Instns_num(unsigned smid) { DP_UNIT_Instns_num[smid]++; }
  void increment_TENSOR_CORE_UNIT_Instns_num(unsigned smid) { TENSOR_CORE_UNIT_Instns_num[smid]++; }
  void increment_LDST_UNIT_Instns_num(unsigned smid) { LDST_UNIT_Instns_num[smid]++; }
  void increment_SPEC_UNIT_1_Instns_num(unsigned smid) { SPEC_UNIT_1_Instns_num[smid]++; }
  void increment_SPEC_UNIT_2_Instns_num(unsigned smid) { SPEC_UNIT_2_Instns_num[smid]++; }
  void increment_SPEC_UNIT_3_Instns_num(unsigned smid) { SPEC_UNIT_3_Instns_num[smid]++; }
  void increment_Other_UNIT_Instns_num(unsigned smid) { Other_UNIT_Instns_num[smid]++; }

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

  // Stalls 

  /*
  bool At_least_four_instns_issued;
  bool At_least_one_Compute_Structural_Stall_found;
  bool At_least_one_Compute_Data_Stall_found;
  bool At_least_one_Memory_Structural_Stall_found;
  bool At_least_one_Memory_Data_Stall_found;
  bool At_least_one_Synchronization_Stall_found;
  bool At_least_one_Control_Stall_found;
  bool At_least_one_Idle_Stall_found;
  bool At_least_one_No_Stall_found;
  */
  
  bool At_least_four_instns_issued;

  // A compute structural stall occurs when a compute instruction 
  // cannot issue because the appropriate compute unit is occupied.
  std::vector<unsigned> Compute_Structural_Stall;
  bool At_least_one_Compute_Structural_Stall_found;
  unsigned num_At_least_one_Compute_Structural_Stall_found;

  // A compute data stall occurs when an instruction cannot issue 
  // because it is dependent on the output of a pending compute 
  // (non-memory) instruction.
  std::vector<unsigned> Compute_Data_Stall;
  bool At_least_one_Compute_Data_Stall_found;
  unsigned num_At_least_one_Compute_Data_Stall_found;

  // A memory structural stall occurs when a memory instruction is 
  // unable to issue to the load/store unit because it is full. 
  // There are multiple sources of memory structural stalls, which 
  // we describe in more detail in Section IV-D.
  std::vector<unsigned> Memory_Structural_Stall;
  bool At_least_one_Memory_Structural_Stall_found;
  unsigned num_At_least_one_Memory_Structural_Stall_found;

  // A memory data stall occurs when an instruction cannot issue 
  // because it is dependent on the output of a pending load. 
  // Section IV-C provides more details on the different subcate-
  // gories of memory data stalls.
  std::vector<unsigned> Memory_Data_Stall;
  bool At_least_one_Memory_Data_Stall_found;
  unsigned num_At_least_one_Memory_Data_Stall_found;

  // A synchronization stall occurs when a warp is blocked due to 
  // a pending synchronization operation (acquire, release, or 
  // thread barrier).GPUs use acquire and release operations to 
  // preserve memory consistency and thread barriers to synchronize 
  // threads in a thread block. Acquire and release operations block 
  // a warp until one or more previous memory accesses have completed. 
  // A thread barrier blocks a warp until all other threads in the 
  // thread block have reached the barrier.
  std::vector<unsigned> Synchronization_Stall;
  bool At_least_one_Synchronization_Stall_found;
  unsigned num_At_least_one_Synchronization_Stall_found;

  // A control stall occurs when the instruction supplied by the ins-
  // truction buffer is not the next instruction to be executed in a 
  // warp. If control stalls dominate, there is significant divergence 
  // in the kernel code.
  std::vector<unsigned> Control_Stall;
  bool At_least_one_Control_Stall_found;
  unsigned num_At_least_one_Control_Stall_found;
  
  // An idle stall occurs when there are no active warps available to 
  // issue instructions. A large number of idle stalls indicates that 
  // the kernel is not fully utilizing the GPU because some SMs simply 
  // have no work to do. This may be because the thread blocks are not 
  // evenly distributed across SMs or because thread block execution 
  // time is highly variable.
  std::vector<unsigned> Idle_Stall;
  bool At_least_one_Idle_Stall_found;
  unsigned num_At_least_one_Idle_Stall_found;

  // When an instruction can be issued in a cycle, the cycle is 
  // classified as no stall.
  std::vector<unsigned> No_Stall;
  bool At_least_one_No_Stall_found;
  unsigned num_At_least_one_No_Stall_found;

  std::vector<unsigned> Other_Stall;

    /**********************************************************************************************
    The following events that may cause stalls:
      
    Compute_Structural_Stall:
        1      Issue: m_[compute]_out has no free slot
               num_Issue_Compute_Structural_out_has_no_free_slot
        2      Issue: previous_issued_inst_exec_type is [compute]
               num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute
        3      Execute: [compute] result_bus has no slot for latency-\d+
               num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency
        4      Execute: [compute] m_dispatch_reg of fu\[\d+\]-\w+\s is not empty
               num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty
        5      Writeback: bank-\d+ of reg-\d+ is not idle
               num_Writeback_Compute_Structural_bank_of_reg_is_not_idle
        6      ReadOperands: bank\[\d+\] reg-\d+ \(order:\d+\) belonged to was allocated
               num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated
        7      ReadOperands: port_num-\d+/m_in_ports\[\d+\].m_in\[\d+\] fails as not found free cu
               num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu
    Compute_Data_Stall:
        9      Issue: [compute] scoreboard
               num_Issue_Compute_Data_scoreboard
    Memory_Structural_Stall:
        1      Issue: m_[memory]_out has no free slot
               num_Issue_Memory_Structural_out_has_no_free_slot
        2      Issue: previous_issued_inst_exec_type is [memory]
               num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory
        3      Execute: [memory] result_bus has no slot for latency-\d+
               num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency
        4      Execute: [memory] m_dispatch_reg of fu\[\d+\]-\w+\s is not empty
               num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty
        5      Writeback: bank-\d+ of reg-\d+ is not idle
               num_Writeback_Memory_Structural_bank_of_reg_is_not_idle
        6      ReadOperands: bank\[\d+\] reg-\d+ \(order:\d+\) belonged to was allocated
               num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated
        7      ReadOperands: port_num-\d+/m_in_ports\[\d+\].m_in\[\d+\] fails as not found free cu
               num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu
        8      Execute: icnt_injection_buffer is full
               num_Execute_Memory_Structural_icnt_injection_buffer_is_full
    Memory_Data_Stall:
        9      Issue: [memory] scoreboard
               num_Issue_Memory_Data_scoreboard
        10     Execute: L1 // first calculate 9, and then allocate the remaining stalls to 10,11,12
               num_Execute_Memory_Data_L1
        11     Execute: L2
               num_Execute_Memory_Data_L2
        12     Execute: Main Memory
               num_Execute_Memory_Data_Main_Memory
    **********************************************************************************************/
  std::vector<unsigned> num_Issue_Compute_Structural_out_has_no_free_slot;
  std::vector<unsigned> num_Issue_Memory_Structural_out_has_no_free_slot;
  std::vector<unsigned> num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute;
  std::vector<unsigned> num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory;
  std::vector<unsigned> num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency;
  std::vector<unsigned> num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency;
  std::vector<unsigned> num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty;
  std::vector<unsigned> num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty;
  std::vector<unsigned> num_Writeback_Compute_Structural_bank_of_reg_is_not_idle;
  std::vector<unsigned> num_Writeback_Memory_Structural_bank_of_reg_is_not_idle;
  std::vector<unsigned> num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated;
  std::vector<unsigned> num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated;
  std::vector<unsigned> num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu;
  std::vector<unsigned> num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu;
  std::vector<unsigned> num_Execute_Memory_Structural_icnt_injection_buffer_is_full;
  std::vector<unsigned> num_Issue_Compute_Data_scoreboard;
  std::vector<unsigned> num_Issue_Memory_Data_scoreboard;
  std::vector<unsigned> num_Execute_Memory_Data_L1;
  std::vector<unsigned> num_Execute_Memory_Data_L2;
  std::vector<unsigned> num_Execute_Memory_Data_Main_Memory;


  /*
  std::string func_unit_name_to_string(FUNC_UNITS_NAME unit) {
    switch (unit) {
      case NON_UNIT:
        return "NON_UNIT";
      case SP_UNIT:
        return "SP";
      case SFU_UNIT:
        return "SFU";
      case INT_UNIT:
        return "INT";
      case DP_UNIT:
        return "DP";
      case TENSOR_CORE_UNIT:
        return "TENSOR_CORE";
      case LDST_UNIT:
        return "LDST";
      case SPEC_UNIT_1:
        return "SPEC_1";
      case SPEC_UNIT_2:
        return "SPEC_2";
      case SPEC_UNIT_3:
        return "SPEC_3";
      default:
        return "Others";
    }
  }
  */
  std::vector<unsigned long long> SP_UNIT_execute_clks_sum;
  std::vector<unsigned long long> SFU_UNIT_execute_clks_sum;
  std::vector<unsigned long long> INT_UNIT_execute_clks_sum;
  std::vector<unsigned long long> DP_UNIT_execute_clks_sum;
  std::vector<unsigned long long> TENSOR_CORE_UNIT_execute_clks_sum;
  std::vector<unsigned long long> LDST_UNIT_execute_clks_sum;
  std::vector<unsigned long long> SPEC_UNIT_1_execute_clks_sum;
  std::vector<unsigned long long> SPEC_UNIT_2_execute_clks_sum;
  std::vector<unsigned long long> SPEC_UNIT_3_execute_clks_sum;
  std::vector<unsigned long long> Other_UNIT_execute_clks_sum;

  std::vector<unsigned long long> SP_UNIT_Instns_num;
  std::vector<unsigned long long> SFU_UNIT_Instns_num;
  std::vector<unsigned long long> INT_UNIT_Instns_num;
  std::vector<unsigned long long> DP_UNIT_Instns_num;
  std::vector<unsigned long long> TENSOR_CORE_UNIT_Instns_num;
  std::vector<unsigned long long> LDST_UNIT_Instns_num;
  std::vector<unsigned long long> SPEC_UNIT_1_Instns_num;
  std::vector<unsigned long long> SPEC_UNIT_2_Instns_num;
  std::vector<unsigned long long> SPEC_UNIT_3_Instns_num;
  std::vector<unsigned long long> Other_UNIT_Instns_num;

};

class PrivateSM {
 public:
  PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg);
  ~PrivateSM();
  void run(unsigned KERNEL_EVALUATION, unsigned MEM_ACCESS_LATENCY, stat_collector* stat_coll);

  bool get_active() { return active; }
  unsigned long long get_cycle() { return m_cycle; }
  unsigned long long set_cycle(unsigned long long value) { m_cycle = value; } 
  void increment_cycle(unsigned long long value) { m_cycle += value; }

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

  app_config* get_appcfg() { return appcfg; }
  std::vector<std::pair<int, int>>* get_kernel_block_pair() { return &kernel_block_pair; }
  std::vector<unsigned>* get_num_warps_per_sm() { return &m_num_warps_per_sm; }
  unsigned get_num_scheds() { return num_scheds; }

  unsigned get_num_m_warp_active_status() { 
    unsigned num_active_warps = 0;
    for (unsigned i = 0; i < m_warp_active_status.size(); i++) {
      for (unsigned j = 0; j < m_warp_active_status[i].size(); j++) {
        if (m_warp_active_status[i][j]) {
          num_active_warps++;
        }
      }
    }
    return num_active_warps;
  }

  unsigned get_num_m_warp_active_status(unsigned index) { 
    unsigned num_active_warps = 0;
      for (unsigned j = 0; j < m_warp_active_status[index].size(); j++) {
        if (m_warp_active_status[index][j]) {
          num_active_warps++;
        }
      }
    return num_active_warps;
  }

  template <unsigned pos>
  void set_clk_record(unsigned kid, unsigned wid, unsigned uid, unsigned value) {
    const std::tuple<unsigned, unsigned, unsigned> key(kid, wid, uid);
    auto it = clk_record.find(key);

    if (it == clk_record.end()) {
      std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, unsigned> new_value(0, 0, 0, 0, 0, 0);
      std::get<pos>(new_value) = value;
      clk_record[key] = new_value;
    } else {
      std::get<pos>(it->second) = value;
    }
  }
  template <unsigned pos>
  unsigned get_clk_record(unsigned kid, unsigned wid, unsigned uid) {
    std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, unsigned>& clk_record_value = 
      clk_record[std::make_tuple(kid, wid, uid)];
    return std::get<pos>(clk_record_value);
  }

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
  std::vector<bool> m_thread_block_has_executed_status;

  unsigned last_check_block_id_index_idx = 0;

  std::map<std::pair<unsigned, unsigned>, unsigned> kernel_id_block_id_last_fetch_wid;


  std::map<std::tuple<unsigned, unsigned, unsigned>, 
    std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, unsigned>> clk_record;
};

#endif