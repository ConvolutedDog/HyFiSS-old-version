

#include "PrivateSM.h"

stat_collector::stat_collector(unsigned num_sm, unsigned kernel_id) {
  this->kernel_id = kernel_id;

  m_num_sm = num_sm;
  max_block_size = 1024; // TODO: SM70
  warp_size = 32; // TODO: SM70
  smem_allocation_size = 256; // TODO: SM70
  max_registers_per_SM = 65536; // TODO: SM70
  max_registers_per_block = 65536; // TODO: SM70
  max_registers_per_thread = 255; // TODO: SM70
  register_allocation_size = 256; // TODO: SM70
  max_active_blocks_per_SM = 32; // TODO: SM70
  max_active_threads_per_SM = 2048; // TODO: SM70
  shared_mem_size = 96*1024; // TODO: V100

  active_SMs = 0;

  Thread_block_limit_SM = 32;
  Thread_block_limit_registers = 0;
  Thread_block_limit_shared_memory = 0;
  Thread_block_limit_warps = 0;
  Theoretical_max_active_warps_per_SM = 0;
  Theoretical_occupancy = 0.;

  Achieved_active_warps_per_SM.resize(m_num_sm, 0.);
  Achieved_occupancy.resize(m_num_sm, 0.);
  Unified_L1_cache_hit_rate.resize(m_num_sm, 0.);
  Unified_L1_cache_requests.resize(m_num_sm, 0);
  Unified_L1_cache_hit_rate_for_read_transactions.resize(m_num_sm, 0.);
  L2_cache_hit_rate = 0.;
  L2_cache_requests = 0;
  GMEM_read_requests.resize(m_num_sm, 0);
  GMEM_write_requests.resize(m_num_sm, 0);
  GMEM_total_requests.resize(m_num_sm, 0);
  GMEM_read_transactions.resize(m_num_sm, 0);
  GMEM_write_transactions.resize(m_num_sm, 0);
  GMEM_total_transactions.resize(m_num_sm, 0);
  Number_of_read_transactions_per_read_requests.resize(m_num_sm, 0);
  Number_of_write_transactions_per_write_requests.resize(m_num_sm, 0);
  L2_read_transactions.resize(m_num_sm, 0);
  L2_write_transactions.resize(m_num_sm, 0);
  L2_total_transactions.resize(m_num_sm, 0);
  DRAM_total_transactions = 0;
  Total_number_of_global_atomic_requests.resize(m_num_sm, 0);
  Total_number_of_global_reduction_requests.resize(m_num_sm, 0);
  Global_memory_atomic_and_reduction_transactions.resize(m_num_sm, 0);
  GPU_active_cycles.resize(m_num_sm, 0);
  SM_active_cycles.resize(m_num_sm, 0);
  Warp_instructions_executed.resize(m_num_sm, 0);
  Instructions_executed_per_clock_cycle_IPC.resize(m_num_sm, 0.);
  Total_instructions_executed_per_seconds.resize(m_num_sm, 0.);
  Kernel_execution_time.resize(m_num_sm, 0);
  Simulation_time_memory_model.resize(m_num_sm, 0);
  Simulation_time_compute_model.resize(m_num_sm, 0);

  Compute_Structural_Stall.resize(m_num_sm, 0);
  Compute_Data_Stall.resize(m_num_sm, 0);
  Memory_Structural_Stall.resize(m_num_sm, 0);
  Memory_Data_Stall.resize(m_num_sm, 0);
  Synchronization_Stall.resize(m_num_sm, 0);
  Control_Stall.resize(m_num_sm, 0);
  Idle_Stall.resize(m_num_sm, 0);
  No_Stall.resize(m_num_sm, 0);
  Other_Stall.resize(m_num_sm, 0);

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
  At_least_four_instns_issued = false;
  At_least_one_Compute_Structural_Stall_found = false;
  At_least_one_Compute_Data_Stall_found = false;
  At_least_one_Memory_Structural_Stall_found = false;
  At_least_one_Memory_Data_Stall_found = false;
  At_least_one_Synchronization_Stall_found = false;
  At_least_one_Control_Stall_found = false;
  At_least_one_Idle_Stall_found = false;
  At_least_one_No_Stall_found = false;
    
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
  num_Issue_Compute_Structural_out_has_no_free_slot.resize(m_num_sm, 0);
  num_Issue_Memory_Structural_out_has_no_free_slot.resize(m_num_sm, 0);
  num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute.resize(m_num_sm, 0);
  num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory.resize(m_num_sm, 0);
  num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency.resize(m_num_sm, 0);
  num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency.resize(m_num_sm, 0);
  num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty.resize(m_num_sm, 0);
  num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty.resize(m_num_sm, 0);
  num_Writeback_Compute_Structural_bank_of_reg_is_not_idle.resize(m_num_sm, 0);
  num_Writeback_Memory_Structural_bank_of_reg_is_not_idle.resize(m_num_sm, 0);
  num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated.resize(m_num_sm, 0);
  num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated.resize(m_num_sm, 0);
  num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu.resize(m_num_sm, 0);
  num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu.resize(m_num_sm, 0);
  num_Execute_Memory_Structural_icnt_injection_buffer_is_full.resize(m_num_sm, 0);
  num_Issue_Compute_Data_scoreboard.resize(m_num_sm, 0);
  num_Issue_Memory_Data_scoreboard.resize(m_num_sm, 0);
  num_Execute_Memory_Data_L1.resize(m_num_sm, 0);
  num_Execute_Memory_Data_L2.resize(m_num_sm, 0);
  num_Execute_Memory_Data_Main_Memory.resize(m_num_sm, 0);

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
  SP_UNIT_execute_clks_sum.resize(m_num_sm, 0);
  SFU_UNIT_execute_clks_sum.resize(m_num_sm, 0);
  INT_UNIT_execute_clks_sum.resize(m_num_sm, 0);
  DP_UNIT_execute_clks_sum.resize(m_num_sm, 0);
  TENSOR_CORE_UNIT_execute_clks_sum.resize(m_num_sm, 0);
  LDST_UNIT_execute_clks_sum.resize(m_num_sm, 0);
  SPEC_UNIT_1_execute_clks_sum.resize(m_num_sm, 0);
  SPEC_UNIT_2_execute_clks_sum.resize(m_num_sm, 0);
  SPEC_UNIT_3_execute_clks_sum.resize(m_num_sm, 0);
  Other_UNIT_execute_clks_sum.resize(m_num_sm, 0);
}

bool create_directory_if_not_exists(const std::string& dir) {
  struct stat sb;
  if (stat(dir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    return true; // 目录已存在
  } else {
    // 使用0777权限创建目录，实际使用时可能需要更精确的权限控制
    if (mkdir(dir.c_str(), 0777) == -1) {
      std::cerr << "Error creating directory." << std::endl;
      return false;
    }
    return true;
  }
}

void remove_file_if_exists(const std::string& filepath) {
  // 如果文件存在，则删除
  if (access(filepath.c_str(), F_OK) != -1) {
    // 删除文件
    remove(filepath.c_str());
  }
}

void stat_collector::dump_output(const std::string& path, unsigned rank) {
  // std::string full_path = path + std::string("/rank-") + std::to_string(rank) + std::string(".temp.txt");
  std::string full_dir = path + std::string("/../outputs");
  std::string full_path = path + std::string("/../outputs/kernel-") + std::to_string(get_kernel_id()) + 
                          std::string("-rank-") + std::to_string(rank) + std::string(".temp.txt");
  if (!create_directory_if_not_exists(full_dir)) {
    std::cout << "Error when creating directory" << full_dir << std::endl;
    exit(0); // 错误情况
  }

  remove_file_if_exists(full_path);

  std::ofstream file(full_path);
  if (file.is_open()) {
    file << "From rank: " << rank << std::endl;
      
    file << "Thread_block_limit_SM = " << Thread_block_limit_SM << std::endl;
    file << "Thread_block_limit_registers = " << Thread_block_limit_registers << std::endl;
    file << "Thread_block_limit_shared_memory = " << Thread_block_limit_shared_memory << std::endl;
    file << "Thread_block_limit_warps = " << Thread_block_limit_warps << std::endl;
    file << "Theoretical_max_active_warps_per_SM = " << Theoretical_max_active_warps_per_SM << std::endl;
    file << "Theoretical_occupancy = " << Theoretical_occupancy << std::endl;

    file << std::endl;

    
    file << "Unified_L1_cache_hit_rate[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Unified_L1_cache_hit_rate[sm_id] << " ";;
    }
    file << std::endl;
    file << "Unified_L1_cache_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Unified_L1_cache_requests[sm_id] << " ";;
    }
    file << std::endl;
      
    // unsigned all_misses = 0;
    // unsigned all_requests = 0;
    // for (unsigned i = 0; i < Unified_L1_cache_requests.size(); i++) {
    //   all_misses += Unified_L1_cache_requests[i] * (1. - Unified_L1_cache_hit_rate[i]);
    //   all_requests += Unified_L1_cache_requests[i];
    // }
    // file << "Unified_L1_cache_hit_rate: " << (float)(((float)all_requests - (float)all_misses)/(float)all_requests) << std::endl;
    // file << "Unified_L1_cache_hit_rate_for_read_transactions[rank] = " << Unified_L1_cache_hit_rate_for_read_transactions[rank] << std::endl;

    file << std::endl;

    file << "L2_cache_hit_rate = " << L2_cache_hit_rate << std::endl;
    file << "L2_cache_requests = " << L2_cache_requests << std::endl;

    file << std::endl;

    file << "GMEM_read_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GMEM_read_requests[sm_id] << " ";;
    }
    file << std::endl;
    file << "GMEM_write_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GMEM_write_requests[sm_id] << " ";;
    }
    file << std::endl;
    file << "GMEM_total_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GMEM_total_requests[sm_id] << " ";;
    }
    file << std::endl;
    file << "GMEM_read_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GMEM_read_transactions[sm_id] << " ";;
    }
    file << std::endl;
    file << "GMEM_write_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GMEM_write_transactions[sm_id] << " ";;
    }
    file << std::endl;
    file << "GMEM_total_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GMEM_total_transactions[sm_id] << " ";;
    }
    file << std::endl;

    file << std::endl;

    file << "Number_of_read_transactions_per_read_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Number_of_read_transactions_per_read_requests[sm_id] << " ";;
    }
    file << std::endl;
    file << "Number_of_write_transactions_per_write_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Number_of_write_transactions_per_write_requests[sm_id] << " ";;
    }
    file << std::endl;

    file << std::endl;


    file << "L2_read_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << L2_read_transactions[sm_id] << " ";;
    }
    file << std::endl;
    file << "L2_write_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << L2_write_transactions[sm_id] << " ";;
    }
    file << std::endl;
    file << "L2_total_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << L2_total_transactions[sm_id] << " ";;
    }

    file << std::endl;
    file << std::endl;
    file << "DRAM_total_transactions = " << DRAM_total_transactions;
    file << std::endl;
    file << std::endl;
    file << "Total_number_of_global_atomic_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Total_number_of_global_atomic_requests[sm_id] << " ";;
    }
    file << std::endl;
    file << "Total_number_of_global_reduction_requests[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Total_number_of_global_reduction_requests[sm_id] << " ";;
    }
    file << std::endl;
    file << "Global_memory_atomic_and_reduction_transactions[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Global_memory_atomic_and_reduction_transactions[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "Achieved_active_warps_per_SM[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Achieved_active_warps_per_SM[sm_id] << " ";;
    }
    file << std::endl;
    file << "Achieved_occupancy[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Achieved_occupancy[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "GPU_active_cycles[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << GPU_active_cycles[sm_id] << " ";;
    }
    file << std::endl;
    file << "SM_active_cycles[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << SM_active_cycles[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "Warp_instructions_executed[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Warp_instructions_executed[sm_id] << " ";;
    }
    file << std::endl;
    file << "Instructions_executed_per_clock_cycle_IPC[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Instructions_executed_per_clock_cycle_IPC[sm_id] << " ";;
    }
    file << std::endl;
    file << "Total_instructions_executed_per_seconds[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Total_instructions_executed_per_seconds[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "Kernel_execution_time[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Kernel_execution_time[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;

    file << "Simulation_time_memory_model[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Simulation_time_memory_model[sm_id] << " ";;
    }
    file << std::endl;
    file << "Simulation_time_compute_model[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Simulation_time_compute_model[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "Compute_Structural_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Compute_Structural_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Compute_Data_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Compute_Data_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Memory_Structural_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Memory_Structural_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Memory_Data_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Memory_Data_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Synchronization_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Synchronization_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Control_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Control_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Idle_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Idle_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "No_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << No_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << "Other_Stall[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Other_Stall[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "num_Issue_Compute_Structural_out_has_no_free_slot[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Issue_Compute_Structural_out_has_no_free_slot[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Issue_Memory_Structural_out_has_no_free_slot[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Issue_Memory_Structural_out_has_no_free_slot[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Writeback_Compute_Structural_bank_of_reg_is_not_idle[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Writeback_Compute_Structural_bank_of_reg_is_not_idle[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Writeback_Memory_Structural_bank_of_reg_is_not_idle[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Writeback_Memory_Structural_bank_of_reg_is_not_idle[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Memory_Structural_icnt_injection_buffer_is_full[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Memory_Structural_icnt_injection_buffer_is_full[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Issue_Compute_Data_scoreboard[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Issue_Compute_Data_scoreboard[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Issue_Memory_Data_scoreboard[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Issue_Memory_Data_scoreboard[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Memory_Data_L1[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Memory_Data_L1[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Memory_Data_L2[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Memory_Data_L2[sm_id] << " ";;
    }
    file << std::endl;
    file << "num_Execute_Memory_Data_Main_Memory[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << num_Execute_Memory_Data_Main_Memory[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;
    file << "SP_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << SP_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "SFU_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << SFU_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "INT_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << INT_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "DP_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << DP_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "TENSOR_CORE_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << TENSOR_CORE_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "LDST_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << LDST_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "SPEC_UNIT_1_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << SPEC_UNIT_1_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "SPEC_UNIT_2_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << SPEC_UNIT_2_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "SPEC_UNIT_3_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << SPEC_UNIT_3_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << "Other_UNIT_execute_clks_sum[]: ";
    for (unsigned sm_id = 0; sm_id < m_num_sm; sm_id++) {
      file << Other_UNIT_execute_clks_sum[sm_id] << " ";;
    }
    file << std::endl;
    file << std::endl;


    file.close();
  }
}


bool operator<(const curr_instn_id_per_warp_entry& lhs, const curr_instn_id_per_warp_entry& rhs) {
  if (lhs.kid != rhs.kid) {
    return lhs.kid < rhs.kid;
  } else {
    if (lhs.block_id != rhs.block_id) {
      return lhs.block_id < rhs.block_id;
    } else {
      return lhs.warp_id < rhs.warp_id;
    }
  }
}

PrivateSM::PrivateSM(const unsigned smid, trace_parser* tracer, hw_config* hw_cfg){

  m_smid = smid;
  m_cycle = 0;
  active = true;
  m_active_warps = 0;
  max_warps_init = 0;

  num_warp_instns_executed = 0;

  active_cycles = 0;
  active_warps_id_size_sum = 0;

  m_hw_cfg = hw_cfg;

  this->tracer = tracer;
  issuecfg = this->tracer->get_issuecfg();
  appcfg = this->tracer->get_appcfg();  
  instncfg = this->tracer->get_instncfg();

  // (kernel_id, block_id) pair that are allocated to this SM
  kernel_block_pair = issuecfg->get_kernel_block_by_smid(m_smid);

  // number of warps per kernel that are allocated to this SM
  m_num_warps_per_sm.resize(appcfg->get_kernels_num(), 0);

  m_warp_active_status.reserve(kernel_block_pair.size());
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned block_id = it->second;
    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    m_warp_active_status.push_back(std::vector<bool>(_warps_per_block, false));
  }
  m_thread_block_has_executed_status.reserve(kernel_block_pair.size());
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    m_thread_block_has_executed_status.push_back(false);
  }

  /* m_num_warps_per_sm[i] stores the i-th kernel's warps number that are 
   * allocated to this SM. m_num_warps_per_sm's first dim is the total num 
   * of kernels that are executed on the GPU, and its second dim is the 
   * number of warps that blongs to the current SM. for example, 
   *   -trace_issued_sm_id_0 5,0,(1,80),(1,160),(1,0),(2,0),(2,80)
   * and the 1st kernel has 3 warps per thread block, the 2nd kernel has 2 
   * warps per thread block, then:
   *   m_num_warps_per_sm[0] = 9, m_num_warps_per_sm[1] = 6.
   */
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    m_num_warps_per_sm[kid] += _warps_per_block;
  }

  // std::map<std::pair<unsigned, unsigned>> kernel_id_block_id_last_fetch_wid;
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned block_id = it->second;
    kernel_id_block_id_last_fetch_wid[{kid, block_id}] = 0;
  }

  m_num_blocks_per_kernel.resize(appcfg->get_kernels_num(), 0);

  /* curr_instn_id_per_warp stores the current instn id of each warp */
  for (auto it = kernel_block_pair.begin(); it != kernel_block_pair.end(); it++) {
    unsigned kid = it->first - 1;
    unsigned block_id = it->second;

    m_num_blocks_per_kernel[kid] += 1;
    // std::cout << "kid: " << kid << " m_num_blocks_per_kernel[kid]: " << m_num_blocks_per_kernel[kid] << std::endl;

    unsigned _warps_per_block = appcfg->get_num_warp_per_block(kid);
    for (unsigned _i = 0; _i < _warps_per_block; _i++) {
      // std::cout << "D: Initial curr_instn_id_per_warp: " << kid << " " << block_id << " " << _i << " to 0." << std::endl;
      curr_instn_id_per_warp_entry _entry = curr_instn_id_per_warp_entry(kid, block_id, _i);
      curr_instn_id_per_warp[_entry] = 0;
    }
  }

  //traverse m_num_warps_per_sm
  if (_DEBUG_LOG_)
    for (auto it = m_num_warps_per_sm.begin(); 
              it != m_num_warps_per_sm.end(); it++){
      std::cout << "m_num_warps_per_sm: " << *it << std::endl;
    }

  /* last_fetch_warp_id: key - kid : value - wid */
  for (auto i = 0; i< m_num_warps_per_sm.size(); i++){
    last_fetch_warp_id[i] = 0;
  }

  // sum of std::vector<unsigned> m_num_warps_per_sm
  all_warps_num = std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.end(), 0);
  if (_DEBUG_LOG_)
    std::cout << "Initial Ibuffer size: " << all_warps_num << std::endl;
  // when accessing the ibuffer, the index is:
  //     global_all_kernels_warp_id = gwarp_id + sum_{kid = 0,1,...,kernel_id-1}(m_num_warps_per_sm[kid])
  m_ibuffer = new IBuffer(m_smid, all_warps_num);
  // last_fetch_warp_id = 0;
  distance_last_fetch_kid = 0;
  last_issue_sched_id = 0;
  
  m_scoreboard = new Scoreboard(m_smid, all_warps_num);

  m_inst_fetch_buffer = new inst_fetch_buffer_entry();
  m_inst_fetch_buffer_copy = new inst_fetch_buffer_entry();               // yangjianchao16 add 20240131
  
  // std::cout << "D: all_warps_num: " << all_warps_num << std::endl;

  num_banks = hw_cfg->get_num_reg_banks();
  bank_warp_shift = hw_cfg->get_bank_warp_shift();
  num_scheds = hw_cfg->get_num_sched_per_sm();
  sub_core_model = hw_cfg->get_sub_core_model();
  banks_per_sched = (unsigned)(num_banks / num_scheds);
  inst_fetch_throughput = hw_cfg->get_inst_fetch_throughput();
  reg_file_port_throughput = hw_cfg->get_reg_file_port_throughput();

  warps_per_sched = (unsigned)(all_warps_num / num_scheds);

  // last_issue_warp_ids.resize(num_scheds, 0);

  last_issue_block_index_per_sched.resize(num_scheds, 0);

  m_reg_bank_allocator = new RegisterBankAllocator(m_smid, 
                                                   num_banks, 
                                                   num_scheds, 
                                                   bank_warp_shift, 
                                                   banks_per_sched);

  parse_blocks_per_kernel();

  total_pipeline_stages =
      N_PIPELINE_STAGES + hw_cfg->get_specialized_unit_size() * 2;
  m_pipeline_reg.reserve(total_pipeline_stages);
  
  if (_DEBUG_LOG_)
    std::cout << "total_pipeline_stages: " 
              << total_pipeline_stages << std::endl;

  for (unsigned j = 0; j < N_PIPELINE_STAGES; j++) {
    if (_DEBUG_LOG_)
      std::cout << ";;;pipeline_width index " << j << " : " 
                << hw_cfg->get_pipe_widths(
                  static_cast<pipeline_stage_name_t>(j)
                ) << " "
                << hw_cfg->get_pipeline_stage_name_decode(
                  static_cast<pipeline_stage_name_t>(j)
                ) << std::endl;
    m_pipeline_reg.push_back(
      register_set(
        hw_cfg->get_pipe_widths(static_cast<pipeline_stage_name_t>(j)), 
        std::string(
          hw_cfg->get_pipeline_stage_name_decode(
          static_cast<pipeline_stage_name_t>(j))
        ), 
        hw_cfg
      )
    );
  }

  for (unsigned j = 0; j < hw_cfg->get_specialized_unit_size(); j++) {
    if (_DEBUG_LOG_)
      std::cout << ";;;pipeline_width index " 
                << j + N_PIPELINE_STAGES << " : " 
                << hw_cfg->get_pipe_widths_ID_OC_spec_unit(j) << " "
                << std::string("ID_OC_") + hw_cfg->get_m_specialized_unit_name(j)
                << std::endl;
    m_pipeline_reg.push_back(
      register_set(
        hw_cfg->get_pipe_widths_ID_OC_spec_unit(j),
        std::string(
          std::string("ID_OC_") + hw_cfg->get_m_specialized_unit_name(j)
        ), 
        hw_cfg
      )
    );
    // m_config->m_specialized_unit[j].ID_OC_SPEC_ID = m_pipeline_reg.size() - 1;
    m_specilized_dispatch_reg.push_back(
        &m_pipeline_reg[m_pipeline_reg.size() - 1]);
  }

  for (unsigned j = 0; j < hw_cfg->get_specialized_unit_size(); j++) {
    if (_DEBUG_LOG_)
      std::cout << ";;;pipeline_width index " 
                << j + N_PIPELINE_STAGES + hw_cfg->get_specialized_unit_size() << " : " 
                << hw_cfg->get_pipe_widths_OC_EX_spec_unit(j) << " "
                << std::string("OC_EX_") + hw_cfg->get_m_specialized_unit_name(j)
                << std::endl;
    m_pipeline_reg.push_back(
      register_set(
        hw_cfg->get_pipe_widths_OC_EX_spec_unit(j),
        std::string(
          std::string("OC_EX_") + hw_cfg->get_m_specialized_unit_name(j)
        ), 
        hw_cfg
      )
    );
    // m_config->m_specialized_unit[j].OC_EX_SPEC_ID = m_pipeline_reg.size() - 1;
  }

  m_sp_out = &m_pipeline_reg[ID_OC_SP];
  m_dp_out = &m_pipeline_reg[ID_OC_DP];
  m_sfu_out = &m_pipeline_reg[ID_OC_SFU];
  m_int_out = &m_pipeline_reg[ID_OC_INT];
  m_tensor_core_out = &m_pipeline_reg[ID_OC_TENSOR_CORE];
  // m_spec_cores_out = m_specilized_dispatch_reg;
  for (unsigned j = 0; j < m_specilized_dispatch_reg.size(); j++) {
    m_spec_cores_out.push_back(m_specilized_dispatch_reg[j]);
  }
  m_mem_out = &m_pipeline_reg[ID_OC_MEM];

  // op collector configuration
  enum { SP_CUS, DP_CUS, SFU_CUS, TENSOR_CORE_CUS, INT_CUS, MEM_CUS, GEN_CUS };
  
  opndcoll_rfu_t::port_vector_t in_ports;
  opndcoll_rfu_t::port_vector_t out_ports;
  opndcoll_rfu_t::uint_vector_t cu_sets;

  m_operand_collector = new opndcoll_rfu_t(m_hw_cfg, m_reg_bank_allocator, this->tracer);


  // configure generic collectors
  m_operand_collector->add_cu_set(
      GEN_CUS, hw_cfg->get_operand_collector_num_units_gen(),
      hw_cfg->get_operand_collector_num_out_ports_gen());

  for (unsigned i = 0; i < hw_cfg->get_operand_collector_num_in_ports_gen();
       i++) {
    in_ports.push_back(&m_pipeline_reg[ID_OC_SP]);
    in_ports.push_back(&m_pipeline_reg[ID_OC_SFU]);
    in_ports.push_back(&m_pipeline_reg[ID_OC_MEM]);
    out_ports.push_back(&m_pipeline_reg[OC_EX_SP]);
    out_ports.push_back(&m_pipeline_reg[OC_EX_SFU]);
    out_ports.push_back(&m_pipeline_reg[OC_EX_MEM]);
    if (1) {
      in_ports.push_back(&m_pipeline_reg[ID_OC_TENSOR_CORE]);
      out_ports.push_back(&m_pipeline_reg[OC_EX_TENSOR_CORE]);
    }
    if (hw_cfg->get_num_dp_units() > 0) {
      in_ports.push_back(&m_pipeline_reg[ID_OC_DP]);
      out_ports.push_back(&m_pipeline_reg[OC_EX_DP]);
    }
    if (hw_cfg->get_num_int_units() > 0) {
      in_ports.push_back(&m_pipeline_reg[ID_OC_INT]);
      out_ports.push_back(&m_pipeline_reg[OC_EX_INT]);
    }
    if (hw_cfg->get_specialized_unit_size() > 0) {
      for (unsigned j = 0; j < hw_cfg->get_specialized_unit_size(); ++j) {
        in_ports.push_back(
            &m_pipeline_reg[N_PIPELINE_STAGES + 0 + j]);
        out_ports.push_back(
            &m_pipeline_reg[N_PIPELINE_STAGES + 3 + j]);
      }
    }
    cu_sets.push_back((unsigned)GEN_CUS);
    m_operand_collector->add_port(in_ports, out_ports, cu_sets);
    in_ports.clear(), out_ports.clear(), cu_sets.clear();
  }

  
  m_operand_collector->init(m_hw_cfg, m_reg_bank_allocator, this->tracer);
  
  // m_fu

  for (unsigned k = 0; k < m_hw_cfg->get_num_sp_units(); k++) {
    m_fu.push_back(new sp_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_SP);
    m_issue_port.push_back(OC_EX_SP);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_sfu_units(); k++) {
    m_fu.push_back(new sfu(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_SFU);
    m_issue_port.push_back(OC_EX_SFU);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_int_units(); k++) {
    m_fu.push_back(new int_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_INT);
    m_issue_port.push_back(OC_EX_INT);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_dp_units(); k++) {
    m_fu.push_back(new dp_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_DP);
    m_issue_port.push_back(OC_EX_DP);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_tensor_core_units(); k++) {
    m_fu.push_back(new tensor_core(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_TENSOR_CORE);
    m_issue_port.push_back(OC_EX_TENSOR_CORE);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_num_mem_units(); k++) {
    m_fu.push_back(new mem_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer));
    m_dispatch_port.push_back(ID_OC_MEM);
    m_issue_port.push_back(OC_EX_MEM);
  }

  for (unsigned k = 0; k < m_hw_cfg->get_specialized_unit_size(); k++) {
    m_fu.push_back(new specialized_unit(&m_pipeline_reg[EX_WB], k, m_hw_cfg, this->tracer, k));
    m_dispatch_port.push_back(N_PIPELINE_STAGES + 0 + k);
    m_issue_port.push_back(N_PIPELINE_STAGES + 3 + k);
  }

  num_result_bus = hw_cfg->get_pipe_widths(static_cast<pipeline_stage_name_t>(EX_WB));
  for (unsigned _ = 0; _ < num_result_bus; _++) {
    m_result_bus.push_back(new std::bitset<MAX_ALU_LATENCY>());
  }
}

/* Here, wid is local wid. */
int PrivateSM::register_bank(int regnum, int wid, unsigned sched_id) {
  int bank = regnum;
  //warp的bank偏移。
  if (bank_warp_shift) bank += wid;
  //在subcore模式下，每个warp调度器在寄存器集合中有一个具体的寄存器可供使用，这个寄
  //存器由调度器的m_id索引。m_num_banks_per_sched的定义为：
  //    num_banks / shader->get_config()->gpgpu_num_sched_per_core;
  //在V100配置中，共有4个warp调度器，0号warp调度器可用的bank为0-3，1号warp调度器可
  //用的bank为4-7，2号warp调度器可用的bank为8-11，3号warp调度器可用的bank为12-15。
  if (sub_core_model) {
    unsigned bank_num = (bank % banks_per_sched) + (sched_id * banks_per_sched);
    assert(bank_num < num_banks);
    return bank_num;
  } else
    return bank % num_banks;
}

bool PrivateSM::check_active(){
  return false; // TODO
}

PrivateSM::~PrivateSM() {
  delete m_ibuffer;
  delete m_inst_fetch_buffer;
  delete m_inst_fetch_buffer_copy;               // yangjianchao16 add 20240131
  delete m_scoreboard;
  delete m_reg_bank_allocator;
  delete m_operand_collector;
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

  for (auto ptr : m_fu) {
    delete ptr;
  }

  for (auto ptr : m_result_bus) {
    delete ptr;
  }
}

unsigned PrivateSM::get_num_warps_per_sm(unsigned kernel_id) { 
  return m_num_warps_per_sm[kernel_id]; 
}

void PrivateSM::parse_blocks_per_kernel() {
  for (unsigned i = 0; i < kernel_block_pair.size(); i++) {
    if (blocks_per_kernel.find(kernel_block_pair[i].first) == blocks_per_kernel.end()) {
      blocks_per_kernel[kernel_block_pair[i].first] = 
        std::vector<unsigned>(1, kernel_block_pair[i].second);
    } else {
      blocks_per_kernel[kernel_block_pair[i].first].push_back(kernel_block_pair[i].second);
    }
  }
}

std::vector<unsigned> PrivateSM::get_blocks_per_kernel(unsigned kernel_id) {
  return blocks_per_kernel[kernel_id];
}

std::map<unsigned, std::vector<unsigned>>* PrivateSM::get_blocks_per_kernel() {
  return &blocks_per_kernel;
}

unsigned PrivateSM::get_inst_fetch_throughput() { return inst_fetch_throughput; }
unsigned PrivateSM::get_reg_file_port_throughput() { return reg_file_port_throughput; }

void PrivateSM::issue_warp(register_set &pipe_reg_set, 
                           ibuffer_entry entry, 
                           unsigned sch_id) {
  // print entry
  if (_DEBUG_LOG_) {
    std::cout << "  issue_warp: " << std::endl;
    std::cout << "    pc: " << entry.pc << ", wid: " << entry.wid 
              << ", kid: " << entry.kid << ", uid: " << entry.uid << std::endl;
  }
  // inst_fetch_buffer_entry **pipe_reg =
  //   pipe_reg_set.get_free(m_hw_cfg->get_sub_core_model(), sch_id);
  // assert(pipe_reg);

  inst_fetch_buffer_entry* tmp = new inst_fetch_buffer_entry();
  tmp->kid = entry.kid;
  tmp->pc = entry.pc;
  tmp->uid = entry.uid;
  tmp->wid = entry.wid;
  tmp->m_valid = true;

  if (_DEBUG_LOG_) {
    std::cout << "  issue_warp: " << std::endl;
    std::cout << "    pc: " << tmp->pc << ", wid: " << tmp->wid 
              << ", kid: " << tmp->kid << ", uid: " << tmp->uid << std::endl;
  }

  pipe_reg_set.move_in(m_hw_cfg->get_sub_core_model(), sch_id, tmp);

  delete tmp;
  tmp = nullptr;  // Optional: set tmp to nullptr to avoid dangling pointer

  // print reigster set
  if (_DEBUG_LOG_) {
    std::cout << "  Now register set: " << std::endl;
    pipe_reg_set.print();
  }
  // Scoreboard: TODO

}

void insert_into_active_warps_id(std::vector<unsigned>* active_warps_id, unsigned wid) {
  // find if wid is in active_warps_id, if yes pass, if no insert into
  // how to use: insert_into_active_warps_id(&active_warps_id, wid);
  if (std::find(active_warps_id->begin(), active_warps_id->end(), wid) == active_warps_id->end()) {
    active_warps_id->push_back(wid);
  }
} 

void PrivateSM::run(unsigned KERNEL_EVALUATION, unsigned MEM_ACCESS_LATENCY, stat_collector* stat_coll){
  // unsigned MEM_ACCESS_LATENCY = 5;
  m_cycle++;

  // if (m_cycle<48515)
  
  if (_CALIBRATION_LOG_) {
    std::cout << "#: m_cycle: " << m_cycle << std::endl;
  }

  if (m_cycle > 2000000) {std::cout << "ECIT BY CYCLE!!!" << std::endl; exit(0);}

  bool active_during_this_cycle = false;

  std::vector<unsigned> active_warps_id;

  bool flag_Issue_Compute_Structural_out_has_no_free_slot = false;
  bool flag_Issue_Memory_Structural_out_has_no_free_slot = false;
  bool flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = false;
  bool flag_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory = false;
  bool flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = false;
  bool flag_Execute_Memory_Structural_result_bus_has_no_slot_for_latency = false;
  bool flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = false;
  bool flag_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty = false;
  bool flag_Writeback_Compute_Structural_bank_of_reg_is_not_idle = false;
  bool flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle = false;
  bool flag_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated = false;
  bool flag_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated = false;
  bool flag_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu = false;
  bool flag_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu = false;
  bool flag_Execute_Memory_Structural_icnt_injection_buffer_is_full = false;
  bool flag_Issue_Compute_Data_scoreboard = false;
  bool flag_Issue_Memory_Data_scoreboard = false;
  bool flag_Execute_Memory_Data_L1 = false;
  bool flag_Execute_Memory_Data_L2 = false;
  bool flag_Execute_Memory_Data_Main_Memory = false;

  // std::cout << "# cycle: " << m_cycle << std::endl;

  // if (m_cycle >= 6261) {
  //   active = false;
  // }

  // for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
  //           it_kernel_block_pair != kernel_block_pair.end(); 
  //           it_kernel_block_pair++) {
START_TIMER(0);

    /* Variables that depend on it_kernel_block_pair:
     *   kid, block_id, warps_per_block, gwarp_id_start, gwarp_id_end
     * The following code is to get these variables:
     *   unsigned index = std::distance(kernel_block_pair.begin(), it_kernel_block_pair);
     *   unsigned kid = it_kernel_block_pair->first - 1;
     *   unsigned block_id = it_kernel_block_pair->second;
     *   unsigned warps_per_block = appcfg->get_num_warp_per_block(kid);
     *   unsigned gwarp_id_start = warps_per_block * block_id;
     *   unsigned gwarp_id_end = gwarp_id_start + warps_per_block - 1;
     */

    /* -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
     * Here, kernel_block_pair is (1,80), (1,160), (1,0), (2,0), (3,0), (4,0)
     * for it_kernel_block_pair : kernel_block_pair:
     *   index           =   0,   1,   2,   3,   4,   5
     *   kid             =   0,   0,   0,   1,   2,   3
     *   block_id        =  80, 160,   0,   0,   0,   0
     *   warps_per_block =   2,   2,   2,   2,   2,   2
     *   gwarp_id_start  =  80, 160,   0,   0,   0,   0
     *   gwarp_id_end    =  81, 161,   1,   1,   1,   1
     */
  
    // TODO: here only first kid-bid pair is considered
    auto it_kernel_block_pair = kernel_block_pair.begin(); 
    
    unsigned kid = it_kernel_block_pair->first - 1;
    
    unsigned block_id = it_kernel_block_pair->second;
    unsigned warps_per_block = appcfg->get_num_warp_per_block(kid);
    /* Calculate gwarp_id:
     *     gwarp_id_start = block_id * warps_per_block
     *     gwarp_id_end   = (block_id + 1) * warps_per_block */
    // std::cout << "D: warps_per_block, block_id: " << warps_per_block << " " << block_id << std::endl;
    unsigned gwarp_id_start = warps_per_block * block_id;
    unsigned gwarp_id_end = gwarp_id_start + warps_per_block - 1;

    // std::cout << "$ SM-" << m_smid << " Kernel-" << kid << " Block-" << block_id 
    //           << ", gwarp_id_start: " << gwarp_id_start << ", gwarp_id_end: " 
    //           << gwarp_id_end << std::endl;
    if (m_cycle == 1) {
      for (auto it_kernel_block_pair_1 = kernel_block_pair.begin(); 
                it_kernel_block_pair_1 != kernel_block_pair.end(); // TODO: here only first kernel is considered
                it_kernel_block_pair_1++) {
        if (it_kernel_block_pair_1->first - 1 != KERNEL_EVALUATION) {
          unsigned _index_ = std::distance(kernel_block_pair.begin(), it_kernel_block_pair_1);
          m_thread_block_has_executed_status[_index_] = true;
        }
      }
    }
    
    for (auto it_kernel_block_pair_2 = kernel_block_pair.begin(); 
              it_kernel_block_pair_2 != kernel_block_pair.end(); // TODO: here only first kernel is considered
              it_kernel_block_pair_2++) {
      if (it_kernel_block_pair_2->first - 1 != KERNEL_EVALUATION) continue;
      
      /* -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
       * Here, kernel_block_pair is (1,80), (1,160), (1,0), (2,0), (3,0), (4,0)
       * for it_kernel_block_pair_2 : kernel_block_pair:
       *   index           =   0,   1,   2,   3,   4,   5
       *   kid             =   0,   0,   0,   1,   2,   3
       *   block_id        =  80, 160,   0,   0,   0,   0
       *   warps_per_block =   2,   2,   2,   2,   2,   2
       *   gwarp_id_start  =  80, 160,   0,   0,   0,   0
       *   gwarp_id_end    =  81, 161,   1,   1,   1,   1
       */
      unsigned _index_ = std::distance(kernel_block_pair.begin(), it_kernel_block_pair_2);

      if (m_thread_block_has_executed_status[_index_] == true) continue;

      unsigned _kid_ = it_kernel_block_pair_2->first - 1;
      unsigned _block_id_ = it_kernel_block_pair_2->second;
      unsigned _warps_per_block_ = appcfg->get_num_warp_per_block(_kid_);
      
      if (get_num_m_warp_active_status() + _warps_per_block_ <= m_hw_cfg->get_max_warps_per_sm()) {
        // std::cout << "    before active_warps: " << get_num_m_warp_active_status() << std::endl;
        for (unsigned _wid_ = 0; _wid_ < _warps_per_block_; _wid_++) {
          /* m_warp_active_status[
          *                      0 ~ kernel_block_pair.size()
          *                     ]
          *                     [
          *                      0 ~ warps_per_block_of_it_kernel_block_pair_2
          *                     ] 
          * In fact, m_warp_active_status's 1st dim is the index of (kid, block_id) pairs
          * that are issued to the current SM, and the 2nd dim is the number of warps in 
          * the thread block - (kid, block_id), and equals to the warp_per_block of the 
          * kernel - kid. */
          if (/*m_cycle == 1 &&*/ m_warp_active_status[_index_][_wid_] == false) {
            m_warp_active_status[_index_][_wid_] = true;
            m_thread_block_has_executed_status[_index_] = true;
            m_active_warps++;
            max_warps_init++;
            if (_DEBUG_LOG_)
              std::cout << "  **First time to activate warp (_index_, wid): (" 
                        << _index_ << ", " << _wid_ << ")" 
                        << " <=> (kid, block_id, wid): (" 
                        << _kid_ << "," 
                        << _block_id_ << "," 
                        << _wid_ << ")" << std::endl;
          }
        }
        // std::cout << "      after active_warps: " << get_num_m_warp_active_status() << std::endl;
      }
      
    }
    // std::cout << "@@@1";


STOP_AND_REPORT_TIMER_rank(0);

START_TIMER(1);
    /*
    bool flag_Issue_Compute_Structural_out_has_no_free_slot = true;                                             V
    bool flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;                        V
    bool flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;                             V
    bool flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;                              V
    bool flag_Writeback_Compute_Structural_bank_of_reg_is_not_idle = true;                                      V
    bool flag_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated = true;
    bool flag_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu = true;       V
    
    
    bool flag_Issue_Compute_Data_scoreboard = true;                                                             V

    bool flag_Issue_Memory_Data_scoreboard = true;                                                              V
    bool flag_Execute_Memory_Data_L1 = true;
    bool flag_Execute_Memory_Data_L2 = true;
    bool flag_Execute_Memory_Data_Main_Memory = true;

    bool flag_Issue_Memory_Structural_out_has_no_free_slot = true;                                              V
    bool flag_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory = true;                          V
    bool flag_Execute_Memory_Structural_result_bus_has_no_slot_for_latency = true;                              X
    bool flag_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty = true;                               V
    bool flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle = true;                                       V
    bool flag_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated = true;
    bool flag_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu = true;        V
    bool flag_Execute_Memory_Structural_icnt_injection_buffer_is_full = true;                                   V
    */
    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                              Write back to register banks.                             ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    inst_fetch_buffer_entry **preg = m_pipeline_reg[EX_WB].get_ready();
    inst_fetch_buffer_entry *pipe_reg = (preg == NULL) ? NULL : *preg;
    std::vector<inst_fetch_buffer_entry> except_regs;
    while (preg and pipe_reg->m_valid) {
      /*
      * Right now, the writeback stage drains all waiting instructions
      * assuming there are enough ports in the register file or the
      * conflicts are resolved at issue.
      */
      /*
      * The operand collector writeback can generally generate a stall
      * However, here, the pipelines should be un-stallable. This is
      * guaranteed because this is the first time the writeback function
      * is called after the operand collector's step function, which
      * resets the allocations. There is one case which could result in
      * the writeback function returning false (stall), which is when
      * an instruction tries to modify two registers (GPR and predicate)
      * To handle this case, we ignore the return value (thus allowing
      * no stalling).
      */

      // m_operand_collector.writeback(*pipe_reg);
      unsigned _kid = pipe_reg->kid;
      unsigned _pc = pipe_reg->pc;
      /* MOST IMPORTANT: Here, _wid is the global warp id in the whole kernel, for example,
       *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
       * and there are 3 warps per block in the 1st kernel, then:
       * wid during the transfer in the pipeline is 
       *   80 * 3 = 240     or 
       *   80 * 3 + 1 = 241 or
       *   80 * 3 + 2 = 242.
       */
      unsigned _wid = pipe_reg->wid;
      unsigned _uid = pipe_reg->uid;

      unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
      /* We have noticed that _wid is the global warp id in the whole kernel, So, _block_id
       * here is just the block id of this instn. With the above supposition, _block_id is
       * 80. */
      unsigned _block_id = (unsigned)(_wid / _warps_per_block);
      /* _gwarp_id_start is the global warp id of the first warp in the block, for example,
       *   80 * 3 = 240. */
      unsigned _gwarp_id_start = _warps_per_block * _block_id;

      auto _compute_instn = tracer->get_one_kernel_one_warp_one_instn(_kid, _wid, _uid);
      auto _trace_warp_inst = 
        _compute_instn->trace_warp_inst;
      unsigned dst_reg_num = _trace_warp_inst.get_outcount();

      std::vector<int> need_write_back_regs_num;

      for (unsigned i = 0; i < dst_reg_num; i++){
        int dst_reg_id = _trace_warp_inst.get_arch_reg_dst(i);
        // std::cout << "    dst_reg_id[" << i << "]: " << dst_reg_id << std::endl;
        // Calculate the scheduler id of the dst reg id
        if (dst_reg_id >= 0) {
          auto local_wid = (unsigned)(_wid % _warps_per_block);
          auto sched_id = (unsigned)(local_wid % num_scheds);
          // Calculate the bank id of the dst reg id
          auto bank_id = register_bank(dst_reg_id, local_wid, sched_id);
          if (_DEBUG_LOG_)
            std::cout << "    kid, wid, uid, dst_reg_num, "
                         "dst_reg_id, bank_id, sched_id: " 
                      << _kid << ", " 
                      << _wid << " (local:" 
                      << local_wid << "), "
                      << _uid << ", " 
                      << dst_reg_num << ", " 
                      << dst_reg_id << ", " 
                      << bank_id << ", " 
                      << sched_id << std::endl;
          // Now we set the bank_id of the dst reg id to be on_write (for dst reg id, it is on_wite).
          if (m_reg_bank_allocator->getBankState(bank_id) == FREE) {
            // active_during_this_cycle = true;

            m_reg_bank_allocator->setBankState(bank_id, ON_WRITING);
            
            _trace_warp_inst.set_arch_reg_dst(i, -1);
            if (_DEBUG_LOG_)
              std::cout << "    setBankState(" << bank_id 
                        << ", WRITING)   dst_reg_id : " 
                        << _trace_warp_inst.get_arch_reg_dst(i) 
                        << std::endl;

            insert_into_active_warps_id(&active_warps_id, _wid); // here _wid id global
          } else {
            if (_DEBUG_LOG_)
              std::cout << "    cannot setBankState "
                           "bank_id-" << bank_id << std::endl;
            flag_Writeback_Compute_Structural_bank_of_reg_is_not_idle = true;
          }
        }
      }

      bool all_write_back = true;
      for (unsigned i = 0; i < dst_reg_num; i++){
        if (_trace_warp_inst.get_arch_reg_dst(i) != -1) {
          all_write_back = false;
          break;
        }
      }

      // if all dst reg ids are written back, then we can release the bank states and
      // remove the instn from writeback_stage_instns
      if (all_write_back) {
        // std::cout << "    all_write_back" << std::endl;
        if (_CALIBRATION_LOG_) {
          std::cout << "    Write back: ("
                    << _kid << ", "
                    << _wid << ", "
                    << _uid << ", "
                    << _pc << ")" << std::endl;
        }
        set_clk_record<5>(_kid, _wid, _uid, m_cycle);
        // inc instns executed
        num_warp_instns_executed++;

        // std::cout << "  **Write back instn "
        //                "(pc, gwid, kid, "
        //                "fetch_instn_id): (" 
        //             << std::hex 
        //             << _pc << ", " << std::dec 
        //             << _wid << ", " 
        //             << _kid << ", " 
        //             << _uid << ")" << std::endl;

        if (_DEBUG_LOG_) {
          std::cout << "  **Write back instn "
                       "(pc, gwid, kid, "
                       "fetch_instn_id): (" 
                    << std::hex 
                    << _pc << ", " << std::dec 
                    << _wid << ", " 
                    << _kid << ", " 
                    << _uid << ")" << std::endl;

          std::cout << "    Write back instn (kid, gwid, "
                       "fetch_instn_id): (" << _kid << ", "
                    << _wid << ", "
                    << _uid << ")" << std::endl;
        }

        // remove the instn from writeback_stage_instns
        pipe_reg->m_valid = false;

        /* If the instn string of (_kid, _wid, _pc) is "EXIT", should deactivate this warp. */
        // std::cout << _kid << " " << _wid << " " << tracer->get_one_kernel_one_warp_instn_count(_kid, _wid) << " " << _uid + 1 << std::endl;
        if (_trace_warp_inst.get_opcode() == OP_EXIT &&
            tracer->get_one_kernel_one_warp_instn_count(_kid, _wid) == _uid + 1) {
          /* m_warp_active_status[
           *                      0 ~ kernel_block_pair.size()
           *                     ]
           *                     [
           *                      0 ~ warps_per_block_of_it_kernel_block_pair
           *                     ] 
           * In fact, m_warp_active_status's 1st dim is the index of (kid, block_id) pairs
           * that are issued to the current SM, and the 2nd dim is the number of warps in 
           * the thread block - (kid, block_id), and equals to the warp_per_block of the 
           * kernel - kid. */
          /* We now need to calculate the index of kernel - block pair (_kid, _block_id). */
          unsigned _index = 0;
          for (auto _it_kernel_block_pair = kernel_block_pair.begin(); 
                    _it_kernel_block_pair != kernel_block_pair.end();
                    _it_kernel_block_pair++) {
            if (_it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
            if (_it_kernel_block_pair->first - 1 == _kid && 
                _it_kernel_block_pair->second == _block_id) {
              _index = std::distance(kernel_block_pair.begin(), _it_kernel_block_pair);
              break;
            }
          }
          /* MOST IMPORTANT: Here, _wid is the global warp id in the whole kernel, for example,
           *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
           * and there are 3 warps per block in the 1st kernel, then:
           * wid during the transfer in the pipeline is 
           *   80 * 3 = 240     or 
           *   80 * 3 + 1 = 241 or
           *   80 * 3 + 2 = 242.
           * _gwarp_id_start is the global warp id of the first warp in the block, for example,
           * 80 * 3 = 240.
           */
          
          m_warp_active_status[_index][_wid - _gwarp_id_start] = false;
          m_active_warps--;
          insert_into_active_warps_id(&active_warps_id, _wid);
          if (_DEBUG_LOG_)
            std::cout << "  **ENDCYCLLE: kid,block_id,wid,cycle:" 
                      << _kid << " " 
                      << _block_id << " " 
                      << _wid << " "
                      << m_cycle << std::endl;
          if (_DEBUG_LOG_)
            std::cout << "  **Deactivate warp (_index_, wid): (" 
                      << _index << ", " << _wid - _gwarp_id_start << ")" 
                      << " <=> (kid, block_id, wid): (" 
                      << _kid << "," 
                      << _block_id << "," 
                      << _wid - _gwarp_id_start << ")" << std::endl;
        }

      } else {
        except_regs.push_back(*pipe_reg);
        if (_DEBUG_LOG_)
          std::cout << "  **Try to but fail to Write back "
                       "instn (pc, gwid, kid, fetch_instn_id): (" << std::hex 
                    << _pc << ", " << std::dec 
                    << _wid << ", " 
                    << _kid << ", " 
                    << _uid << ")" << std::endl;
        insert_into_active_warps_id(&active_warps_id, _wid);
        // exit(0); /// NEED TO DELETE
        
      }

      /* m_num_warps_per_sm's first dim is the total num of kernels that are executed on
       * the GPU, and its second dim is the number of warps that blongs to the current SM.
       * for example, 
       *   -trace_issued_sm_id_0 5,0,(1,80),(1,160),(1,0),(2,0),(2,80)
       *   and the 1st kernel has 3 warps per thread block, the 2nd kernel has 2 warps per
       *   thread block, then:
       *     m_num_warps_per_sm[0] = 9, m_num_warps_per_sm[1] = 6.
       * So, the following code is to get the global warp id of the total kernels 
       * (global_all_kernels_warp_id). For example, we have 2 kernels in the whole SM, and 
       * the 1st kernel has 3 warps per thread block, then the 2nd kernel has 2 warps per 
       * thread block, thus the global_all_kernels_warp_id of the 2st kernel's 2nd warp is 
       * 9 + 2 = 11.
       * 
       * And with the above assumption, the IBuffer's slots are allocated in the following:
       *  0 slot => warp 0 from kernel 0
       *  1 slot => warp 1 from kernel 0
       *  ......
       *  8 slot => warp 8 from kernel 0
       *  9 slot => warp 0 from kernel 1
       * 10 slot => warp 1 from kernel 1
       * ......
       * 14 slot => warp 5 from kernel 1
       */

      /* Count the entries in the kernel_block_pair that are smaller than _block_id with
       * the same _kid. */
      unsigned _kid_block_id_count = 0;
      for (auto _it_kernel_block_pair = kernel_block_pair.begin(); 
                _it_kernel_block_pair != kernel_block_pair.end();
                _it_kernel_block_pair++) {
        if (_it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
        if (_it_kernel_block_pair->first - 1 == _kid) {
          if (_it_kernel_block_pair->second < _block_id) {
            _kid_block_id_count++;
          }
        }
      }

      /* With the above assumption, for _wid from (2,80), global_all_kernels_warp_id:
       * slot 12/13/14 in the IBuffer will be filled.  */
      auto global_all_kernels_warp_id = 
        (unsigned)(_wid % _warps_per_block) + 
        _kid_block_id_count * _warps_per_block +
        std::accumulate(m_num_warps_per_sm.begin(), 
                        m_num_warps_per_sm.begin() + _kid, 0);

      if (all_write_back) {
        

        if (_DEBUG_LOG_)
          m_scoreboard->printContents();

        _inst_trace_t* tmp_inst_trace = _compute_instn->inst_trace;
        for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
          need_write_back_regs_num.push_back(tmp_inst_trace->reg_src[i]);
          // std::cout << "        R" << tmp_inst_trace->reg_src[i] << std::endl;
        }
        for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
          if (tmp_inst_trace->reg_dest_is_pred[i]) {
            need_write_back_regs_num.push_back(tmp_inst_trace->reg_dest[i] + PRED_NUM_OFFSET);
          }
          else {
            need_write_back_regs_num.push_back(tmp_inst_trace->reg_dest[i]);
          }
        }
        auto pred = _trace_warp_inst.get_pred();
        need_write_back_regs_num.push_back((pred < 0) ? pred : pred + PRED_NUM_OFFSET);

        for (auto regnum : need_write_back_regs_num) {
          // void Scoreboard::releaseRegisters(const unsigned wid, std::vector<int> regnums)
          // m_scoreboard->releaseRegisters(_wid, regnum);
          m_scoreboard->releaseRegisters(global_all_kernels_warp_id, regnum);
          insert_into_active_warps_id(&active_warps_id, global_all_kernels_warp_id);
        }

        if (_DEBUG_LOG_)
          m_scoreboard->printContents();
      }
      
      preg = m_pipeline_reg[EX_WB].get_ready(&except_regs);
      pipe_reg = (preg == NULL) ? NULL : *preg;
      active_during_this_cycle = true;
    }

STOP_AND_REPORT_TIMER_rank(1);
START_TIMER(2);
    // std::cout << "@@@2";

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                                         Execute.                                       ***/
    /***                                                                                        ***/
    /**********************************************************************************************/

    for (unsigned i = 0; i < num_result_bus; i++) {
      *(m_result_bus[i]) >>= 1;
    }

    for (unsigned n = 0; n < m_fu.size(); n++) {
      for (auto _ = 0; _ < m_fu[n]->clock_multiplier(); _++) {
        // if (m_fu[n]->cycle()) {
        //   active_during_this_cycle = true;
        // }
        std::vector<unsigned> returned_wids = m_fu[n]->cycle(tracer, m_scoreboard, appcfg, 
                                                             &kernel_block_pair, 
                                                             &m_num_warps_per_sm, 
                                                             KERNEL_EVALUATION, 
                                                             num_scheds,
                                                             m_reg_bank_allocator,
                                                             &flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle,
                                                             &clk_record,
                                                             m_cycle);
        for (auto wid : returned_wids) {
          insert_into_active_warps_id(&active_warps_id, wid);
          active_during_this_cycle = true;
        }
      }
    }

    std::vector<opndcoll_rfu_t::input_port_t>* m_in_ports_ptr = m_operand_collector->get_m_in_ports();
    /* m_in_ports:
     *   m_in_ports[0]: GEN_CUS, &m_pipeline_reg[ID_OC_SP], &m_pipeline_reg[ID_OC_DP], ... => m_in
     *                           &m_pipeline_reg[OC_EX_SP], &m_pipeline_reg[OC_EX_DP], ... => m_out
     *   m_in_ports[1~7]: ... */
    for (unsigned p = 0; p < m_in_ports_ptr->size(); p++) {
      /* inp => m_in_ports[p]:
       *   GEN_CUS, &m_pipeline_reg[ID_OC_SP], &m_pipeline_reg[ID_OC_DP], ... => m_in
       *            &m_pipeline_reg[OC_EX_SP], &m_pipeline_reg[OC_EX_DP], ... => m_out 
       * We have put the instns that have completed the read operands process into 
       * m_pipeline_reg[OC_EX_SP] or m_pipeline_reg[OC_EX_DP] or ..., so here we will 
       * traverse all the inp.m_out[.] to find ready instns and send them to the execute
       * process. */
      opndcoll_rfu_t::input_port_t &inp = (*m_in_ports_ptr)[p];
      if (_DEBUG_LOG_)
        std::cout << "  **Execute: port_idx: " << p << std::endl;
      /* inp.m_out: 
       *   &m_pipeline_reg[OC_EX_SP], 
       *   &m_pipeline_reg[OC_EX_DP], ... */
      for (unsigned i = 0; i < inp.m_out.size(); i++) {
        if ((*inp.m_out[i]).has_ready()) {
          // print the ready instn
          /*//*/ if (_DEBUG_LOG_)
            std::cout << "  Ready instn in inp.m_out[" << i << "]: " << std::endl;
          // (*inp.m_out[i]).print();
          std::vector<unsigned> ready_reg_ids = (*inp.m_out[i]).get_ready_reg_ids();
          for (unsigned j = 0; j < ready_reg_ids.size(); j++) {
            unsigned reg_id = ready_reg_ids[j];
            unsigned _kid = (*inp.m_out[i]).get_kid(reg_id);
            unsigned _pc = (*inp.m_out[i]).get_pc(reg_id);
            unsigned _wid = (*inp.m_out[i]).get_wid(reg_id);
            unsigned _uid = (*inp.m_out[i]).get_uid(reg_id);
            unsigned _latency = (*inp.m_out[i]).get_latency(reg_id);
            /*//*/ if (_DEBUG_LOG_)
              std::cout << "    Ready instn "
                           "(pc, gwid, kid, fetch_instn_id, latency): (" << _pc << ", " 
                                                                         << _wid << ", " 
                                                                         << _kid << ", " 
                                                                         << _uid << ", "
                                                                         << _latency << ")" 
                                                                         << std::endl;
            // remove the instn from (*inp.m_out[i]) and add it to execute stages
            compute_instn* tmp = 
              tracer->get_one_kernel_one_warp_one_instn(_kid, _wid, _uid);
            _inst_trace_t* tmp_inst_trace = tmp->inst_trace;

            /*//*/ if (_DEBUG_LOG_)
              std::cout << "    tmp_inst_trace->get_func_unit(): " 
                        << tmp_inst_trace->get_func_unit() << std::endl;
            
            unsigned offset_fu = 0;
            bool schedule_wb_now = false;
            int resbus = -1;
            
            




            switch (tmp_inst_trace->get_func_unit()) {
              // In PrivateSM.cc, m_fu[SP_UNIT-1] => SP_UNIT_PIPELINE.
              case SP_UNIT:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    SP unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    SP unit initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                if (_DEBUG_LOG_) std::cout << "@#@#@#@#@#@#1 " << reg_id << std::endl;
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  SP initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = 0;
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_sp_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) {
                      std::cout << "num_result_bus: " << num_result_bus << std::endl;
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl
                                << "resbus: " << resbus << std::endl;
                    }
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_sp_units() << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              case DP_UNIT:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    DP unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    DP unit initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  DP initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_dp_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_dp_units() << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              case SFU_UNIT:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    SFU unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    SFU unit initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  SFU initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_sfu_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_sfu_units() << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              case TENSOR_CORE_UNIT:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    TENSOR_CORE unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    TENSOR_CORE unit initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  TC initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_tensor_core_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_tensor_core_units() << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              case INT_UNIT:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    INT unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    INT unit initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  INT initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_int_units(); _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) {
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    }
                    // std::cout << "    schedule_wb_now: " << schedule_wb_now 
                    //             << " resbus: " << resbus
                    //             << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn (schedule_wb_now): " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      // (*inp.m_out[i]).print();
                      // std::cout << "      after issue occupied.to_string(): ";
                      // for (int iii = 0; iii < 32; ++iii) {
                      //   std::cout << m_fu[offset_fu + _]->occupied[iii];
                      // }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn (!schedule_wb_now): " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      // std::cout << "      after issue occupied.to_string(): ";
                      // for (int iii = 0; iii < 32; ++iii) {
                      //   std::cout << m_fu[offset_fu + _]->occupied[iii];
                      // }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }
                    

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << m_hw_cfg->get_num_int_units() << std::endl;
                  } else {
                    if (_EXECUTE_DEBUG_LOG_) {
                      schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                      resbus = test_result_bus(tmp_inst_trace->get_latency());
                      std::cout << "    -schedule_wb_now: " << schedule_wb_now 
                                << " resbus: " << resbus
                                << std::endl;
                    }
                    
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  
                  }
                }
                break;
              case LDST_UNIT: {
                std::vector<std::string> opcode_tokens = tmp_inst_trace->get_opcode_tokens();
                // std::cout << "opcode_tokens: ";
                // for (auto x : opcode_tokens) {
                //   std::cout << x << " ";
                // }
                // std::cout << tmp_inst_trace->get_func_unit() << std::endl;
                /*
                LDC.U16/LDC.U8/LDC                              => LDC v
                LDS.U.128/LDS.U16/LDS.64/LDS.U                  => LDS v
                LDG.E.U16.CONSTANT.GPU/LDG.E.128.CONSTANT.GPU/
                LDG.E.CONSTANT.SYS/LDG.E.U16.CONSTANT.SYS/
                LDG.E.64.CONSTANT.SYS/LDG.E.64.CONSTANT.GPU/
                LDG.E.128.CONSTANT.SYS                          => CONSTANT v
                LDG.E.U16.STRONG.GPU/LDG.E.EL.STRONG.GPU/
                LDG.E.STRONG.SYS                                => LDG && STRONG v
                LDL.LU/LDL.64                                   => LDL v
                LD.E.SYS                                        => LD. v
                LDG.E.128.SYS/LDG.E.U16.SYS/LDG.E.SYS/
                LDG.E.64.SYS/LDG.E.U8.SYS/LDG.E.EF.SYS/
                LDG.E.EF.U16.SYS                                => ""

                STL/STL.64                                      => STL v
                STS.U16/STS/STS.128/STS.64                      => STS v
                STG.E.EF.STRONG.GPU/STG.E.EF.U16.SYS/
                STG.E.64.SYS/STG.E.128.SYS/
                STG.E.U16.SYS/STG.E.SYS/
                STG.E.U16.STRONG.GPU/STG.E.STRONG.SYS/
                STG.E.U8.SYS                                    => STG v
                */
                (*inp.m_out[i]).set_latency(33, reg_id);
                for (const auto& token : opcode_tokens) {
                  if (token.find("CONSTANT") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(8, reg_id);
                  } else if (token.find("LDC") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(8, reg_id);
                  } else if (token.find("LDS") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(33, reg_id);
                  } else if (token.find("STRONG") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(MEM_ACCESS_LATENCY, reg_id);
                  } else if (token.find("LDL") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(302, reg_id);
                  } else if (token.find("LD.") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(33, reg_id);
                  } else if (token.find("STL") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(302, reg_id);
                  } else if (token.find("STS") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(33, reg_id);
                  } else if (token.find("STG") != std::string::npos) {
                    (*inp.m_out[i]).set_latency(MEM_ACCESS_LATENCY, reg_id);
                  }
                }

                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    LDST unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    LDST unit initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                  std::cout << "@#@#@#@#@#@#2 " << reg_id << std::endl;
                  std::cout << "    LDST unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                }
                // TODO: get latency from memory_model from reuse distance.
                // (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id); // yangjianchao16 0310
                
                // mem_instn* mem_instns_ptr = 
                //   tracer->get_one_kernel_one_block_one_uid_mem_instn((*inp.m_out[i]).get_kid(reg_id), 
                //                                                      (*inp.m_out[i]).get_wid(reg_id), 
                //                                                      (*inp.m_out[i]).get_uid(reg_id)
                //                                                     );
                // std::cout << " LDG/STG: " << mem_instns_ptr->opcode << " " << mem_instns_ptr->has_mem_instn_type() << std::endl;
                
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                
                // std::cout << "  LDST initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units();
                for (unsigned _ = 0; _ < m_hw_cfg->get_num_mem_units(); _++) {
                  // if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                  if (m_fu[offset_fu + _]->can_issue(MEM_ACCESS_LATENCY)) { // yangjianchao16 0310
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    // resbus = test_result_bus(tmp_inst_trace->get_latency()); // yangjianchao16 0310
                    resbus = test_result_bus(MEM_ACCESS_LATENCY);
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      // m_result_bus[resbus]->set(tmp_inst_trace->get_latency()); // yangjianchao16 0310
                      m_result_bus[resbus]->set(MEM_ACCESS_LATENCY);
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid 
                                  << ", latecy-" << _latency << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Memory_Structural_icnt_injection_buffer_is_full = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  } else {
                    flag_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }

                break;
              }
              case SPEC_UNIT_1:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    SPEC_UNIT_1 unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    SPEC_UNIT_1 initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  SPEC1 initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units()
                            + m_hw_cfg->get_num_mem_units();
                for (unsigned _ = 0; _ < 1; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              case SPEC_UNIT_2:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    SPEC_UNIT_2 unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    SPEC_UNIT_2 initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  SPEC2 initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units()
                            + m_hw_cfg->get_num_mem_units();
                for (unsigned _ = 1; _ < 2; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              case SPEC_UNIT_3:
                if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                  std::cout << "    SPEC_UNIT_3 unit latency: " 
                            << tmp_inst_trace->get_latency() 
                            << std::endl;
                  std::cout << "    SPEC_UNIT_3 initiation interval: " 
                          << tmp_inst_trace->get_initiation_interval() 
                          << std::endl;
                }
                (*inp.m_out[i]).set_latency(tmp_inst_trace->get_latency(), reg_id);
                (*inp.m_out[i]).set_initial_interval(tmp_inst_trace->get_initiation_interval(), reg_id);
                // std::cout << "  SPEC3 initial interval: " 
                //           << tmp_inst_trace->get_initiation_interval() 
                //           << std::endl;

                offset_fu = m_hw_cfg->get_num_sp_units() + m_hw_cfg->get_num_sfu_units() 
                            + m_hw_cfg->get_num_int_units() + m_hw_cfg->get_num_dp_units()
                            + m_hw_cfg->get_num_tensor_core_units()
                            + m_hw_cfg->get_num_mem_units();
                for (unsigned _ = 2; _ < 3; _++) {
                  if (m_fu[offset_fu + _]->can_issue(tmp_inst_trace->get_latency())) {
                    schedule_wb_now = !m_fu[offset_fu + _]->stallable();
                    resbus = test_result_bus(tmp_inst_trace->get_latency());
                    if (_DEBUG_LOG_) 
                      std::cout << "schedule_wb_now: " << schedule_wb_now << std::endl;
                    insert_into_active_warps_id(&active_warps_id, _wid);
                    active_during_this_cycle = true;
                    if (schedule_wb_now &&
                        (resbus != -1)) {
                      m_result_bus[resbus]->set(tmp_inst_trace->get_latency());
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else if (!schedule_wb_now) {
                      m_fu[offset_fu + _]->issue((*inp.m_out[i]), reg_id);
                      if (_DEBUG_LOG_ || _EXECUTE_DEBUG_LOG_) {
                        std::cout << "    executing instn: " 
                                  << "pc-" << _pc 
                                  << ", wid-" << _wid 
                                  << ", kid-" << _kid 
                                  << ", uid-" << _uid << std::endl;
                      }
                      break;
                    } else {
                      /* stall issue (cannot reserve result bus) */
                      flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = true;
                    }

                    // std::cout << "@@@@@@" 
                    //           << (*inp.m_out[i]).get_size() << std::endl;
                    // std::cout << "######" 
                    //           << 1 << std::endl;
                  } else {
                    flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = true;
                  }
                }
                break;
              default:
                std::cout << "Error: tmp_inst_trace->get_func_unit(): " 
                          << tmp_inst_trace->get_func_unit() << std::endl;
                assert(false);
            }
          }
        }
      }
    }
    // std::cout << "@@@3";

STOP_AND_REPORT_TIMER_rank(2);
START_TIMER(3);

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                                      Read Operands.                                    ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    for (unsigned _iter = 0; _iter < get_reg_file_port_throughput(); _iter++) {
      if (_DEBUG_LOG_)
        std::cout << "  **Read Operands: " << "reg_file_port_idx: " 
                  << _iter << std::endl;
      m_operand_collector->step(&flag_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu,
                                &flag_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated,
                                &flag_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated,
                                &flag_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu,
                                tracer,
                                &clk_record, 
                                m_cycle
                                );
    }
    // std::cout << "@@@4";

STOP_AND_REPORT_TIMER_rank(3);
START_TIMER(4);

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                               Issue intns to issue_port.                               ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    /* Variables that depend on it_kernel_block_pair:
     *   kid, block_id, warps_per_block, gwarp_id_start, gwarp_id_end
     * The following code is to get these variables:
     *   unsigned kid = it_kernel_block_pair->first - 1;
     *   unsigned block_id = it_kernel_block_pair->second;
     *   unsigned warps_per_block = appcfg->get_num_warp_per_block(kid);
     *   unsigned gwarp_id_start = warps_per_block * block_id;
     *   unsigned gwarp_id_end = gwarp_id_start + warps_per_block - 1;
     */
    
    /*
    Algorithm 1 Instruction Stall Classification
 
    if No active warps to consider then
      classify idle stall
      stat_coll->set_At_least_one_Idle_Stall_found(true);                         V
    else if The next instruction to issue is unavailable then
      classify control stall
      stat_coll->set_At_least_one_Control_Stall_found(true);                      X
    else if Warp is blocked for a synchronization then
      classify synchronization stall
      stat_coll->set_At_least_one_Synchronization_Stall_found(true);              X
    else if Instruction has a data hazard on a pending load then
      classify memory data stall
      stat_coll->set_At_least_one_Memory_Data_Stall_found(true);                  V
    else if Instruction has a structural hazard on load/store unit then
      classify memory structural stall
      stat_coll->set_At_least_one_Memory_Structural_Stall_found(true);            V
    else if Instruction has a data hazard on a pending compute operation then
      classify compute data stall
      stat_coll->set_At_least_one_Compute_Data_Stall_found(true);                 V
    else if Instruction has a structural hazard on a compute unit then
      classify compute structural stall
      stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);           V
    else if Instruction is able to issue then
      classify no stall
      stat_coll->set_At_least_one_No_Stall_found(true);                           V
    end if
    */

    
    unsigned max_issue_per_warp = m_hw_cfg->get_max_insn_issue_per_warp();
    if (_DEBUG_LOG_)
      std::cout << "  **ISSUE stage: " << std::endl;
    // Loop for four warp shedulers
    unsigned total_issued_instn_num = 0;
    for (unsigned _sched_id = 0; _sched_id < num_scheds; _sched_id++) {
      auto sched_id = (last_issue_sched_id + _sched_id) % num_scheds;
      if (_DEBUG_LOG_)
        std::cout << "    D: issue sched_id: " << sched_id << std::endl;
      
      // std::cout << "### 1" << std::endl;
      // std::cout << "kernel_block_pair.size(): " << kernel_block_pair.size() << std::endl;
      // it_kernel_block_pair is the current block that is being considered.
      for (unsigned i = 0; i < kernel_block_pair.size(); i++) {
        // std::cout << "### 2" << std::endl;
        auto it_kernel_block_pair = 
          kernel_block_pair.begin() + 
          (last_issue_block_index_per_sched[sched_id] + i) % kernel_block_pair.size();
        // std::cout << "### 3" << std::endl;
        if (it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;


      // for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
      //           (it_kernel_block_pair != kernel_block_pair.end()) &&
      //           // (it_kernel_block_pair->second == 80) && // NEED TO DELETE
      //           (it_kernel_block_pair->first - 1 == KERNEL_EVALUATION); // TODO: here only first kernel is considered
      //           it_kernel_block_pair++) {
        
        /* -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
         * Here, kernel_block_pair is (1,80), (1,160), (1,0), (2,0), (3,0), (4,0)
         * for it_kernel_block_pair : kernel_block_pair:
         *   _kid             =   0,   0,   0,   1,   2,   3
         *   _block_id        =  80, 160,   0,   0,   0,   0
         *   _warps_per_block =   3,   3,   3,   3,   3,   3
         *   _gwarp_id_start  = 240, 480,   0,   0,   0,   0
         *   _gwarp_id_end    = 242, 482,   1,   1,   1,   1
         */
        
        unsigned _kid = it_kernel_block_pair->first - 1;
        unsigned _block_id = it_kernel_block_pair->second;
        unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
        unsigned _gwarp_id_start = _warps_per_block * _block_id;
        unsigned _gwarp_id_end = _gwarp_id_start + _warps_per_block - 1;

        /* Find if <_kid, _block_id> exits in last_issue_warp_ids, if true: get its value; if 
         * false, set 0. Its defination:
         *   // Key: <kid, block_id>, value: warp_id.
         *   std::map<std::pair<int, int>, int> last_issue_warp_ids; */
        unsigned last_issue_warp_id;

        auto find_last_issue_warp_id = last_issue_warp_ids.find(std::make_pair(_kid, _block_id));
          
        if (find_last_issue_warp_id == last_issue_warp_ids.end()) {
          last_issue_warp_ids[std::make_pair(_kid, _block_id)] = 0;
          last_issue_warp_id = 0;
        } else {
          last_issue_warp_id = find_last_issue_warp_id->second;
        }
      
        /* LRR warp sheduling */
        for (auto gwid = _gwarp_id_start; 
            (gwid <= _gwarp_id_end) 
            // && (gwid % num_scheds == sched_id)
            ; 
            gwid++) {
          // std::cout << last_issue_warp_ids.size() << std::endl;
          // for (auto it = last_issue_warp_ids.begin(); it != last_issue_warp_ids.end(); it++) {
          //   std::cout << it->second << std::endl;
          // }

          /* Here, wid is actually in the bound [_gwarp_id_start, _gwarp_id_end]. */
          auto wid = (last_issue_warp_id + gwid) % _warps_per_block + _gwarp_id_start;
          /* What we need to note here is that x % num_scheds != sched_id, the x variable should 
           * be the index the wid is in the current SM. So we should first calculate the index
           * wid is. */
          unsigned _idx_wid_in_SM = wid - _gwarp_id_start;
          for (auto _ = kernel_block_pair.begin(); 
                    _ != kernel_block_pair.end(); 
                    _++) {
            if (_->first - 1 != KERNEL_EVALUATION) continue;
            if ((_->first - 1 < _kid) || (_->first - 1 == _kid && _->second < _block_id)) 
              _idx_wid_in_SM += appcfg->get_num_warp_per_block(_->first - 1);
          }
          if (_idx_wid_in_SM % num_scheds != sched_id) {
            if (_DEBUG_LOG_)
              std::cout << "      $Kid-" << _kid 
                        << ", block_id-" << _block_id 
                        << ", wid-" << wid 
                        << ", idx_wid_in_SM-" << _idx_wid_in_SM 
                        << " NOT BELONG TO sched_id-" << sched_id 
                        << std::endl;
            continue;
          }
          /* m_num_warps_per_sm's first dim is the total num of kernels that are executed on
           * the GPU, and its second dim is the number of warps that blongs to the current SM.
           * for example, 
           *   -trace_issued_sm_id_0 5,0,(1,80),(1,160),(1,0),(2,0),(2,80)
           *   and the 1st kernel has 3 warps per thread block, the 2nd kernel has 2 warps per
           *   thread block, then:
           *     m_num_warps_per_sm[0] = 9, m_num_warps_per_sm[1] = 6.
           * So, the following code is to get the global warp id of the total kernels 
           * (global_all_kernels_warp_id). For example, we have 2 kernels in the whole SM, and 
           * the 1st kernel has 3 warps per thread block, then the 2nd kernel has 2 warps per 
           * thread block, thus the global_all_kernels_warp_id of the 2st kernel's 2nd warp is 
           * 9 + 2 = 11.
           * 
           * And with the above assumption, the IBuffer's slots are allocated in the following:
           *  0 slot => warp 0 from kernel 0
           *  1 slot => warp 1 from kernel 0
           *  ......
           *  8 slot => warp 8 from kernel 0
           *  9 slot => warp 0 from kernel 1
           * 10 slot => warp 1 from kernel 1
           * ......
           * 14 slot => warp 5 from kernel 1
           */

          /* Count the entries in the kernel_block_pair that are smaller than _block_id with
          * the same _kid. */
          unsigned _kid_block_id_count = 0;
          for (auto _it_kernel_block_pair = kernel_block_pair.begin(); 
                    _it_kernel_block_pair != kernel_block_pair.end();
                    _it_kernel_block_pair++) {
            if (_it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
            if (_it_kernel_block_pair->first - 1 == _kid) {
              if (_it_kernel_block_pair->second < _block_id) {
                _kid_block_id_count++;
              }
            }
          }
          // std::cout << " @_block_id: " << _kid << " " << _kid_block_id_count  << std::endl;

          /* With the above assumption, for _wid from (2,80), global_all_kernels_warp_id:
           * slot 12/13/14 in the IBuffer will be filled.  */
          auto global_all_kernels_warp_id = 
            (unsigned)(wid % _warps_per_block) + 
            _kid_block_id_count * _warps_per_block +
            std::accumulate(m_num_warps_per_sm.begin(), 
                            m_num_warps_per_sm.begin() + _kid, 0);
          
          // auto global_all_kernels_warp_id = wid + std::accumulate(m_num_warps_per_sm.begin(), m_num_warps_per_sm.begin() + _kid, 0);
          // std::cout << "D: global_all_kernels_warp_id: " << global_all_kernels_warp_id << std::endl;
          // std::cout << "D: m_ibuffer->is_not_empty(global_all_kernels_warp_id): " 
          //           << m_ibuffer->is_not_empty(global_all_kernels_warp_id) << std::endl;
          
          unsigned issued_num = 0;
          unsigned checked_num = 0;

          exec_unit_type_t previous_issued_inst_exec_type = exec_unit_type_t::NONE;

          while (
            (issued_num < max_issue_per_warp) &&
            (checked_num <= issued_num)
          ) {
            bool warp_inst_issued = false;

            std::vector<int> regnums;
            int pred;
            int ar1;
            int ar2;

            if (m_ibuffer->is_not_empty(global_all_kernels_warp_id)) {
              if (_DEBUG_LOG_) {
                std::cout << "  Before pop,m_ibuffer:" << std::endl;
                std::cout << "wid:" << global_all_kernels_warp_id;
                m_ibuffer->print_ibuffer(global_all_kernels_warp_id);
              }
              // pop the instn from ibuffer
              ibuffer_entry entry = m_ibuffer->front(global_all_kernels_warp_id);
              if (_DEBUG_LOG_) {
                std::cout << "  pop_front(uid,pc,wid,kid): " 
                          << entry.uid << " " 
                          << entry.pc << " " 
                          << entry.wid << " " 
                          << entry.kid << std::endl;
                std::cout << "  After pop,m_ibuffer:" << std::endl;
                m_ibuffer->print_ibuffer(global_all_kernels_warp_id);
              }
              unsigned _fetch_instn_id = entry.uid;
              unsigned _pc = entry.pc;
              unsigned _gwid = entry.wid;
              unsigned _kid = entry.kid;
              if (_DEBUG_LOG_)
                std::cout << "  try to issue: " << _fetch_instn_id << " " 
                                                << _pc << " " 
                                                << _gwid << " " 
                                                << _kid << std::endl;
              
              
              
              // get instn by entry
              compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(_kid, _gwid, _fetch_instn_id);
              _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
              trace_warp_inst_t* tmp_trace_warp_inst = &(tmp->trace_warp_inst);

              if (_DEBUG_LOG_) 
                std::cout << "    instn: " 
                          << tmp_inst_trace->instn_str << std::endl;
              
              if (_DEBUG_LOG_)
                std::cout << "    opcode: " 
                          << tmp_trace_warp_inst->get_opcode() << " " 
                          << tmp_trace_warp_inst->get_op() << std::endl;
              
              if (_DEBUG_LOG_)
                std::cout << "            " << OP_SHFL 
                          << " " << ALU_OP << std::endl;

              auto latency = tmp_inst_trace->get_latency();
              
              if (_DEBUG_LOG_)
                std::cout << "    latency: " << latency << std::endl;
              
              auto init_latency = tmp_inst_trace->get_initiation_interval();
              
              if (_DEBUG_LOG_)
                std::cout << "    init_latency: " << init_latency 
                          << std::endl;

              if (_DEBUG_LOG_) {
                std::cout << "  @ pred: " 
                          << tmp_trace_warp_inst->get_pred() << " ";;
                std::cout << "  @ ar1: " 
                          << tmp_trace_warp_inst->get_ar1() << " ";;
                std::cout << "  @ ar2: " 
                          << tmp_trace_warp_inst->get_ar2() << " ";;
                std::cout << "  @ srcs: ";
                for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
                  std::cout << tmp_inst_trace->reg_src[i] << " ";
                }
                std::cout << std::endl;
                std::cout << "  @ dsts: ";
                for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
                  std::cout << tmp_inst_trace->reg_dest[i] << " ";
                }
                std::cout << std::endl;
              }
              
              pred = tmp_trace_warp_inst->get_pred();
              ar1 = tmp_trace_warp_inst->get_ar1();
              ar2 = tmp_trace_warp_inst->get_ar2();
              
              
              for (unsigned i = 0; i < tmp_inst_trace->reg_srcs_num; i++) {
                regnums.push_back(tmp_inst_trace->reg_src[i]);
              }
              for (unsigned i = 0; i < tmp_inst_trace->reg_dsts_num; i++) {
                if (tmp_inst_trace->reg_dest_is_pred[i]) 
                  regnums.push_back(tmp_inst_trace->reg_dest[i] + PRED_NUM_OFFSET);
                else
                  regnums.push_back(tmp_inst_trace->reg_dest[i]);
              }

              bool check_is_scoreboard_collision = false;
              check_is_scoreboard_collision = 
                m_scoreboard->checkCollision(global_all_kernels_warp_id, 
                                             regnums, 
                                             /* set PRED+NUM_OFFSET to avoid collision with regnums */
                                             (pred < 0) ? pred : pred + PRED_NUM_OFFSET, 
                                             ar1,
                                             ar2);

              // get the function unit of the instn
              auto fu = tmp_inst_trace->get_func_unit();

              if (check_is_scoreboard_collision) {
                if (fu == LDST_UNIT) {
                  stat_coll->set_At_least_one_Memory_Data_Stall_found(true);
                  flag_Issue_Memory_Data_scoreboard = true;
                } else {
                  stat_coll->set_At_least_one_Compute_Data_Stall_found(true);
                  flag_Issue_Compute_Data_scoreboard = true;
                }
              }
              
              if (_DEBUG_LOG_)
                std::cout << "  check_is_scoreboard_collision: " 
                          << check_is_scoreboard_collision << " " << global_all_kernels_warp_id << std::endl;
              // if (check_is_scoreboard_collision)
              //   m_scoreboard->printContents(global_all_kernels_warp_id);
              if (tmp_trace_warp_inst->get_opcode() == OP_EXIT && 
                  tracer->get_one_kernel_one_warp_instn_count(_kid, _gwid) == _fetch_instn_id + 1 &&
                  m_scoreboard->regs_size(global_all_kernels_warp_id) > 0) {
                check_is_scoreboard_collision = true;
              }
              
              if (check_is_scoreboard_collision) {
                checked_num++;
                continue;
              }
              
              
              
              if (_DEBUG_LOG_)
                std::cout << "  Execute on FU: ";
              
              /* Identify the availability of function units. */
              bool sp_pipe_avail = false;
              bool sfu_pipe_avail = false;
              bool int_pipe_avail = false;
              bool dp_pipe_avail = false;
              bool tensor_core_pipe_avail = false;
              bool ldst_pipe_avail = false;
              bool spec_1_pipe_avail = false;
              bool spec_2_pipe_avail = false;
              bool spec_3_pipe_avail = false;
              /*
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
              */
              switch (fu) {
                case NON_UNIT:
                  if (_DEBUG_LOG_)
                    std::cout << "NON_UNIT" << std::endl;
                  assert(0);
                  break;
                case SP_UNIT: {
                  if (_DEBUG_LOG_)
                    std::cout << "SP_UNIT" << std::endl;
                  bool _has_free_slot_sp = m_sp_out->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_sp) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::SP);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  sp_pipe_avail = 
                    (m_hw_cfg->get_num_sp_units() > 0) &&
                    _has_free_slot_sp &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "sp_pipe_avail: " << sp_pipe_avail << std::endl;
                  if (sp_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_sp_out, entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::SP;
                    
                    if (_DEBUG_LOG_) m_sp_out->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case SFU_UNIT: {
                  if (_DEBUG_LOG_)
                    std::cout << "SFU_UNIT" << std::endl;
                  bool _has_free_slot_sfu = m_sfu_out->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_sfu) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::SFU);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  sfu_pipe_avail = 
                    (m_hw_cfg->get_num_sfu_units() > 0) &&
                    _has_free_slot_sfu &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "sfu_pipe_avail: " << sfu_pipe_avail << std::endl;
                  if (sfu_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_sfu_out, entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::SFU;
                    
                    if (_DEBUG_LOG_) m_sfu_out->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case INT_UNIT: {
                  if (_DEBUG_LOG_)
                    std::cout << "INT_UNIT" << std::endl;
                  bool _has_free_slot_int = m_int_out->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_int) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::INT);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  int_pipe_avail = 
                    (m_hw_cfg->get_num_int_units() > 0) &&
                    _has_free_slot_int &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() || 
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  int_pipe_avail: " << int_pipe_avail << std::endl;
                  if (int_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_int_out, entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::INT;
                    
                    if (_DEBUG_LOG_) m_int_out->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case DP_UNIT: {
                  if (_DEBUG_LOG_)
                    std::cout << "DP_UNIT" << std::endl;
                  bool _has_free_slot_dp = m_dp_out->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_dp) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::DP);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  dp_pipe_avail = 
                    (m_hw_cfg->get_num_dp_units() > 0) &&
                    _has_free_slot_dp &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  dp_pipe_avail: " << dp_pipe_avail << std::endl;
                  if (dp_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_dp_out, entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::DP;
                    
                    if (_DEBUG_LOG_) m_dp_out->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case TENSOR_CORE_UNIT: {
                  if (_DEBUG_LOG_)
                    std::cout << "TENSOR_CORE_UNIT" << std::endl;
                  bool _has_free_slot_tc = m_tensor_core_out->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_tc) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::TENSOR);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  tensor_core_pipe_avail = 
                    (m_hw_cfg->get_num_tensor_core_units() > 0) &&
                    _has_free_slot_tc &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  tensor_core_pipe_avail: " << tensor_core_pipe_avail << std::endl;
                  if (tensor_core_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_tensor_core_out, entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::TENSOR;
                    
                    if (_DEBUG_LOG_) m_tensor_core_out->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case LDST_UNIT: {
                  if (_DEBUG_LOG_)
                    std::cout << "LDST_UNIT" << std::endl;
                  bool _has_free_slot_mem = m_mem_out->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_mem) {
                    flag_Issue_Memory_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::LDST);
                  if (!pre) {
                    flag_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory = true;
                  }
                  ldst_pipe_avail = 
                    (m_hw_cfg->get_num_mem_units() > 0) &&
                    _has_free_slot_mem &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() || 
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  ldst_pipe_avail: " << ldst_pipe_avail << std::endl;
                  if (ldst_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_mem_out, entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::LDST;
                    
                    if (_DEBUG_LOG_) m_mem_out->print();
                  } else {
                    stat_coll->set_At_least_one_Memory_Structural_Stall_found(true);
                  }
                  break;
                }
                case SPEC_UNIT_1: {
                  if (_DEBUG_LOG_)
                    std::cout << "SPEC_UNIT_1" << std::endl;
                  bool _has_free_slot_spec1 = m_spec_cores_out[0]->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_spec1) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::SPECIALIZED);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  spec_1_pipe_avail = 
                    (m_hw_cfg->get_specialized_unit_1_enabled()) &&
                    _has_free_slot_spec1 &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  spec_1_pipe_avail: " << spec_1_pipe_avail << std::endl;
                  if (spec_1_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_spec_cores_out[0], entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::SPECIALIZED;
                    
                    if (_DEBUG_LOG_) m_spec_cores_out[0]->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case SPEC_UNIT_2: {
                  if (_DEBUG_LOG_)
                    std::cout << "SPEC_UNIT_2" << std::endl;
                  bool _has_free_slot_spec2 = m_spec_cores_out[1]->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_spec2) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::SPECIALIZED);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  spec_2_pipe_avail = 
                    (m_hw_cfg->get_specialized_unit_2_enabled()) &&
                    _has_free_slot_spec2 &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  spec_2_pipe_avail: " << spec_2_pipe_avail << std::endl;
                  if (spec_2_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_spec_cores_out[1], entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::SPECIALIZED;
                    
                    if (_DEBUG_LOG_) m_spec_cores_out[1]->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                case SPEC_UNIT_3: {
                  if (_DEBUG_LOG_)
                    std::cout << "SPEC_UNIT_3" << std::endl;
                  bool _has_free_slot_spec3 = m_spec_cores_out[2]->has_free(m_hw_cfg->get_sub_core_model(), sched_id);
                  if (!_has_free_slot_spec3) {
                    flag_Issue_Compute_Structural_out_has_no_free_slot = true;
                  }
                  bool pre = (previous_issued_inst_exec_type != exec_unit_type_t::SPECIALIZED);
                  if (!pre) {
                    flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = true;
                  }
                  spec_3_pipe_avail = 
                    (m_hw_cfg->get_specialized_unit_3_enabled()) &&
                    _has_free_slot_spec3 &&
                    (!m_hw_cfg->get_dual_issue_diff_exec_units() ||
                    pre);
                  if (_DEBUG_LOG_)
                    std::cout << "  spec_3_pipe_avail: " << spec_3_pipe_avail << std::endl;
                  if (spec_3_pipe_avail) {
                    // issue
                    warp_inst_issued = true;
                    issued_num++;
                    issue_warp(*m_spec_cores_out[2], entry, sched_id);
                    previous_issued_inst_exec_type = exec_unit_type_t::SPECIALIZED;
                    
                    if (_DEBUG_LOG_) m_spec_cores_out[2]->print();
                  } else {
                    stat_coll->set_At_least_one_Compute_Structural_Stall_found(true);
                  }
                  break;
                }
                default:
                  if (_DEBUG_LOG_)
                    std::cout << "default UNIT" << std::endl;
                  assert(0);
              }



              // traverse m_pipeline_reg
              if (false) {
                std::cout << "==================================================" << std::endl;
                for (auto it = m_pipeline_reg.begin(); it != m_pipeline_reg.end(); it++) {
                  std::cout << "  ||name: " << it->get_name() << std::endl;
                  std::cout << "    has_ready: " << it->has_ready() << std::endl;
                  std::cout << "    has_free: " << it->has_free() << std::endl;
                }


                std::cout << "  ||name: " << m_sp_out->get_name() << std::endl;
                std::cout << "    has_ready: " << m_sp_out->has_ready() << std::endl;
                std::cout << "    has_free: " << m_sp_out->has_free() << std::endl;
                // m_sp_out->print();
                std::cout << "  ||name: " << m_dp_out->get_name() << std::endl;
                std::cout << "    has_ready: " << m_dp_out->has_ready() << std::endl;
                std::cout << "    has_free: " << m_dp_out->has_free() << std::endl;
                // m_dp_out->print();
                std::cout << "  ||name: " << m_sfu_out->get_name() << std::endl;
                std::cout << "    has_ready: " << m_sfu_out->has_ready() << std::endl;
                std::cout << "    has_free: " << m_sfu_out->has_free() << std::endl;
                // m_sfu_out->print();
                std::cout << "  ||name: " << m_int_out->get_name() << std::endl;
                std::cout << "    has_ready: " << m_int_out->has_ready() << std::endl;
                std::cout << "    has_free: " << m_int_out->has_free() << std::endl;
                // m_int_out->print();
                std::cout << "  ||name: " << m_tensor_core_out->get_name() << std::endl;
                std::cout << "    has_ready: " << m_tensor_core_out->has_ready() << std::endl;
                std::cout << "    has_free: " << m_tensor_core_out->has_free() << std::endl;
                // m_tensor_core_out->print();
                std::cout << "  ||name: " << m_mem_out->get_name() << std::endl;
                std::cout << "    has_ready: " << m_mem_out->has_ready() << std::endl;
                std::cout << "    has_free: " << m_mem_out->has_free() << std::endl;
                // m_mem_out->print();
                std::cout << "  ||name: " << m_spec_cores_out[0]->get_name() << std::endl;
                std::cout << "    has_ready: " << m_spec_cores_out[0]->has_ready() << std::endl;
                std::cout << "    has_free: " << m_spec_cores_out[0]->has_free() << std::endl;
                // m_spec_cores_out[0]->print();
                std::cout << "  ||name: " << m_spec_cores_out[1]->get_name() << std::endl;
                std::cout << "    has_ready: " << m_spec_cores_out[1]->has_ready() << std::endl;
                std::cout << "    has_free: " << m_spec_cores_out[1]->has_free() << std::endl;
                // m_spec_cores_out[1]->print();
                std::cout << "  ||name: " << m_spec_cores_out[2]->get_name() << std::endl;
                std::cout << "    has_ready: " << m_spec_cores_out[2]->has_ready() << std::endl;
                std::cout << "    has_free: " << m_spec_cores_out[2]->has_free() << std::endl;
                // m_spec_cores_out[2]->print();
                std::cout << "==================================================" << std::endl;
              }

              if (warp_inst_issued) {
                total_issued_instn_num++;
                if (_CALIBRATION_LOG_) {
                  std::cout << "    ISSUE: ("
                            << _kid << ", "
                            << _gwid << ", "
                            << _fetch_instn_id << ", "
                            << _pc << ")" << std::endl;
                }
                set_clk_record<2>(_kid, _gwid, _fetch_instn_id, m_cycle);
              }
              
            } else {
              stat_coll->set_At_least_one_Idle_Stall_found(true);
            }

            if (warp_inst_issued) {
              active_during_this_cycle = true;
              insert_into_active_warps_id(&active_warps_id, global_all_kernels_warp_id);
              m_ibuffer->pop_front(global_all_kernels_warp_id);
              // check_is_scoreboard_collision = 
              //   m_scoreboard->checkCollision(global_all_kernels_warp_id, 
              //                                regnums, 
              //                                /* set PRED+NUM_OFFSET to avoid collision with regnums */
              //                                pred + PRED_NUM_OFFSET, 
              //                                ar1,
              //                                ar2);

              // reserveRegisters(const unsigned wid, std::vector<int> regnums, bool is_load)
              
              regnums.push_back((pred < 0) ? pred : pred + PRED_NUM_OFFSET); 
              // BUG: need to change pred < 0 to "P?"
              regnums.push_back(ar1);
              regnums.push_back(ar2);
              if (_DEBUG_LOG_)
                std::cout << "  NOW reserveRegisters for warp " 
                          << global_all_kernels_warp_id << std::endl;
              m_scoreboard->reserveRegisters(global_all_kernels_warp_id, regnums, false);
              if (_DEBUG_LOG_)
                m_scoreboard->printContents();
            }

            checked_num++;
          }
        }
        last_issue_warp_ids[std::make_pair(_kid, _block_id)] = 
          (last_issue_warp_ids[std::make_pair(_kid, _block_id)] + 1) % _warps_per_block;
      }
      last_issue_block_index_per_sched[sched_id] = 
        (last_issue_block_index_per_sched[sched_id] + 1) % kernel_block_pair.size();
    }
    
    last_issue_sched_id = (last_issue_sched_id + 1) % num_scheds;

    if (total_issued_instn_num >= num_scheds) {
      stat_coll->set_At_least_one_No_Stall_found(true);
    }
    // std::cout << "total_issued_instn_num: " << (total_issued_instn_num >= num_scheds) << std::endl;
    // std::cout << "@@@5";

STOP_AND_REPORT_TIMER_rank(4);
START_TIMER(5);

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                           Fetch and Decode instns to Fbuffer.                          ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    /* Variables that depend on it_kernel_block_pair:
     *   kid, block_id, warps_per_block, gwarp_id_start, gwarp_id_end
     * The following code is to get these variables:
     *   unsigned kid = it_kernel_block_pair->first - 1;
     *   unsigned block_id = it_kernel_block_pair->second;
     *   unsigned warps_per_block = appcfg->get_num_warp_per_block(kid);
     *   unsigned gwarp_id_start = warps_per_block * block_id;
     *   unsigned gwarp_id_end = gwarp_id_start + warps_per_block - 1;
     */
    /*
    for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
                (it_kernel_block_pair != kernel_block_pair.end()) &&
                // (it_kernel_block_pair->second == 80) && // NEED TO DELETE
                (it_kernel_block_pair->first - 1 == KERNEL_EVALUATION); // TODO: here only first kernel is considered
                it_kernel_block_pair++) {

        unsigned _kid = it_kernel_block_pair->first - 1;
        unsigned _block_id = it_kernel_block_pair->second;
        unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
        unsigned _gwarp_id_start = _warps_per_block * _block_id;
        unsigned _gwarp_id_end = _gwarp_id_start + _warps_per_block - 1;
    */
    
    for (unsigned _ = 0; _ < get_inst_fetch_throughput(); _++) {
      /* DECODE */
      
      if (m_inst_fetch_buffer->m_valid) {
        auto _pc = m_inst_fetch_buffer->pc;
        /* MOST IMPORTANT: From here that <kid, wid, uid, pc> that are transferred in the 
         * pipeline is defined. The most important thing is that, the wid is the global
         * warp id in the whole kernel, for example,
         *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
         * and there are 3 warps per block in the 1st kernel, then:
         * wid during the transfer in the pipeline is 
         *   80 * 3 = 240     or 
         *   80 * 3 + 1 = 241 or
         *   80 * 3 + 2 = 242.
         */
        auto _wid = m_inst_fetch_buffer->wid;
        auto _kid = m_inst_fetch_buffer->kid;
        auto _uid = m_inst_fetch_buffer->uid;
        // std::cout << "_wid: " << _wid << " _kid: " << _kid << " _uid: " << _uid << std::endl;

        unsigned __pc, __wid, __kid, __uid;                         // yangjianchao16 add 20240131

        if (m_inst_fetch_buffer_copy->m_valid) {
          __pc = m_inst_fetch_buffer_copy->pc;                 // yangjianchao16 add 20240131
          __wid = m_inst_fetch_buffer_copy->wid;               // yangjianchao16 add 20240131
          __kid = m_inst_fetch_buffer_copy->kid;               // yangjianchao16 add 20240131
          __uid = m_inst_fetch_buffer_copy->uid;               // yangjianchao16 add 20240131
        }

        /* m_num_warps_per_sm's first dim is the total num of kernels that are executed on
         * the GPU, and its second dim is the number of warps that blongs to the current SM.
         * for example, 
         *   -trace_issued_sm_id_0 5,0,(1,80),(1,160),(1,0),(2,0),(2,80)
         *   and the 1st kernel has 3 warps per thread block, the 2nd kernel has 2 warps per
         *   thread block, then:
         *     m_num_warps_per_sm[0] = 9, m_num_warps_per_sm[1] = 6.
         * So, the following code is to get the global warp id of the total kernels 
         * (global_all_kernels_warp_id). For example, we have 2 kernels in the whole SM, and 
         * the 1st kernel has 3 warps per thread block, then the 2nd kernel has 2 warps per 
         * thread block, thus the global_all_kernels_warp_id of the 2st kernel's 2nd warp is 
         * 9 + 2 = 11.
         * 
         * And with the above assumption, the IBuffer's slots are allocated in the following:
         *    0 slot => warp 0 from kernel 0
         *    1 slot => warp 1 from kernel 0
         *    ......
         *    8 slot => warp 8 from kernel 0
         *    9 slot => warp 0 from kernel 1
         *   10 slot => warp 1 from kernel 1
         *   ......
         *   14 slot => warp 5 from kernel 1
         */
        
        /* Get warps_per_block of instn <_pc, _wid, _kid, _uid>. */
        unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
        /* Calculate the block id. */
        
        unsigned _block_id = (unsigned)(_wid / _warps_per_block);
        
        /* Count the entries in the kernel_block_pair that are smaller than _block_id with
         * the same _kid. */
        unsigned _kid_block_id_count = 0;
        for (auto _it_kernel_block_pair = kernel_block_pair.begin(); 
                  _it_kernel_block_pair != kernel_block_pair.end();
                  _it_kernel_block_pair++) {
          if (_it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
          if (_it_kernel_block_pair->first - 1 == _kid) {
            if (_it_kernel_block_pair->second < _block_id) {
              _kid_block_id_count++;
            }
          }
        }
        
        /* With the above assumption, for _wid from (2,80), global_all_kernels_warp_id:
         * slot 12/13/14 in the IBuffer will be filled.  */
        auto global_all_kernels_warp_id = 
          (unsigned)(_wid % _warps_per_block) + 
          _kid_block_id_count * _warps_per_block +
          std::accumulate(m_num_warps_per_sm.begin(), 
                          m_num_warps_per_sm.begin() + _kid, 0);
        // auto global_all_kernels_warp_id = _wid;
        
        // std::cout << "global_all_kernels_warp_id: " << global_all_kernels_warp_id << std::endl;

        // std::cout << "@@@ " << m_ibuffer->has_free_slot(global_all_kernels_warp_id) 
        //           << " " << std::hex << _pc << std::dec << " " << _wid << " " << _kid << " " << _uid << std::endl;
        if (m_ibuffer->has_free_slot(global_all_kernels_warp_id)) {
          auto _entry = ibuffer_entry(_pc, _wid, _kid, _uid);
          m_ibuffer->push_back(global_all_kernels_warp_id, _entry);
          m_inst_fetch_buffer->m_valid = false;
          active_during_this_cycle = true;
          insert_into_active_warps_id(&active_warps_id, global_all_kernels_warp_id);
          if (_CALIBRATION_LOG_) {
            std::cout << "    DECODE: ("
                      << _entry.kid << ", "
                      << _entry.wid << ", "
                      << _entry.uid << ", "
                      << _entry.pc << ")" << std::endl;
            
          }
          set_clk_record<1>(_entry.kid, _entry.wid, _entry.uid, m_cycle);

          if (m_inst_fetch_buffer_copy->m_valid) {
            ibuffer_entry __entry = ibuffer_entry(__pc, __wid, __kid, __uid);       // yangjianchao16 add 20240131
            m_ibuffer->push_back(global_all_kernels_warp_id, __entry);     // yangjianchao16 add 20240131
            m_inst_fetch_buffer_copy->m_valid = false;                     // yangjianchao16 add 20240131
            if (_CALIBRATION_LOG_) {
              std::cout << "    DECODE: ("
                        << __entry.kid << ", "
                        << __entry.wid << ", "
                        << __entry.uid << ", "
                        << __entry.pc << ")" << std::endl;
            }
            set_clk_record<1>(__entry.kid, __entry.wid, __entry.uid, m_cycle);
          }

          if (_DEBUG_LOG_) {
            std::cout << "  **DECODE: ";
            m_ibuffer->print_ibuffer(global_all_kernels_warp_id);
          }
          
          
          
        } else {
          if (_DEBUG_LOG_)
            std::cout << "  **No DECODE cause m_ibuffer->"
                         "has_free_slot() == false" << std::endl;
        }
      } else {
        if (_DEBUG_LOG_)
          std::cout << "  **No DECODE cause m_inst_fetch_buffer->"
                       "m_valid == false" << std::endl;
      }
      
      // std::cout << "@@@ 1" << std::endl;
      
      /* Continue the loop in advance based on the state. */
      if (m_inst_fetch_buffer->m_valid) {
        if (_DEBUG_LOG_)
          std::cout << "  **No fetch cauz m_inst_fetch_buffer->"
                       "m_valid == true" << std::endl;
        continue;
      }

      // for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
      //           it_kernel_block_pair != kernel_block_pair.end();
      //           it_kernel_block_pair++) {
      //   std::cout << "@ it_kernel_block_pair: " << it_kernel_block_pair->first 
      //             << " " << it_kernel_block_pair->second << std::endl;
      //   std::cout << "@ KERNEL_EVALUATION: " << KERNEL_EVALUATION << std::endl;
      // }

      unsigned all_blocks_in_this_sm = 0;
      unsigned first_block_in_this_sm = -1;
      for (auto it_kernel_block_pair_ = kernel_block_pair.begin(); 
                it_kernel_block_pair_ != kernel_block_pair.end();
                it_kernel_block_pair_++) {
        // std::cout << "it_kernel_block_pair_: " << it_kernel_block_pair_->first << " " 
        //           << it_kernel_block_pair_->second << " " << KERNEL_EVALUATION << std::endl;
        if ((it_kernel_block_pair_->first - 1) == KERNEL_EVALUATION) {
          all_blocks_in_this_sm += 1;
          // std::cout << "   all_blocks_in_this_sm+1" << std::endl;
          if (first_block_in_this_sm == -1) {
            first_block_in_this_sm = std::distance(kernel_block_pair.begin(), it_kernel_block_pair_);
          }
        }
      }



      // std::cout << "all_blocks_in_this_sm: " << all_blocks_in_this_sm << std::endl;
    if (true) {
      
      std::vector<std::pair<int, int>> kernel_block_pair_need_to_check;
      for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
                it_kernel_block_pair != kernel_block_pair.end();
                it_kernel_block_pair++) {
        if (it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
        unsigned _index= std::distance(kernel_block_pair.begin(), it_kernel_block_pair);
        if (m_thread_block_has_executed_status[_index] == true && get_num_m_warp_active_status(_index) > 0) {
          kernel_block_pair_need_to_check.push_back(*it_kernel_block_pair);
        }
      }

      // std::cout << "kernel_block_pair_need_to_check: " << kernel_block_pair_need_to_check.size() << std::endl;

      for (unsigned check_block_id_index_idx = 0; check_block_id_index_idx < kernel_block_pair_need_to_check.size(); check_block_id_index_idx++) {
        if (m_inst_fetch_buffer->m_valid) break;
        
        unsigned check_block_id = (check_block_id_index_idx + last_check_block_id_index_idx) % kernel_block_pair_need_to_check.size();
        
        unsigned _kid = kernel_block_pair_need_to_check[check_block_id].first - 1;
        unsigned _block_id = kernel_block_pair_need_to_check[check_block_id].second;
        unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
        unsigned _gwarp_id_start = _warps_per_block * _block_id;
        unsigned _gwarp_id_end = _gwarp_id_start + _warps_per_block - 1;

        for (auto gwid = _gwarp_id_start; gwid <= _gwarp_id_end; gwid++) {
          unsigned wid = (gwid + kernel_id_block_id_last_fetch_wid[{_kid, check_block_id}]) % _warps_per_block  + _gwarp_id_start;
          
          unsigned _index;
          for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
                    it_kernel_block_pair != kernel_block_pair.end();
                    it_kernel_block_pair++) {
            if ((it_kernel_block_pair->first - 1 == _kid) && (it_kernel_block_pair->second == _block_id)) {
              _index = std::distance(kernel_block_pair.begin(), it_kernel_block_pair);
            }
          }
          
          unsigned _w_id_ = (unsigned)(gwid % _warps_per_block);
          if (!(m_thread_block_has_executed_status[_index] == true && m_warp_active_status[_index][_w_id_])) continue;

          bool fetch_instn = false;

          while (!m_inst_fetch_buffer->m_valid) {
            curr_instn_id_per_warp_entry _entry = curr_instn_id_per_warp_entry(_kid, _block_id, wid - _gwarp_id_start);
            unsigned fetch_instn_id = curr_instn_id_per_warp[_entry];
            
            unsigned one_warp_instn_size = tracer->get_one_kernel_one_warp_instn_size(_kid, wid);
            
            if (one_warp_instn_size <= fetch_instn_id) {
              unsigned _wid_1 = wid - _gwarp_id_start;
              m_warp_active_status[_index][_wid_1] = false;
              break;
            }

            compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(_kid, wid, fetch_instn_id);
            
            _inst_trace_t* tmp_inst_trace = tmp->inst_trace;

              if (!tmp_inst_trace->m_valid) break;

              m_inst_fetch_buffer->pc = tmp_inst_trace->m_pc;
              /* MOST IMPORTANT: From here that <kid, wid, uid, pc> that are transferred in the 
               * pipeline is defined. The most important thing is that, the wid is the global
               * warp id in the whole kernel, for example,
               *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
               * and there are 3 warps per block in the 1st kernel, then:
               * wid during the transfer in the pipeline is 
               *   80 * 3 = 240     or 
               *   80 * 3 + 1 = 241 or
               *   80 * 3 + 2 = 242.
               */
              m_inst_fetch_buffer->wid = wid;
              m_inst_fetch_buffer->kid = _kid;
              m_inst_fetch_buffer->uid = fetch_instn_id;
              m_inst_fetch_buffer->m_valid = true;
              
              active_during_this_cycle = true;
              insert_into_active_warps_id(&active_warps_id, wid);

              if (_CALIBRATION_LOG_) {
                std::cout << "    FETCH: ("
                          << _kid << ", "
                          << wid << ", "
                          << fetch_instn_id << ", "
                          << tmp_inst_trace->m_pc << ")" << std::endl;
              }
              set_clk_record<0>(_kid, wid, fetch_instn_id, m_cycle);
              
              fetch_instn = true;
              
              curr_instn_id_per_warp[_entry] += 2;               // yangjianchao16 add 20240131
              
              /* COPY FETCH START */              // yangjianchao16 add 20240131
              if (fetch_instn_id + 1 < tracer->get_one_kernel_one_warp_one_instn_max_size(_kid, wid)) {
                tmp = tracer->get_one_kernel_one_warp_one_instn(_kid, wid, fetch_instn_id + 1);
                tmp_inst_trace = tmp->inst_trace;
                if (!tmp_inst_trace->m_valid) break;
                m_inst_fetch_buffer_copy->pc = tmp_inst_trace->m_pc;
                m_inst_fetch_buffer_copy->wid = wid;
                m_inst_fetch_buffer_copy->kid = _kid;
                m_inst_fetch_buffer_copy->uid = fetch_instn_id + 1;
                m_inst_fetch_buffer_copy->m_valid = true;
              } else {
                m_inst_fetch_buffer_copy->pc = tmp_inst_trace->m_pc;
                m_inst_fetch_buffer_copy->wid = wid;
                m_inst_fetch_buffer_copy->kid = _kid;
                m_inst_fetch_buffer_copy->uid = fetch_instn_id;
                m_inst_fetch_buffer_copy->m_valid = false;
              }
              
              if (_CALIBRATION_LOG_) {
                std::cout << "    FETCH: ("
                          << _kid << ", "
                          << wid << ", "
                          << fetch_instn_id + 1 << ", "
                          << tmp_inst_trace->m_pc << ")" << std::endl;
              }
              set_clk_record<0>(_kid, wid, fetch_instn_id + 1, m_cycle);
          }
        }
        kernel_id_block_id_last_fetch_wid[{_kid, check_block_id}] = 
          (kernel_id_block_id_last_fetch_wid[{_kid, check_block_id}] + 1) % _warps_per_block;
      }
      
      // last_check_block_id_index_idx = (last_check_block_id_index_idx + 1) % kernel_block_pair_need_to_check.size();
      last_check_block_id_index_idx++;
    }

    if (false) {
      unsigned _index_ = -1;
      for (auto it_kernel_block_pair = kernel_block_pair.begin(); 
                it_kernel_block_pair != kernel_block_pair.end();
                it_kernel_block_pair++) {
        if (it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;
        _index_++;
        // std::cout << "        _index_ : " << _index_ << std::endl;
        // std::cout << "        distance_last_fetch_kid : " << distance_last_fetch_kid << std::endl;
        // std::cout << "@@@ 3" << std::endl;
        // std::cout << " it_kernel_block_pair: " << it_kernel_block_pair->first << " " 
        //           << it_kernel_block_pair->second << std::endl;
        /* Calculate the distance of it_kernel_block_pair to kernel_block_pair.begin(). */
        // unsigned _index_ = std::distance(kernel_block_pair.begin(), it_kernel_block_pair);

        if (_index_ < distance_last_fetch_kid) continue;
        else if (_index_ > distance_last_fetch_kid) break;
        // std::cout << "@@@ 4" << std::endl;
        /* Terminate the loop in advance based on the state. */
        if (m_inst_fetch_buffer->m_valid) break;
        
        /* -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
         * Here, kernel_block_pair is (1,80), (1,160), (1,0), (2,0), (3,0), (4,0)
         * for it_kernel_block_pair : kernel_block_pair:
         *   _index_          =   0,   1,   2,   3,   4,   5
         *   _kid             =   0,   0,   0,   1,   2,   3
         *   _block_id        =  80, 160,   0,   0,   0,   0
         *   _warps_per_block =   3,   3,   3,   3,   3,   3
         *   _gwarp_id_start  = 240, 480,   0,   0,   0,   0
         *   _gwarp_id_end    = 242, 482,   1,   1,   1,   1
         */
        
        unsigned _kid = it_kernel_block_pair->first - 1;
        unsigned _block_id = it_kernel_block_pair->second;
        unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
        unsigned _gwarp_id_start = _warps_per_block * _block_id;
        unsigned _gwarp_id_end = _gwarp_id_start + _warps_per_block - 1;
        // std::cout << "@@@ 5" << std::endl;
        // std::cout << "D: _gwarp_id_start, _gwarp_id_end: " << _gwarp_id_start << " " << _gwarp_id_end << std::endl;

        /* FETCH */
        /* Here, gwid is the global warp id in the whole kernel, for example,
         *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
         * and there are 3 warps per block in the 1st kernel, then in the following loop:
         *   _gwarp_id_start = 80 * 3 = 240, 
         *   _gwarp_id_end   = 80 + 3 - 1 = 242,
         *   gwid = 240, 241, 242. */

        bool need_contiue_fetch_outer = false;

        for (auto gwid = _gwarp_id_start; gwid <= _gwarp_id_end; gwid++) {
          /* Round-robin issue */
          /* In fact, here wid is still in the bound [_gwarp_id_start, _gwarp_id_end], and
           * it is just a reorder of the loop to simulate the round-robin issue. So with the 
           * last supposition, gwid = 240 or 241 or 242. 
           * The reason that here we need the global warp id in the whole kernel, is that, we
           * will get new instns, where the get_one_kernel_one_warp_instn_size function should
           * be passed the parameter (kernel id, global warp id in the whole kernel). */
          auto wid = (last_fetch_warp_id[_kid] + gwid) % _warps_per_block + _gwarp_id_start;

          unsigned _index = std::distance(kernel_block_pair.begin(), it_kernel_block_pair);
          unsigned _w_id_ = (unsigned)(wid % _warps_per_block);
          if (!(m_thread_block_has_executed_status[_index] == true && m_warp_active_status[_index][_w_id_])) continue;
          
          // std::cout << "           gwid: " << gwid << std::endl;
          // std::cout << "           _gwarp_id_start: " << _gwarp_id_start << std::endl;
          // std::cout << "           _gwarp_id_end: " << _gwarp_id_end << std::endl;
          // std::cout << "           wid: " << wid << std::endl;

          // std::cout << "           D: last_fetch_warp_id[_kid], gwid, wid: " 
          //           << last_fetch_warp_id[_kid] << " " << gwid << " " << wid << std::endl;
          
          /* check if the ibuffer has free slot */
          bool fetch_instn = false;
          bool need_contiue_fetch = false;
          // std::cout << "           D: m_inst_fetch_buffer->m_valid: " << m_inst_fetch_buffer->m_valid << std::endl;
          while (!m_inst_fetch_buffer->m_valid) {
            // std::cout << "@@@ " << !m_inst_fetch_buffer->m_valid << std::endl;
            /* curr_instn_id_per_warp stores the current instn id of each warp, which is indexed 
             * using the object curr_instn_id_per_warp_entry(_kid,_block_id,gwid-_gwarp_id_start).
             * After accessing a new instn, the value curr_instn_id_per_warp[_entry] should plus 1. */
            
            // std::cout << "D: Access curr_instn_id_per_warp: " << _kid << " " <<  _block_id << " " << gwid - _gwarp_id_start << std::endl;
            
            
            curr_instn_id_per_warp_entry _entry = curr_instn_id_per_warp_entry(_kid, _block_id, wid - _gwarp_id_start);
            unsigned fetch_instn_id = curr_instn_id_per_warp[_entry];
            
            // std::cout << "           _kid, wid: " << _kid << " " << wid << std::endl;
            // std::cout << "           D: fetch_instn_id: " << fetch_instn_id << std::endl;
            // std::cout << "           D: tracer->get_one_kernel_one_warp_one_instn_max_size(_kid, wid): " << tracer->get_one_kernel_one_warp_one_instn_max_size(_kid, wid) << std::endl;
            unsigned one_warp_instn_size = tracer->get_one_kernel_one_warp_instn_size(_kid, wid);
            
            if (one_warp_instn_size <= fetch_instn_id) {
              unsigned _index_1 = std::distance(kernel_block_pair.begin(), it_kernel_block_pair);
              unsigned _wid_1 = wid - _gwarp_id_start;
              // std::cout << "         set m_warp_active_status[_index_1][_wid_1] = false: " << _index_1 << " " << _wid_1 << std::endl;
              m_warp_active_status[_index_1][_wid_1] = false;
            }
            if (one_warp_instn_size <= fetch_instn_id) {
              // std::cout << "           need_contiue_fetch" << std::endl;
              need_contiue_fetch = true;
              break;
            }
            // std::cout << "@@@ 6" << std::endl;


            // std::cout << "           " << _kid << " " << wid << " " << fetch_instn_id << std::endl;
            compute_instn* tmp = tracer->get_one_kernel_one_warp_one_instn(_kid, wid, fetch_instn_id);
            
            // std::cout << "D: @@@@@@ : " << (tmp == nullptr) << std::endl;
            

            _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
            
            // m_inst_fetch_buffer = inst_fetch_buffer_entry(tmp_inst_trace->m_pc, wid, _kid, fetch_instn_id);
            // if (tmp_inst_trace != nullptr) {
              // std::cout << "11111111111" << std::endl;
              // std::cout << !tmp_inst_trace->m_valid << std::endl;
              // std::cout << "22222222222" << std::endl;
              if (!tmp_inst_trace->m_valid) break;

              // std::cout << "D: @@@ "  << tmp_inst_trace->m_pc << std::endl;
              // std::cout << "1111" << std::endl;
              m_inst_fetch_buffer->pc = tmp_inst_trace->m_pc;
              // std::cout << "2222" << std::endl;
              /* MOST IMPORTANT: From here that <kid, wid, uid, pc> that are transferred in the 
               * pipeline is defined. The most important thing is that, the wid is the global
               * warp id in the whole kernel, for example,
               *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
               * and there are 3 warps per block in the 1st kernel, then:
               * wid during the transfer in the pipeline is 
               *   80 * 3 = 240     or 
               *   80 * 3 + 1 = 241 or
               *   80 * 3 + 2 = 242.
               */
              m_inst_fetch_buffer->wid = wid;
              // std::cout << "3333" << std::endl;
              m_inst_fetch_buffer->kid = _kid;
              // std::cout << "4444" << std::endl;
              m_inst_fetch_buffer->uid = fetch_instn_id;
              // std::cout << "5555" << std::endl;
              m_inst_fetch_buffer->m_valid = true;
              // std::cout << "6666" << std::endl;
              if (_DEBUG_LOG_)
                std::cout << "  **Fetch instn "
                             "(pc,gwid,kid,fetch_instn_id): " << "(" 
                                                              << std::hex 
                                                              << tmp_inst_trace->m_pc << ", " 
                                                              << std::dec
                                                              << wid << ", " 
                                                              << _kid << ", " 
                                                              << fetch_instn_id << ")" 
                                                              << std::endl;

              active_during_this_cycle = true;
              insert_into_active_warps_id(&active_warps_id, wid);

              if (_CALIBRATION_LOG_) {
                std::cout << "    FETCH: ("
                          << _kid << ", "
                          << wid << ", "
                          << fetch_instn_id << ", "
                          << tmp_inst_trace->m_pc << ")" << std::endl;
              }
              set_clk_record<0>(_kid, wid, fetch_instn_id, m_cycle);
              fetch_instn = true;
              // curr_instn_id_per_warp[_entry]++;               // yangjianchao16 del 20240131
              curr_instn_id_per_warp[_entry] += 2;               // yangjianchao16 add 20240131
              if (_DEBUG_LOG_)
                std::cout << " @+1 kid:" << _entry.kid 
                          << " block_id:" << _entry.block_id 
                          << " wid:" << _entry.warp_id << std::endl;

              /* COPY FETCH START */              // yangjianchao16 add 20240131
              // std::cout << " ### " << tracer->get_one_kernel_one_warp_one_instn_max_size(_kid, wid) << std::endl;
              if (fetch_instn_id + 1 < tracer->get_one_kernel_one_warp_one_instn_max_size(_kid, wid)) {
                tmp = tracer->get_one_kernel_one_warp_one_instn(_kid, wid, fetch_instn_id + 1);
                tmp_inst_trace = tmp->inst_trace;
                if (!tmp_inst_trace->m_valid) break;
                m_inst_fetch_buffer_copy->pc = tmp_inst_trace->m_pc;
                m_inst_fetch_buffer_copy->wid = wid;
                m_inst_fetch_buffer_copy->kid = _kid;
                m_inst_fetch_buffer_copy->uid = fetch_instn_id + 1;
                m_inst_fetch_buffer_copy->m_valid = true;
              } else {
                m_inst_fetch_buffer_copy->pc = tmp_inst_trace->m_pc;
                m_inst_fetch_buffer_copy->wid = wid;
                m_inst_fetch_buffer_copy->kid = _kid;
                m_inst_fetch_buffer_copy->uid = fetch_instn_id;
                m_inst_fetch_buffer_copy->m_valid = false;
              }
              
              if (_DEBUG_LOG_)
                std::cout << "  **Fetch instn "
                             "(pc,gwid,kid,fetch_instn_id): " << "(" 
                                                              << std::hex 
                                                              << tmp_inst_trace->m_pc << ", " 
                                                              << std::dec
                                                              << wid << ", " 
                                                              << _kid << ", " 
                                                              << fetch_instn_id + 1 << ")" 
                                                              << std::endl;

              if (_CALIBRATION_LOG_) {
                std::cout << "    FETCH: ("
                          << _kid << ", "
                          << wid << ", "
                          << fetch_instn_id + 1 << ", "
                          << tmp_inst_trace->m_pc << ")" << std::endl;
              }
              set_clk_record<0>(_kid, wid, fetch_instn_id + 1, m_cycle);
              /* COPY FETCH END */                // yangjianchao16 add 20240131

              
            // }
          }

          if (gwid == _gwarp_id_end && need_contiue_fetch) {
            need_contiue_fetch_outer = true;
            break;
          }

          if (need_contiue_fetch) {
            continue;
          } 
          
          if (fetch_instn) break;
          else {
            if (_DEBUG_LOG_)
              std::cout << "  **No FETCH" << std::endl;
          }

        }

        // std::cout << "          it_kernel_block_pair: " 
        //           << std::distance(kernel_block_pair.begin(), it_kernel_block_pair) << " " 
        //           << all_blocks_in_this_sm << std::endl;
        if (need_contiue_fetch_outer && 
            // (it_kernel_block_pair + 1 != kernel_block_pair.end())
            (std::distance(kernel_block_pair.begin(), it_kernel_block_pair) - first_block_in_this_sm != all_blocks_in_this_sm - 1) 
           ) {
          distance_last_fetch_kid = (distance_last_fetch_kid + 1) % all_blocks_in_this_sm;
          // std::cout << "                  set distance_last_fetch_kid 1 = " << distance_last_fetch_kid << std::endl;
          continue;
        }

        // last_fetch_warp_id = (last_fetch_warp_id + 1) % _warps_per_block;
        last_fetch_warp_id[_kid] = (last_fetch_warp_id[_kid] + 1) % _warps_per_block;
      }
      // std::cout << ";10 ";
      // std::cout << KERNEL_EVALUATION << " " << m_num_blocks_per_kernel[KERNEL_EVALUATION] << " ";
      // distance_last_fetch_kid = (distance_last_fetch_kid + 1) % m_num_blocks_per_kernel[KERNEL_EVALUATION];
      // distance_last_fetch_kid = (distance_last_fetch_kid + 1) % kernel_block_pair.size();
      // std::cout << "@@@ 7 " << all_blocks_in_this_sm << std::endl;
      distance_last_fetch_kid = (distance_last_fetch_kid + 1) % all_blocks_in_this_sm;
      // std::cout << "                  set distance_last_fetch_kid 2 = " << distance_last_fetch_kid << std::endl;

      // std::cout << ";11 ";
    }
    }

    // std::cout << "@@@6";

STOP_AND_REPORT_TIMER_rank(5);
START_TIMER(6);

    /**********************************************************************************************/
    /***                                                                                        ***/
    /***                            Release all register bank state.                            ***/
    /***                                                                                        ***/
    /**********************************************************************************************/
    /* At the end of a single cycle, we should release all the bank to start a new cycle. */
    for (unsigned i = 0; i < num_banks; i++) {
      if (m_reg_bank_allocator->getBankState(i) == ON_WRITING || 
          m_reg_bank_allocator->getBankState(i) == ON_READING) {
        m_reg_bank_allocator->releaseBankState(i);
      }
    }

    bool all_warps_finished = true;

    for (auto it_kernel_block_pair = kernel_block_pair.begin();
              it_kernel_block_pair != kernel_block_pair.end();
              it_kernel_block_pair++) {
      if (it_kernel_block_pair->first - 1 != KERNEL_EVALUATION) continue;

      // std::cout << "D: SM-" << m_smid << " Kernel-" << it_kernel_block_pair->first - 1 << " Block-" << it_kernel_block_pair->second << std::endl;
      
      /* -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
       * Here, kernel_block_pair is (1,80), (1,160), (1,0), (2,0), (3,0), (4,0)
       * for it_kernel_block_pair : kernel_block_pair:
       *   _index           =   0,   1,   2,   3,   4,   5
       *   _kid             =   0,   0,   0,   1,   2,   3
       *   _block_id        =  80, 160,   0,   0,   0,   0
       *   _warps_per_block =   3,   3,   3,   3,   3,   3
       *   _gwarp_id_start  = 240, 480,   0,   0,   0,   0
       *   _gwarp_id_end    = 242, 482,   1,   1,   1,   1
       */
      unsigned _index= std::distance(kernel_block_pair.begin(), it_kernel_block_pair);
      unsigned _kid = it_kernel_block_pair->first - 1;
      unsigned _block_id = it_kernel_block_pair->second;
      unsigned _warps_per_block = appcfg->get_num_warp_per_block(_kid);
      unsigned _gwarp_id_start = _warps_per_block * _block_id;
      unsigned _gwarp_id_end = _gwarp_id_start + _warps_per_block - 1;

      /* In fact, here _w_id_ is the local warp id of the thread block (_kid, _block_id). */
      for (unsigned _w_id_ = 0; _w_id_ < _warps_per_block; _w_id_++) {
        
        /* m_warp_active_status[
         *                      0 ~ kernel_block_pair.size()
         *                     ]
         *                     [
         *                      0 ~ warps_per_block_of_it_kernel_block_pair
         *                     ]
         * In fact, m_warp_active_status's 1st dim is the index of (kid, block_id) pairs
         * that are issued to the current SM, and the 2nd dim is the number of warps in 
         * the thread block - (kid, block_id), and equals to the warp_per_block of the 
         * kernel - kid. */
        /* We now need to calculate the index of kernel - block pair (_kid, _block_id). */

        
        
        /* MOST IMPORTANT: Here, _wid is the global warp id in the whole kernel, for example,
         *   -trace_issued_sm_id_0 6,0,(1,80),(1,160),(1,0),(2,0),(3,0),(4,0)
         * and there are 3 warps per block in the 1st kernel, then:
         * wid during the transfer in the pipeline is 
         *   80 * 3 = 240     or 
         *   80 * 3 + 1 = 241 or
         *   80 * 3 + 2 = 242.
         * _gwarp_id_start is the global warp id of the first warp in the block, for example,
         * 80 * 3 = 240.
         */

        // std::cout << "    D: SM-" << m_smid 
        //           << " Kernel-" << _kid 
        //           << " Warp-" << _w_id_ 
        //           << " active status: " 
        //           << m_warp_active_status[_index][_w_id_] << std::endl;

        if ( (m_thread_block_has_executed_status[_index] == true && m_warp_active_status[_index][_w_id_]) ||
             (m_thread_block_has_executed_status[_index] == false) 
           ) {
          all_warps_finished = false;
          break;
        }

        // if (m_warp_active_status[_index][_w_id_]) {
        //   all_warps_finished = false;
        //   break;
        // }
      }

      if (!all_warps_finished) break; // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    }
    
    if (all_warps_finished) {
      // std::cout << " all_warps_finished : " << all_warps_finished << std::endl;
      active = false;
      if (_DEBUG_LOG_)
        std::cout << "D: SM-" << m_smid << " is finished." << std::endl;
    }

    if (active_during_this_cycle) {
      active_cycles++;
    }

    // std::cout << m_active_warps << " " << std::endl;

    active_warps_id_size_sum +=  get_num_m_warp_active_status();

    /*
    void set_At_least_one_Compute_Structural_Stall_found(bool value) { At_least_one_Compute_Structural_Stall_found = value; }
    void set_At_least_one_Compute_Data_Stall_found(bool value) { At_least_one_Compute_Data_Stall_found = value; }
    void set_At_least_one_Memory_Structural_Stall_found(bool value) { At_least_one_Memory_Structural_Stall_found = value; }
    void set_At_least_one_Memory_Data_Stall_found(bool value) { At_least_one_Memory_Data_Stall_found = value; }
    void set_At_least_one_Synchronization_Stall_found(bool value) { At_least_one_Synchronization_Stall_found = value; }
    void set_At_least_one_Control_Stall_found(bool value) { At_least_one_Control_Stall_found = value; }
    void set_At_least_one_Idle_Stall_found(bool value) { At_least_one_Idle_Stall_found = value; }
    void set_At_least_one_No_Stall_found(bool value) { At_least_one_No_Stall_found = value; }
    */
    /*
    Algorithm 2 Issue Cycle Stall Classification
    
    if At least one instruction was able to issue then
      classify no stall
    else if At least one memory structural stall was found then
      classify memory structural stall
    else if At least one memory data stall was found then
      classify memory data stall
    else if At least one synchronization stall was found then
      classify synchronization stall
    else if At least one compute structural stall was found then
      classify compute structural stall
    else if At least one compute data stall was found then
      classify compute data stall
    else if At least one control stall was found then
      classify control stall
    else if At least one idle stall was found then
      classify idle stall
    end if
    */
    /*
    bool flag_Issue_Compute_Structural_out_has_no_free_slot = false;
    bool flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute = false;
    bool flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency = false;
    bool flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty = false;
    bool flag_Writeback_Compute_Structural_bank_of_reg_is_not_idle = false;
    bool flag_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated = false;
    bool flag_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu = false;
    
    
    bool flag_Issue_Compute_Data_scoreboard = false;

    bool flag_Issue_Memory_Data_scoreboard = false;
    bool flag_Execute_Memory_Data_L1 = false;
    bool flag_Execute_Memory_Data_L2 = false;
    bool flag_Execute_Memory_Data_Main_Memory = false;

    bool flag_Issue_Memory_Structural_out_has_no_free_slot = false;
    bool flag_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory = false;
    bool flag_Execute_Memory_Structural_result_bus_has_no_slot_for_latency = false;
    bool flag_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty = false;
    bool flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle = false;
    bool flag_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated = false;
    bool flag_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu = false;
    bool flag_Execute_Memory_Structural_icnt_injection_buffer_is_full = false;
    */

    
    if (stat_coll->get_At_least_one_No_Stall_found()) {
      stat_coll->increment_No_Stall(m_smid);
    } else if (stat_coll->get_At_least_one_Memory_Structural_Stall_found()) {
      stat_coll->increment_Memory_Structural_Stall(m_smid);

      stat_coll->increment_num_Issue_Memory_Structural_out_has_no_free_slot(
        (unsigned)flag_Issue_Memory_Structural_out_has_no_free_slot, m_smid);
      stat_coll->increment_num_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory(
        (unsigned)flag_Issue_Memory_Structural_previous_issued_inst_exec_type_is_memory, m_smid);
      stat_coll->increment_num_Execute_Memory_Structural_result_bus_has_no_slot_for_latency(
        (unsigned)flag_Execute_Memory_Structural_result_bus_has_no_slot_for_latency, m_smid);
      stat_coll->increment_num_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty(
        (unsigned)flag_Execute_Memory_Structural_m_dispatch_reg_of_fu_is_not_empty, m_smid);
      stat_coll->increment_num_Writeback_Memory_Structural_bank_of_reg_is_not_idle(
        (unsigned)flag_Writeback_Memory_Structural_bank_of_reg_is_not_idle, m_smid);
      stat_coll->increment_num_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated(
        (unsigned)flag_ReadOperands_Memory_Structural_bank_reg_belonged_to_was_allocated, m_smid);
      stat_coll->increment_num_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(
        (unsigned)flag_ReadOperands_Memory_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu, m_smid);
      stat_coll->increment_num_Execute_Memory_Structural_icnt_injection_buffer_is_full(
        (unsigned)flag_Execute_Memory_Structural_icnt_injection_buffer_is_full, m_smid);

    } else if (stat_coll->get_At_least_one_Memory_Data_Stall_found()) {
      stat_coll->increment_Memory_Data_Stall(m_smid);

      stat_coll->increment_num_Issue_Memory_Data_scoreboard(
        (unsigned)flag_Issue_Memory_Data_scoreboard, m_smid);
      stat_coll->increment_num_Execute_Memory_Data_L1(
        (unsigned)flag_Execute_Memory_Data_L1, m_smid);
      stat_coll->increment_num_Execute_Memory_Data_L2(
        (unsigned)flag_Execute_Memory_Data_L2, m_smid);
      stat_coll->increment_num_Execute_Memory_Data_Main_Memory(
        (unsigned)flag_Execute_Memory_Data_Main_Memory, m_smid);

    } else if (stat_coll->get_At_least_one_Synchronization_Stall_found()) {
      stat_coll->increment_Synchronization_Stall(m_smid);
    } else if (stat_coll->get_At_least_one_Compute_Structural_Stall_found()) {
      stat_coll->increment_Compute_Structural_Stall(m_smid);

      stat_coll->increment_num_Issue_Compute_Structural_out_has_no_free_slot(
        (unsigned)flag_Issue_Compute_Structural_out_has_no_free_slot, m_smid);
      stat_coll->increment_num_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute(
        (unsigned)flag_Issue_Compute_Structural_previous_issued_inst_exec_type_is_compute, m_smid);
      stat_coll->increment_num_Execute_Compute_Structural_result_bus_has_no_slot_for_latency(
        (unsigned)flag_Execute_Compute_Structural_result_bus_has_no_slot_for_latency, m_smid);
      stat_coll->increment_num_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty(
        (unsigned)flag_Execute_Compute_Structural_m_dispatch_reg_of_fu_is_not_empty, m_smid);
      stat_coll->increment_num_Writeback_Compute_Structural_bank_of_reg_is_not_idle(
        (unsigned)flag_Writeback_Compute_Structural_bank_of_reg_is_not_idle, m_smid);
      stat_coll->increment_num_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated(
        (unsigned)flag_ReadOperands_Compute_Structural_bank_reg_belonged_to_was_allocated, m_smid);
      stat_coll->increment_num_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu(
        (unsigned)flag_ReadOperands_Compute_Structural_port_num_m_in_ports_m_in_fails_as_not_found_free_cu, m_smid);

    } else if (stat_coll->get_At_least_one_Compute_Data_Stall_found()) {
      stat_coll->increment_Compute_Data_Stall(m_smid);

      stat_coll->increment_num_Issue_Compute_Data_scoreboard(
        (unsigned)flag_Issue_Compute_Data_scoreboard, m_smid);

    } else if (stat_coll->get_At_least_one_Control_Stall_found()) {
      stat_coll->increment_Control_Stall(m_smid);
    } else if (stat_coll->get_At_least_one_Idle_Stall_found()) {
      stat_coll->increment_Idle_Stall(m_smid);
    } else {
      stat_coll->increment_Other_Stall(m_smid);
    }

    stat_coll->set_At_least_four_instns_issued(false);
    stat_coll->set_At_least_one_Compute_Structural_Stall_found(false);
    stat_coll->set_At_least_one_Compute_Data_Stall_found(false);
    stat_coll->set_At_least_one_Memory_Structural_Stall_found(false);
    stat_coll->set_At_least_one_Memory_Data_Stall_found(false);
    stat_coll->set_At_least_one_Synchronization_Stall_found(false);
    stat_coll->set_At_least_one_Control_Stall_found(false);
    stat_coll->set_At_least_one_Idle_Stall_found(false);
    stat_coll->set_At_least_one_No_Stall_found(false);

    if (all_warps_finished && PRINT_STALLS_DISTRIBUTION) {
      float all_total_stalls = (float) (stat_coll->get_Compute_Structural_Stall(m_smid) + 
                                        stat_coll->get_Compute_Data_Stall(m_smid) + 
                                        stat_coll->get_Memory_Structural_Stall(m_smid) + 
                                        stat_coll->get_Memory_Data_Stall(m_smid) + 
                                        stat_coll->get_Synchronization_Stall(m_smid) + 
                                        stat_coll->get_Control_Stall(m_smid) + 
                                        stat_coll->get_Idle_Stall(m_smid) + 
                                        stat_coll->get_Other_Stall(m_smid) + 
                                        stat_coll->get_No_Stall(m_smid));
      std::cout << "  Stalls Distribution:" << std::endl;
      std::cout << "    Compute Structural Stall: " << (float)stat_coll->get_Compute_Structural_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Compute Data Stall: " << (float)stat_coll->get_Compute_Data_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Memory Structural Stall: " << (float)stat_coll->get_Memory_Structural_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Memory Data Stall: " << (float)stat_coll->get_Memory_Data_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Synchronization Stall: " << (float)stat_coll->get_Synchronization_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Control Stall: " << (float)stat_coll->get_Control_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Idle Stall: " << (float)stat_coll->get_Idle_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    Other Stall: " << (float)stat_coll->get_Other_Stall(m_smid) / all_total_stalls << std::endl;
      std::cout << "    No Stall: " << (float)stat_coll->get_No_Stall(m_smid) / all_total_stalls << std::endl;
    }
    


STOP_AND_REPORT_TIMER_rank(6);

  // } /* end of it_kernel_block_pair */
}