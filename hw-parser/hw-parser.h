

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#ifndef HW_PARSER_H
#define HW_PARSER_H

#define MHZ *1000000

#define _DEBUG_LOG_ 0
#define _CALIBRATION_LOG_ 0

enum pipeline_stage_name_t {
  ID_OC_SP = 0,
  ID_OC_DP = 1,
  ID_OC_INT = 2,
  ID_OC_SFU = 3,
  ID_OC_MEM = 4,
  OC_EX_SP = 5,
  OC_EX_DP = 6,
  OC_EX_INT = 7,
  OC_EX_SFU = 8,
  OC_EX_MEM = 9,
  EX_WB = 10,
  ID_OC_TENSOR_CORE = 11,
  OC_EX_TENSOR_CORE = 12,
  N_PIPELINE_STAGES
};

const char *const pipeline_stage_name_decode[] = {
    "ID_OC_SP",          "ID_OC_DP",         "ID_OC_INT", "ID_OC_SFU",
    "ID_OC_MEM",         "OC_EX_SP",         "OC_EX_DP",  "OC_EX_INT",
    "OC_EX_SFU",         "OC_EX_MEM",        "EX_WB",     "ID_OC_TENSOR_CORE",
    "OC_EX_TENSOR_CORE", "N_PIPELINE_STAGES"};

enum opcode_operation {
  ADD = 0,
  MAX = 1,
  MUL = 2,
  MAD = 3,
  DIV = 4,
  SHFL = 5,
};

enum specialized_unit_name {
  BRA_name = 0,
  TEX_name = 1,
  TENSOR_name = 2,
};

/* Type of Warp Scheduler */
enum warp_scheduler_type {
  SCHEDULER_LRR = 0,
  SCHEDULER_GTO,
  SCHEDULER_TWO_LEVEL_ACTIVE,
  SCHEDULER_RRR,
  SCHEDULER_WARP_LIMITING,
  SCHEDULER_OLDEST_FIRST,
  NUM_SCHEDULERS
};

class hw_config {
 public:
  hw_config(const std::string config_file) {
    m_valid = false;

    /* Device Limits */
    stack_size_limit = 0;
    heap_size_limit = 0;
    kernel_launch_latency = 0;
    thread_block_launch_latency = 0;
    max_concurent_kernel = 0;

    /* High Level Architecture Configuration */
    num_clusters = 0;
    num_sms_per_cluster = 0;
    num_memory_controllers = 0;
    num_sub_partition_per_memory_channel = 0;

    /* Clock Domain Frequencies in MHZ */
    core_clock_mhz = 0;
    icnt_clock_mhz = 0;
    l2d_clock_mhz = 0;
    dram_clock_mhz = 0;

    /* SM Pipeline Config */
    max_registers_per_sm = 0;
    max_registers_per_cta = 0;

    /* SM Warp Config */
    max_threads_per_sm = 0;
    warp_size = 0;
    max_ctas_per_sm = 0;

    /* Pipeline Widths */
    ID_OC_SP_pipeline_width = 0;
    ID_OC_DP_pipeline_width = 0;
    ID_OC_INT_pipeline_width = 0;
    ID_OC_SFU_pipeline_width = 0;
    ID_OC_MEM_pipeline_width = 0;
    OC_EX_SP_pipeline_width = 0;
    OC_EX_DP_pipeline_width = 0;
    OC_EX_INT_pipeline_width = 0;
    OC_EX_SFU_pipeline_width = 0;
    OC_EX_MEM_pipeline_width = 0;
    EX_WB_pipeline_width = 0;
    ID_OC_TENSOR_CORE_pipeline_width = 0;
    OC_EX_TENSOR_CORE_pipeline_width = 0;

    /* Number of FUs */
    num_sp_units = 0;
    num_sfu_units = 0;
    num_dp_units = 0;
    num_int_units = 0;
    num_tensor_core_units = 0;
    num_mem_units = 0;

    /* Instruction Latencies, ADD,MAX,MUL,MAD,DIV,[SHFL] */
    opcode_latency_int = std::vector<unsigned>(6, 0);
    opcode_latency_fp = std::vector<unsigned>(5, 0);
    opcode_latency_dp = std::vector<unsigned>(5, 0);
    opcode_latency_sfu = 0;
    opcode_latency_tensor_core = 0;

    /* Initiation Intervals, ADD,MAX,MUL,MAD,DIV,[SHFL] */
    opcode_initiation_interval_int = std::vector<unsigned>(6, 0);
    opcode_initiation_interval_fp = std::vector<unsigned>(5, 0);
    opcode_initiation_interval_dp = std::vector<unsigned>(5, 0);
    opcode_initiation_interval_sfu = 0;
    opcode_initiation_interval_tensor_core = 0;

    /* Sub Core Model, warp schedulers are isolated */
    sub_core_model = false;

    /* Generic Operand Collectors */
    operand_collector_num_units_gen = 0;
    operand_collector_num_in_ports_gen = 0;
    operand_collector_num_out_ports_gen = 0;

    /* Register Banks */
    num_reg_banks = 0;
    reg_file_port_throughput = 0;

    /* Shared Memory Bankconflict Detection */
    shmem_num_banks = 0;
    shmem_limited_broadcast = 0;
    shmem_warp_parts = 0;
    coalesce_arch = 0;

    /* Warp Schedulers */
    inst_fetch_throughput = 0;
    num_sched_per_sm = 0;
    max_insn_issue_per_warp = 0;
    dual_issue_diff_exec_units = false;

    /* L1/Shared Memory Configuration */
    /* Size of L1 cache + shared memory (KB) */
    unified_l1d_size = 0;
    /* The number of L1 cache banks */
    l1d_cache_banks = 0;
    /* L1D Cache */
    l1d_cache_sets = 0;
    l1d_cache_block_size = 0;
    l1d_cache_associative = 0;
    l1d_latency = 0;

    /* Shared memory */
    shmem_size_per_sm = 0;
    shmem_size_per_cta = 0;
    shmem_latency = 0;

    /* L2 Configuration */
    l2d_size_per_sub_partition = 0;
    l2d_cache_sets = 0;
    l2d_cache_block_size = 0;
    l2d_cache_associative = 0;

    /* Maximum length of icnt-to-L2, L2-to-dram, dram-to-L2, and L2-to-icnt in a 
     * memory sub-partition. */
    dram_partition_queues_icnt_to_l2 = 0;
    dram_partition_queues_l2_to_dram = 0;
    dram_partition_queues_dram_to_l2 = 0;
    dram_partition_queues_l2_to_icnt = 0;

    /* Cluster Ejection Buffer */
    num_pkts_cluster_ejection_buffer = 0;

    /* Interconnection */
    icnt_in_buffer_limit = 0;
    icnt_out_buffer_limit = 0;
    icnt_subnets = 0;
    icnt_flit_size = 0;

    /* DRAM Configuration */
    dram_latency = 0;

    /* Trace OpCode Latency and Initiation Interval */
    // trace_opcode_latency_initiation_int = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_sp = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_dp = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_sfu = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_tensor = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_spec_op_1 = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_spec_op_2 = std::vector<unsigned>(2, 0);
    // trace_opcode_latency_initiation_spec_op_3 = std::vector<unsigned>(2, 0);
    m_specialized_unit_size = 0;
    m_specialized_unit_1_enabled = false;
    m_specialized_unit_2_enabled = false;
    m_specialized_unit_3_enabled = false;

    m_specialized_unit_1_max_latency = 0;
    m_specialized_unit_2_max_latency = 0;
    m_specialized_unit_3_max_latency = 0;

    ID_OC_specialized_unit_1_pipeline_width = 0;
    OC_EX_specialized_unit_1_pipeline_width = 0;
    ID_OC_specialized_unit_2_pipeline_width = 0;
    OC_EX_specialized_unit_2_pipeline_width = 0;
    ID_OC_specialized_unit_3_pipeline_width = 0;
    OC_EX_specialized_unit_3_pipeline_width = 0;

    num_specialized_unit_1_units = 0;
    num_specialized_unit_2_units = 0;
    num_specialized_unit_3_units = 0;

    m_specialized_unit_1_name = "";
    m_specialized_unit_2_name = "";
    m_specialized_unit_3_name = "";

    init(config_file);

    // std::cout << "````trace_opcode_latency_initiation_int.size(): " 
    //           << trace_opcode_latency_initiation_int.size() << std::endl;
  }

  /* Read from QV100.config, and assign configs to entries. */
  void init(const std::string config_file);

  /* Parse "a,b,c,d,e,f" into std::vector<unsigned> x = {a, b, c, d, e, f}. */
  std::vector<unsigned> parse_value(std::string value);

  std::vector<unsigned> parse_value_spec_unit(std::string value);

  /* Getters */
  unsigned get_stack_size_limit() const { return stack_size_limit; }
  unsigned get_heap_size_limit() const { return heap_size_limit; }
  unsigned get_kernel_launch_latency() const { return kernel_launch_latency; }
  unsigned get_thread_block_launch_latency() const {
    return thread_block_launch_latency;
  }
  unsigned get_max_concurent_kernel() const { return max_concurent_kernel; }
  unsigned get_num_clusters() const { return num_clusters; }
  unsigned get_num_sms_per_cluster() const { return num_sms_per_cluster; }
  unsigned get_num_memory_controllers() const {
    return num_memory_controllers;
  }
  unsigned get_num_sub_partition_per_memory_channel() const {
    return num_sub_partition_per_memory_channel;
  }
  float get_core_clock_mhz() const { return core_clock_mhz; }
  float get_icnt_clock_mhz() const { return icnt_clock_mhz; }
  float get_l2d_clock_mhz() const { return l2d_clock_mhz; }
  float get_dram_clock_mhz() const { return dram_clock_mhz; }
  unsigned get_max_registers_per_sm() const { return max_registers_per_sm; }
  unsigned get_max_registers_per_cta() const { return max_registers_per_cta; }
  unsigned get_max_threads_per_sm() const { return max_threads_per_sm; }
  unsigned get_warp_size() const { return warp_size; }
  unsigned get_max_ctas_per_sm() const { return max_ctas_per_sm; }
  unsigned get_max_warps_per_sm() const { return max_warps_per_sm; }
  
  unsigned get_ID_OC_SP_pipeline_width() const {
    return ID_OC_SP_pipeline_width;
  }
  unsigned get_ID_OC_DP_pipeline_width() const {
    return ID_OC_DP_pipeline_width;
  }
  unsigned get_ID_OC_INT_pipeline_width() const {
    return ID_OC_INT_pipeline_width;
  }
  unsigned get_ID_OC_SFU_pipeline_width() const {
    return ID_OC_SFU_pipeline_width;
  }
  unsigned get_ID_OC_MEM_pipeline_width() const {
    return ID_OC_MEM_pipeline_width;
  }
  unsigned get_OC_EX_SP_pipeline_width() const {
    return OC_EX_SP_pipeline_width;
  }
  unsigned get_OC_EX_DP_pipeline_width() const {
    return OC_EX_DP_pipeline_width;
  }
  unsigned get_OC_EX_INT_pipeline_width() const {
    return OC_EX_INT_pipeline_width;
  }
  unsigned get_OC_EX_SFU_pipeline_width() const {
    return OC_EX_SFU_pipeline_width;
  }
  unsigned get_OC_EX_MEM_pipeline_width() const {
    return OC_EX_MEM_pipeline_width;
  }
  unsigned get_EX_WB_pipeline_width() const { return EX_WB_pipeline_width; }
  unsigned get_ID_OC_TENSOR_CORE_pipeline_width() const {
    return ID_OC_TENSOR_CORE_pipeline_width;
  }
  unsigned get_OC_EX_TENSOR_CORE_pipeline_width() const {
    return OC_EX_TENSOR_CORE_pipeline_width;
  }
  
  unsigned get_pipe_widths(enum pipeline_stage_name_t pipeline_stage_name) const {
    switch (pipeline_stage_name) {
      case ID_OC_SP:
        return ID_OC_SP_pipeline_width;
      case ID_OC_DP:
        return ID_OC_DP_pipeline_width;
      case ID_OC_INT:
        return ID_OC_INT_pipeline_width;
      case ID_OC_SFU:
        return ID_OC_SFU_pipeline_width;
      case ID_OC_MEM:
        return ID_OC_MEM_pipeline_width;
      case OC_EX_SP:
        return OC_EX_SP_pipeline_width;
      case OC_EX_DP:
        return OC_EX_DP_pipeline_width;
      case OC_EX_INT:
        return OC_EX_INT_pipeline_width;
      case OC_EX_SFU:
        return OC_EX_SFU_pipeline_width;
      case OC_EX_MEM:
        return OC_EX_MEM_pipeline_width;
      case EX_WB:
        return EX_WB_pipeline_width;
      case ID_OC_TENSOR_CORE:
        return ID_OC_TENSOR_CORE_pipeline_width;
      case OC_EX_TENSOR_CORE:
        return OC_EX_TENSOR_CORE_pipeline_width;
      default:
        assert(0);
    }
  }

  std::string get_m_specialized_unit_name(unsigned index) {
    switch (index) {
      case 0:
        return m_specialized_unit_1_name;
      case 1:
        return m_specialized_unit_2_name;
      case 2:
        return m_specialized_unit_3_name;
      default:
        assert(0);
    }
  }

  unsigned get_pipe_widths_ID_OC_spec_unit(unsigned index) {
    switch (index) {
      case 0:
        return ID_OC_specialized_unit_1_pipeline_width;
      case 1:
        return ID_OC_specialized_unit_2_pipeline_width;
      case 2:
        return ID_OC_specialized_unit_3_pipeline_width;
      default:
        assert(0);
    }
  }

  unsigned get_pipe_widths_OC_EX_spec_unit(unsigned index) {
    switch (index) {
      case 0:
        return OC_EX_specialized_unit_1_pipeline_width;
      case 1:
        return OC_EX_specialized_unit_2_pipeline_width;
      case 2:
        return OC_EX_specialized_unit_3_pipeline_width;
      default:
        assert(0);
    }
  }

  std::string get_pipeline_stage_name_decode(
    enum pipeline_stage_name_t pipeline_stage_name) const {
    return std::string(pipeline_stage_name_decode[pipeline_stage_name]);
  }

  unsigned get_num_sp_units() const { return num_sp_units; }
  unsigned get_num_sfu_units() const { return num_sfu_units; }
  unsigned get_num_dp_units() const { return num_dp_units; }
  unsigned get_num_int_units() const { return num_int_units; }
  unsigned get_num_tensor_core_units() const { return num_tensor_core_units; }
  unsigned get_num_mem_units() const { return num_mem_units; }
  unsigned get_opcode_latency_int(opcode_operation op) const {
    return opcode_latency_int[op];
  }
  unsigned get_opcode_latency_fp(opcode_operation op) const {
    return opcode_latency_fp[op];
  }
  unsigned get_opcode_latency_dp(opcode_operation op) const {
    return opcode_latency_dp[op];
  }
  unsigned get_opcode_latency_sfu() const {
    return opcode_latency_sfu;
  }
  unsigned get_opcode_latency_tensor_core() const {
    return opcode_latency_tensor_core;
  }
  unsigned get_opcode_initiation_interval_int(opcode_operation op) const {
    return opcode_initiation_interval_int[op];
  }
  unsigned get_opcode_initiation_interval_fp(opcode_operation op) const {
    return opcode_initiation_interval_fp[op];
  }
  unsigned get_opcode_initiation_interval_dp(opcode_operation op) const {
    return opcode_initiation_interval_dp[op];
  }
  unsigned get_opcode_initiation_interval_sfu() const {
    return opcode_initiation_interval_sfu;
  }
  unsigned get_opcode_initiation_interval_tensor_core() const {
    return opcode_initiation_interval_tensor_core;
  }
  bool get_sub_core_model() const { return sub_core_model; }
  unsigned get_operand_collector_num_units_gen() const {
    return operand_collector_num_units_gen;
  }
  unsigned get_operand_collector_num_in_ports_gen() const {
    return operand_collector_num_in_ports_gen;
  }
  unsigned get_operand_collector_num_out_ports_gen() const {
    return operand_collector_num_out_ports_gen;
  }
  unsigned get_num_reg_banks() const { return num_reg_banks; }
  unsigned get_reg_file_port_throughput() const {
    return reg_file_port_throughput;
  }
  unsigned get_bank_warp_shift() const { return bank_warp_shift; }
  unsigned get_shmem_num_banks() const { return shmem_num_banks; }
  unsigned get_shmem_limited_broadcast() const {
    return shmem_limited_broadcast;
  }
  unsigned get_shmem_warp_parts() const { return shmem_warp_parts; }
  unsigned get_coalesce_arch() const { return coalesce_arch; }
  unsigned get_inst_fetch_throughput() const { return inst_fetch_throughput; }
  unsigned get_num_sched_per_sm() const { return num_sched_per_sm; }
  unsigned get_max_insn_issue_per_warp() const {
    return max_insn_issue_per_warp;
  }
  bool get_dual_issue_diff_exec_units() const {
    return dual_issue_diff_exec_units;
  }
  unsigned get_unified_l1d_size() const { return unified_l1d_size; }
  unsigned get_l1d_cache_banks() const { return l1d_cache_banks; }
  unsigned get_l1d_cache_sets() const { return l1d_cache_sets; }
  unsigned get_l1d_cache_block_size() const { return l1d_cache_block_size; }
  unsigned get_l1d_cache_associative() const { return l1d_cache_associative; }
  unsigned get_l1d_latency() const { return l1d_latency; }
  unsigned get_shmem_size_per_sm() const { return shmem_size_per_sm; }
  unsigned get_shmem_size_per_cta() const { return shmem_size_per_cta; }
  unsigned get_shmem_latency() const { return shmem_latency; }
  unsigned get_l2d_size_per_sub_partition() const {
    return l2d_size_per_sub_partition;
  }
  unsigned get_l2d_cache_sets() const { return l2d_cache_sets; }
  unsigned get_l2d_cache_block_size() const { return l2d_cache_block_size; }
  unsigned get_l2d_cache_associative() const { return l2d_cache_associative; }
  unsigned get_dram_partition_queues_icnt_to_l2() const {
    return dram_partition_queues_icnt_to_l2;
  }
  unsigned get_dram_partition_queues_l2_to_dram() const {
    return dram_partition_queues_l2_to_dram;
  }
  unsigned get_dram_partition_queues_dram_to_l2() const {
    return dram_partition_queues_dram_to_l2;
  }
  unsigned get_dram_partition_queues_l2_to_icnt() const {
    return dram_partition_queues_l2_to_icnt;
  }
  unsigned get_num_pkts_cluster_ejection_buffer() const {
    return num_pkts_cluster_ejection_buffer;
  }
  unsigned get_icnt_in_buffer_limit() const { return icnt_in_buffer_limit; }
  unsigned get_icnt_out_buffer_limit() const { return icnt_out_buffer_limit; }
  unsigned get_icnt_subnets() const { return icnt_subnets; }
  unsigned get_icnt_flit_size() const { return icnt_flit_size; }
  unsigned get_dram_latency() const { return dram_latency; }
  
  unsigned get_opcode_latency_initiation_int(unsigned id) const {
    // std::cout << "trace_opcode_latency_initiation_int.size(): " 
    //           << trace_opcode_latency_initiation_int.size() << std::endl;
    // std::cout << "trace_opcode_latency_initiation_int[id]: " 
    //           << trace_opcode_latency_initiation_int[0] << std::endl;
    return trace_opcode_latency_initiation_int[id];
  }
  unsigned get_opcode_latency_initiation_sp(int id) const {
    return trace_opcode_latency_initiation_sp[id];
  }
  unsigned get_opcode_latency_initiation_dp(int id) const {
    return trace_opcode_latency_initiation_dp[id];
  }
  unsigned get_opcode_latency_initiation_sfu(int id) const {
    return trace_opcode_latency_initiation_sfu[id];;
  }
  unsigned get_opcode_latency_initiation_tensor_core(int id) const {
    return trace_opcode_latency_initiation_tensor[id];
  }
  unsigned get_opcode_latency_initiation_spec_op_1(int id) const {
    return trace_opcode_latency_initiation_spec_op_1[id];
  }
  unsigned get_opcode_latency_initiation_spec_op_2(int id) const {
    return trace_opcode_latency_initiation_spec_op_2[id];
  }
  unsigned get_opcode_latency_initiation_spec_op_3(int id) const {
    return trace_opcode_latency_initiation_spec_op_3[id];
  }

  unsigned get_opcode_latency_initiation_spec_unit(unsigned index, int id) const {
    switch (index) {
      case 0:
        return trace_opcode_latency_initiation_spec_op_1[id];
      case 1:
        return trace_opcode_latency_initiation_spec_op_2[id];
      case 2:
        return trace_opcode_latency_initiation_spec_op_3[id];
      default:
        assert(0);
    }
  }

  unsigned get_specialized_unit_size() const {
    return m_specialized_unit_size;
  }

      /*
      m_specialized_unit_size = 0;
      m_specialized_unit_1_enabled = false;
      m_specialized_unit_2_enabled = false;
      m_specialized_unit_3_enabled = false;

      m_specialized_unit_1_max_latency = 0;
      m_specialized_unit_2_max_latency = 0;
      m_specialized_unit_3_max_latency = 0;

      ID_OC_specialized_unit_1_pipeline_width = 0;
      OC_EX_specialized_unit_1_pipeline_width = 0;
      ID_OC_specialized_unit_2_pipeline_width = 0;
      OC_EX_specialized_unit_2_pipeline_width = 0;
      ID_OC_specialized_unit_3_pipeline_width = 0;
      OC_EX_specialized_unit_3_pipeline_width = 0;

      num_specialized_unit_1_units = 0;
      num_specialized_unit_2_units = 0;
      num_specialized_unit_3_units = 0;

      m_specialized_unit_1_name = "";
      m_specialized_unit_2_name = "";
      m_specialized_unit_3_name = "";
      # <enabled>,<num_units>,<max_latency>,<ID_OC_SPEC>,<OC_EX_SPEC>,<NAME>
      */
  
  bool get_specialized_unit_1_enabled() const {
    return m_specialized_unit_1_enabled;
  }
  bool get_specialized_unit_2_enabled() const {
    return m_specialized_unit_2_enabled;
  }
  bool get_specialized_unit_3_enabled() const {
    return m_specialized_unit_3_enabled;
  }

  unsigned get_specialized_unit_1_max_latency() const {
    return m_specialized_unit_1_max_latency;
  }
  unsigned get_specialized_unit_2_max_latency() const {
    return m_specialized_unit_2_max_latency;
  }
  unsigned get_specialized_unit_3_max_latency() const {
    return m_specialized_unit_3_max_latency;
  }

  unsigned get_ID_OC_specialized_unit_1_pipeline_width() const {
    return ID_OC_specialized_unit_1_pipeline_width;
  }
  unsigned get_OC_EX_specialized_unit_1_pipeline_width() const {
    return OC_EX_specialized_unit_1_pipeline_width;
  }
  unsigned get_ID_OC_specialized_unit_2_pipeline_width() const {
    return ID_OC_specialized_unit_2_pipeline_width;
  }
  unsigned get_OC_EX_specialized_unit_2_pipeline_width() const {
    return OC_EX_specialized_unit_2_pipeline_width;
  }
  unsigned get_ID_OC_specialized_unit_3_pipeline_width() const {
    return ID_OC_specialized_unit_3_pipeline_width;
  }
  unsigned get_OC_EX_specialized_unit_3_pipeline_width() const {
    return OC_EX_specialized_unit_3_pipeline_width;
  }

  unsigned get_num_specialized_unit_1_units() const {
    return num_specialized_unit_1_units;
  }
  unsigned get_num_specialized_unit_2_units() const {
    return num_specialized_unit_2_units;
  }
  unsigned get_num_specialized_unit_3_units() const {
    return num_specialized_unit_3_units;
  }

  std::string get_specialized_unit_1_name() const {
    return m_specialized_unit_1_name;
  }
  std::string get_specialized_unit_2_name() const {
    return m_specialized_unit_2_name;
  }
  std::string get_specialized_unit_3_name() const {
    return m_specialized_unit_3_name;
  }

 private:
  bool m_valid;


  /* Device Limits */
  /* thread stack size */
  unsigned stack_size_limit;
  /* malloc heap size */
  unsigned heap_size_limit;
  /* kernel launch latency in cycles */
  unsigned kernel_launch_latency;
  /* thread block launch latency in cycles */
  unsigned thread_block_launch_latency;
  unsigned max_concurent_kernel;


  /* High Level Architecture Configuration */
  /* number of shader clusters */
  unsigned num_clusters;
  /* number of stream processors per shader cluster */
  unsigned num_sms_per_cluster;
  /* number of memory modules (e.g. memory controllers) in gpu */
  unsigned num_memory_controllers;
  /* number of memory subpartition in each memory module */
  unsigned num_sub_partition_per_memory_channel;


  /* Clock Domain Frequencies in MHZ */
  /* core clock domin */
  float core_clock_mhz;
  /* interconnect clock domin */
  float icnt_clock_mhz;
  /* l2 clock domin */
  float l2d_clock_mhz;
  /* dram clock domin */
  float dram_clock_mhz;


  /* SM Pipeline Config */
  /* max number of registers per SM, limits the number of concurrent CTAs */
  unsigned max_registers_per_sm;
  /* max number of registers per CTA */
  unsigned max_registers_per_cta;


  /* SM Warp Config */
  unsigned max_threads_per_sm;
  unsigned warp_size;
  unsigned max_ctas_per_sm;
  unsigned max_warps_per_sm;


  /* Pipeline Widths */
  unsigned ID_OC_SP_pipeline_width;
  unsigned ID_OC_DP_pipeline_width;
  unsigned ID_OC_INT_pipeline_width;
  unsigned ID_OC_SFU_pipeline_width;
  unsigned ID_OC_MEM_pipeline_width;
  unsigned OC_EX_SP_pipeline_width;
  unsigned OC_EX_DP_pipeline_width;
  unsigned OC_EX_INT_pipeline_width;
  unsigned OC_EX_SFU_pipeline_width;
  unsigned OC_EX_MEM_pipeline_width;
  unsigned EX_WB_pipeline_width;
  unsigned ID_OC_TENSOR_CORE_pipeline_width;
  unsigned OC_EX_TENSOR_CORE_pipeline_width;


  /* Number of FUs */
  unsigned num_sp_units;
  unsigned num_sfu_units;
  unsigned num_dp_units;
  unsigned num_int_units;
  unsigned num_tensor_core_units;
  unsigned num_mem_units;


  /* Instruction Latencies, ADD,MAX,MUL,MAD,DIV,[SHFL] */
  std::vector<unsigned> opcode_latency_int;
  std::vector<unsigned> opcode_latency_fp;
  std::vector<unsigned> opcode_latency_dp;
  unsigned opcode_latency_sfu;
  unsigned opcode_latency_tensor_core;


  /* Initiation Intervals, ADD,MAX,MUL,MAD,DIV,[SHFL] */
  std::vector<unsigned> opcode_initiation_interval_int;
  std::vector<unsigned> opcode_initiation_interval_fp;
  std::vector<unsigned> opcode_initiation_interval_dp;
  unsigned opcode_initiation_interval_sfu;
  unsigned opcode_initiation_interval_tensor_core;

  
  /* Sub Core Model, warp schedulers are isolated */
  bool sub_core_model;


  /* Generic Operand Collectors */
  unsigned operand_collector_num_units_gen;
  unsigned operand_collector_num_in_ports_gen;
  unsigned operand_collector_num_out_ports_gen;


  /* Register Banks */
  unsigned num_reg_banks;
  unsigned reg_file_port_throughput;
  unsigned bank_warp_shift;

  /* Shared Memory Bankconflict Detection */
  unsigned shmem_num_banks;
  unsigned shmem_limited_broadcast;
  unsigned shmem_warp_parts;
  unsigned coalesce_arch;


  /* Warp Schedulers */
  unsigned inst_fetch_throughput;
  /* Number of schedulers per SM. */
  unsigned num_sched_per_sm;
  /* How many instns a warp scheduler can issue per cycle. */
  unsigned max_insn_issue_per_warp;
  /* Whether dual issue only occurs with using two different execution unit or not. */
  bool dual_issue_diff_exec_units;


  /* L1/Shared Memory Configuration */
  /* Size of L1 cache + shared memory (KB) */
  unsigned unified_l1d_size;
  /* The number of L1 cache banks */
  unsigned l1d_cache_banks;
  /* L1D Cache */
  unsigned l1d_cache_sets;
  unsigned l1d_cache_block_size;
  unsigned l1d_cache_associative;
  unsigned l1d_latency;
  /* Shared memory */
  unsigned shmem_size_per_sm;
  unsigned shmem_size_per_cta;
  unsigned shmem_latency;

  /* L2 Configuration */
  unsigned l2d_size_per_sub_partition;
  unsigned l2d_cache_sets;
  unsigned l2d_cache_block_size;
  unsigned l2d_cache_associative;
  /* The maximum length of icnt-to-L2, L2-to-dram, dram-to-L2, and L2-to-icnt in a 
   * memory sub-partition. Packets of memory request enter the memory sub-partition 
   * from the interconnect through the ICNT->L2 queue. The L2 Cache Bank pops up a 
   * request from the ICNT->L2 queue for service at each L2 clock cycle. Any memory 
   * requests for off-chip DRAM generated by L2 are pushed into the L2->DRAM queue. 
   * If L2 Cache is disabled, packets will be ejected from the ICNT->L2 queue and 
   * pushed directly into the L2->DRAM queue, still at the L2 clock rate. The padd-
   * ing request returned from the off-chip DRAM is ejected from the DRAM->L2 queue 
   * and consumed by the L2 Cache Bank. Read responses from L2 to SMs are pushed 
   * through the L2->ICNT queue. */
  unsigned dram_partition_queues_icnt_to_l2;
  unsigned dram_partition_queues_l2_to_dram;
  unsigned dram_partition_queues_dram_to_l2;
  unsigned dram_partition_queues_l2_to_icnt;


  /* Cluster Ejection Buffer */
  unsigned num_pkts_cluster_ejection_buffer;


  /* Interconnection */
  unsigned icnt_in_buffer_limit;
  unsigned icnt_out_buffer_limit;
  unsigned icnt_subnets;
  unsigned icnt_flit_size;

  /* DRAM Configuration */
  unsigned dram_latency;

  /* Trace OpCode Latency and Initiation Interval */
  std::vector<unsigned> trace_opcode_latency_initiation_int;
  std::vector<unsigned> trace_opcode_latency_initiation_sp;
  std::vector<unsigned> trace_opcode_latency_initiation_dp;
  std::vector<unsigned> trace_opcode_latency_initiation_sfu;
  std::vector<unsigned> trace_opcode_latency_initiation_tensor;
  std::vector<unsigned> trace_opcode_latency_initiation_spec_op_1;
  std::vector<unsigned> trace_opcode_latency_initiation_spec_op_2;
  std::vector<unsigned> trace_opcode_latency_initiation_spec_op_3;
  unsigned m_specialized_unit_size;
  /*
  # <enabled>,<num_units>,<max_latency>,<ID_OC_SPEC>,<OC_EX_SPEC>,<NAME>
  -gpgpu_specialized_unit_1 1,4,4,4,4,BRA
  */
  unsigned ID_OC_specialized_unit_1_pipeline_width;
  unsigned OC_EX_specialized_unit_1_pipeline_width;
  unsigned ID_OC_specialized_unit_2_pipeline_width;
  unsigned OC_EX_specialized_unit_2_pipeline_width;
  unsigned ID_OC_specialized_unit_3_pipeline_width;
  unsigned OC_EX_specialized_unit_3_pipeline_width;

  unsigned num_specialized_unit_1_units;
  unsigned num_specialized_unit_2_units;
  unsigned num_specialized_unit_3_units;

  bool m_specialized_unit_1_enabled;
  bool m_specialized_unit_2_enabled;
  bool m_specialized_unit_3_enabled;

  unsigned m_specialized_unit_1_max_latency;
  unsigned m_specialized_unit_2_max_latency;
  unsigned m_specialized_unit_3_max_latency;

  std::string m_specialized_unit_1_name;
  std::string m_specialized_unit_2_name;
  std::string m_specialized_unit_3_name;

  std::map<std::string, std::string> configs_str;

};


#endif