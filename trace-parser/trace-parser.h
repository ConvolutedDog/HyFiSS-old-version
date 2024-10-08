// developed by Mahmoud Khairy, Purdue Univ
// abdallm@purdue.edu

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <bitset>
#include <map>
#include <list>
#include <string.h>
#include <unordered_map>
#include <math.h>

// #include "option_parser.h"

#include "../ISA-Def/ampere_opcode.h"
#include "../ISA-Def/kepler_opcode.h"
#include "../ISA-Def/pascal_opcode.h"
#include "../ISA-Def/trace_opcode.h"
#include "../ISA-Def/turing_opcode.h"
#include "../ISA-Def/volta_opcode.h"

#include "../common/common_def.h"
#include "../common/vector_types.h"
#include "inst-memadd-info.h"
#include "inst-trace.h"
#include "memory-space.h"
#include "../trace-driven/kernel-trace.h"
#include "../trace-driven/trace-warp-inst.h"

#include "../trace-driven/inst-stt.h"

#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

// type of the config files
enum config_type { APP_CONFIG, INSTN_CONFIG, ISSUE_CONFIG, CONFIGS_TYPE_NUM };

class app_config {
 public:
  app_config() {
    m_valid = false;
    kernels_num = 1;
  }
  void init(std::string config_path, bool PRINT_LOG);

  /* get kernels_num */
  int get_kernels_num() { return kernels_num; }
  /* get kernel_grid_size */
  std::vector<int>* get_kernels_grid_size() { return &kernel_grid_size; }
  int get_kernel_grid_size(int kernel_id) { return kernel_grid_size[kernel_id]; }
  /* get kernel_name */
  std::vector<std::string>* get_kernels_name() { return &kernel_name; }
  std::string get_kernel_name(int kernel_id) { return kernel_name[kernel_id]; }
  /* get app_kernels_id */
  std::vector<int>* get_app_kernels_id() { return &app_kernels_id; }
  int get_app_kernel_id(int kernel_id) { return app_kernels_id[kernel_id]; }
  /* get kernel_grid_dim_x, kernel_grid_dim_y, kernel_grid_dim_z */
  std::vector<int>* get_kernels_grid_dim_x() { return &kernel_grid_dim_x; }
  int get_kernel_grid_dim_x(int kernel_id) { return kernel_grid_dim_x[kernel_id]; }
  std::vector<int>* get_kernels_grid_dim_y() { return &kernel_grid_dim_y; }
  int get_kernel_grid_dim_y(int kernel_id) { return kernel_grid_dim_y[kernel_id]; }
  std::vector<int>* get_kernels_grid_dim_z() { return &kernel_grid_dim_z; }
  int get_kernel_grid_dim_z(int kernel_id) { return kernel_grid_dim_z[kernel_id]; }
  /* get the total number of global warps */
  int get_num_global_warps(int kernel_id) {
    return (int)( (get_kernel_grid_dim_x(kernel_id) * get_kernel_grid_dim_y(kernel_id) * 
                   get_kernel_grid_dim_z(kernel_id)
                  ) * 
                  ((get_kernel_tb_dim_x(kernel_id) * 
                    get_kernel_tb_dim_y(kernel_id) * get_kernel_tb_dim_z(kernel_id) + WARP_SIZE - 1
                   ) / WARP_SIZE
                  )
                );
  }
  /* get the total number of warps per block */
  int get_num_warp_per_block(int kernel_id) {
    return (int)((get_kernel_tb_dim_x(kernel_id) * get_kernel_tb_dim_y(kernel_id) * 
                 get_kernel_tb_dim_z(kernel_id) + WARP_SIZE - 1) / WARP_SIZE);
  }

  /* get kernel_tb_dim_x,  kernel_tb_dim_y,  kernel_tb_dim_z */
  std::vector<int>* get_kernels_tb_dim_x() { return &kernel_tb_dim_x; }
  int get_kernel_tb_dim_x(int kernel_id) { return kernel_tb_dim_x[kernel_id]; }
  std::vector<int>* get_kernels_tb_dim_y() { return &kernel_tb_dim_y; }
  int get_kernel_tb_dim_y(int kernel_id) { return kernel_tb_dim_y[kernel_id]; }
  std::vector<int>* get_kernels_tb_dim_z() { return &kernel_tb_dim_z; }
  int get_kernel_tb_dim_z(int kernel_id) { return kernel_tb_dim_z[kernel_id]; }
  /* get kernel_num_registers */
  std::vector<int>* get_kernels_num_registers() { return &kernel_num_registers; }
  int get_kernel_num_registers(int kernel_id) { return kernel_num_registers[kernel_id]; }
  /* get kernel_shared_mem_bytes */
  std::vector<int>* get_kernels_shared_mem_bytes() { return &kernel_shared_mem_bytes; }
  int get_kernel_shared_mem_bytes(int kernel_id) { return kernel_shared_mem_bytes[kernel_id]; }
  /* get kernel_block_size */
  std::vector<int>* get_kernels_block_size() { return &kernel_block_size; }
  int get_kernel_block_size(int kernel_id) { return kernel_block_size[kernel_id]; }
  /* get kernel_cuda_stream_id */
  std::vector<int>* get_kernels_cuda_stream_id() { return &kernel_cuda_stream_id; }
  int get_kernel_cuda_stream_id(int kernel_id) { return kernel_cuda_stream_id[kernel_id]; }
  /* get kernel_shmem_base_addr */
  std::vector<unsigned long long>* get_kernels_shmem_base_addr() { return &kernel_shmem_base_addr; }
  unsigned long long get_kernel_shmem_base_addr(int kernel_id) { return kernel_shmem_base_addr[kernel_id]; }
  /* get kernel_local_base_addr */
  std::vector<unsigned long long>* get_kernels_local_base_addr() { return &kernel_local_base_addr; }
  unsigned long long get_kernel_local_base_addr(int kernel_id) { return kernel_local_base_addr[kernel_id]; }
  int get_concurrentKernels() { return concurrentKernels; }

 private:
  bool m_valid;
  int concurrentKernels = 0;
  std::string app_kernels_id_string;
  std::vector<int> app_kernels_id;
  int kernels_num;
  /*
    -kernel_2_name vecAdd_2
    -kernel_2_num_registers 10
    -kernel_2_shared_mem_bytes 0
    -kernel_2_grid_size 160
    -kernel_2_block_size 256
    -kernel_2_cuda_stream_id 0
  */
  std::vector<std::string> kernel_name;
  std::vector<int> kernel_num_registers;
  std::vector<int> kernel_shared_mem_bytes;
  std::vector<int> kernel_grid_size;
  std::vector<int> kernel_block_size;
  std::vector<int> kernel_cuda_stream_id;

  std::vector<int> kernel_grid_dim_x;
  std::vector<int> kernel_grid_dim_y;
  std::vector<int> kernel_grid_dim_z;
  std::vector<int> kernel_tb_dim_x;
  std::vector<int> kernel_tb_dim_y;
  std::vector<int> kernel_tb_dim_z;
  std::vector<unsigned long long> kernel_shmem_base_addr;
  std::vector<unsigned long long> kernel_local_base_addr;
};

class instn_info_t {
 public:
  instn_info_t() {}
  instn_info_t(unsigned _kernel_id, unsigned _pc, std::string _instn_str) {
    kernel_id = _kernel_id;
    pc = _pc;
    instn_str = _instn_str;


  }

 private:
  unsigned kernel_id;
  unsigned pc;
  std::string instn_str;
};

class instn_config {
 public:
  instn_config() {
    m_valid = false;
  }
  instn_config(hw_config* hw_cfg) {
    m_valid = false;
    this->hw_cfg = hw_cfg;
  }
  ~instn_config() {
    for (auto iter = instn_info_vector.begin(); iter != instn_info_vector.end(); ++iter) {
      delete iter->second;
    }
  }
  void init(std::string config_path, bool PRINT_LOG);

  std::map<std::pair<int, int>, _inst_trace_t*>* get_instn_info_vector() { return &instn_info_vector; }

  int get_instn_latency(int kernel_id, int pc) {
    auto iter = instn_info_vector.find(std::make_pair(kernel_id, pc));
    if (iter != instn_info_vector.end()) {
      return iter->second->latency;
    } else {
      return -1;
    }
  }

 private:
  bool m_valid;
  hw_config* hw_cfg;
  // instn format:
  // kernel_id  pc  instn_str
  //     2      c0  DSETP.GTU.AND P0 P7 R2 R4 P7
  // std::vector<instn_info_t> instn_info_vector;
  // std::map<std::pair<int, int>, instn_info_t*> instn_info_vector;
  std::map<std::pair<int, int>, _inst_trace_t*> instn_info_vector;
};

/* -trace_issued_sm_id_0 4,(1,0),(1,80),(2,80),(2,0), */
struct block_info_t {
  block_info_t() {}
  block_info_t(unsigned _kernel_id, unsigned _block_id, unsigned long long _time_stamp, unsigned _sm_id) {
    kernel_id = _kernel_id;
    block_id = _block_id;
    time_stamp = _time_stamp;
    sm_id = _sm_id;
  }

  unsigned kernel_id;
  unsigned block_id;
  unsigned long long time_stamp;
  unsigned sm_id;

#ifdef USE_BOOST
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & kernel_id;
    ar & block_id;
    ar & time_stamp;
    ar & sm_id;
  }
#endif
};

class issue_config {
 public:
  issue_config() {
    m_valid = false;
    trace_issued_sms_num = 1;
  }
  void init(std::string config_path, bool PRINT_LOG);

  std::vector<std::vector<block_info_t>>* get_trace_issued_sm_id_blocks() { return &trace_issued_sm_id_blocks; }
  std::vector<block_info_t>* get_trace_issued_one_sm_blocks(unsigned sm_id) { // BUG : sm_id is not index
    for (auto iter = trace_issued_sm_id_blocks.begin();                  // fixed
              iter != trace_issued_sm_id_blocks.end(); ++iter)           // fixed
      if ((*iter)[0].sm_id == sm_id)                                     // fixed
        return &(*iter);                                                 // fixed
    return NULL; 
  }

  block_info_t* get_trace_issued_one_sm_one_block(unsigned sm_id, unsigned block_id) { // BUG : sm_id is not index
    for (auto iter = trace_issued_sm_id_blocks.begin();                      // fixed
              iter != trace_issued_sm_id_blocks.end(); ++iter)               // fixed
      if ((*iter)[0].sm_id == sm_id)                                         // fixed
        for (auto iter2 = iter->begin(); iter2 != iter->end(); ++iter2)      // fixed
          if (iter2->block_id == block_id)                                   // fixed
            return &(*iter2);                                                // fixed

    return NULL;
  }

  std::vector<std::pair<int, int>> get_kernel_block_by_smid(int smid) {
    std::vector<std::pair<int, int>> result;
    for (auto iter = trace_issued_sm_id_blocks.begin();
              iter != trace_issued_sm_id_blocks.end(); ++iter) {
      if ((*iter)[0].sm_id == (unsigned)smid) {
        for (auto iter2 = iter->begin(); iter2 != iter->end(); ++iter2) {
          result.push_back(std::make_pair(iter2->kernel_id, iter2->block_id));      
        }
      }
    }
    return result;
  }

  std::vector<std::pair<int, int>> get_kernel_block_by_smid_0(int smid) {
    std::vector<std::pair<int, int>> result;
    for (auto iter = trace_issued_sm_id_blocks.begin();
              iter != trace_issued_sm_id_blocks.end(); ++iter) {
      // if ((*iter)[0].sm_id == (unsigned)smid) {
        for (auto iter2 = iter->begin(); iter2 != iter->end(); ++iter2) {
          // judge if std::make_pair(iter2->kernel_id, iter2->block_id) is in result
          bool is_in_result = false;
          for (auto iter3 = result.begin(); iter3 != result.end(); ++iter3) {
            if ((iter3->first == iter2->kernel_id) && (iter3->second == iter2->block_id)) {
              is_in_result = true;
              break;
            }
          }
          if (!is_in_result) {
            result.push_back(std::make_pair(iter2->kernel_id, iter2->block_id));
            // std::cout << "$" << smid << "$" << iter2->kernel_id << "$" << iter2->block_id << "$" << std::endl;
          }   
        }
      // }
    }
    return result;
  }

  int get_trace_issued_sms_num() { return trace_issued_sms_num; }

  int get_sm_id_of_one_block(unsigned kernel_id, unsigned block_id);
  int get_sm_id_of_one_block_fast(unsigned kernel_id, unsigned block_id);

  std::vector<int> get_trace_issued_sms_vector() { return trace_issued_sms_vector; }

 private:
  bool m_valid;
  int trace_issued_sms_num;
  
  /* Store the <kernel_id, block_id> -> sm_id pair. */
  std::map<std::pair<unsigned, unsigned>, int> trace_issued_sm_id_blocks_map;

  std::vector<block_info_t> parse_blocks_info(const std::string& blocks_info_str);
  std::vector<int> trace_issued_sms_vector = {};
  /*
    trace_issued_sm_id_blocks[0]: SMx -> block(kernel_id, block_id)
  */
  std::vector<std::string> trace_issued_sm_id_blocks_str;
  std::vector<std::vector<block_info_t>> trace_issued_sm_id_blocks;
};

/* enum of 'RED'/'ATOM'/'LDG'/'STG'/'LDL'/'STL' */
enum mem_instn_type {
  UNKOWN_TYPE = 0,
  RED,
  ATOM,
  LDG,
  STG,
  LDS,
  STS,
  LDL,
  STL,
  num_mem_instn_types,
};

struct mem_instn {
  mem_instn() {}
  mem_instn(unsigned _pc, unsigned long long _addr_start1, 
            unsigned _time_stamp, int addr_groups, 
            unsigned long long _addr_start2,
            unsigned _mask, std::string _opcode) {
    /* This function has been deprecated. */
    pc = _pc;
    time_stamp = _time_stamp;
    mask = _mask;
    std::bitset<32> active_mask(mask);
    opcode = _opcode;
    for (unsigned i = 0; i < 32; i++)
      if (active_mask.test(i))
        addr.push_back(_addr_start1 + i*8);
    if (addr_groups == 2) 
      for (unsigned i = 0; i < 32; i++) 
        if (active_mask.test(i))
          addr.push_back(_addr_start2 + i*8);
    valid = true;
    mem_access_type = has_mem_instn_type();

    distance.resize(addr.size());
    distance_L2.resize(addr.size());
    miss.resize(addr.size());
  }
  mem_instn(unsigned _pc, unsigned long long _addr_start1, 
            unsigned _time_stamp, int addr_groups, 
            unsigned long long _addr_start2,
            unsigned _mask, std::string _opcode, 
            std::vector<long long>* _stride_num) {
    pc = _pc;
    time_stamp = _time_stamp;
    mask = _mask;
    std::bitset<32> active_mask(mask);
    opcode = _opcode;
    valid = true;
    mem_access_type = has_mem_instn_type();

    unsigned long long last_addr;
    for (unsigned i = 0; i < 32; i++) {
      if (i == 0) {
        last_addr = _addr_start1;
      } else {
        last_addr += (*_stride_num)[i-1];
      }
      if (active_mask.test(i))
        addr.push_back(last_addr);
    }
    if (addr_groups == 2) {
      for (unsigned i = 0; i < 32; i++) {
        if (i == 0) {
          last_addr = _addr_start2;
        } else {
          last_addr += (*_stride_num)[31 + i-1];
        }
        if (active_mask.test(i))
          addr.push_back(last_addr);
      }
    }

    distance.resize(addr.size());
    distance_L2.resize(addr.size());
    miss.resize(addr.size());
  }

  unsigned pc;
  std::vector<unsigned long long> addr;
  unsigned time_stamp;
  bool valid = false;
  unsigned mask;
  std::string opcode;
  enum mem_instn_type mem_access_type;

  std::vector<int> distance;
  std::vector<int> distance_L2;
  std::vector<bool> miss;

  /* Here we will judge if or not the opcode has 'RED'/'ATOM'/'LDG'/'STG'/'LDL'/'STL'.
   * 1. RED: global reduction operations, Reduction Operation on generic Memory.
   * 2. ATOM: global atomic operations, Atomic Operation on generic Memory.
   * 3. LDG: load from global memory access.
   * 4. STG: store to global memory access.
   * 5. LDL: load from local memory access.
   * 6. STL: store to local memory access. */
  enum mem_instn_type has_mem_instn_type() {
    if (opcode.find("RED") != std::string::npos) return RED;
    else if (opcode.find("ATOM") != std::string::npos) return ATOM;
    else if (opcode.find("LDG") != std::string::npos) return LDG;
    else if (opcode.find("STG") != std::string::npos) return STG;
    else if (opcode.find("LDS") != std::string::npos) return LDS;
    else if (opcode.find("STS") != std::string::npos) return STS;
    else if (opcode.find("LDL") != std::string::npos) return LDL;
    else if (opcode.find("STL") != std::string::npos) return STL;
    else return UNKOWN_TYPE;
  }

#ifdef USE_BOOST
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & pc;
    ar & addr;
    ar & time_stamp;
    ar & mask;
    ar & opcode;
    ar & valid;
    ar & mem_access_type;
  }
#endif
};


struct compute_instn {
  compute_instn() {}
  compute_instn(unsigned _kernel_id, unsigned _pc,
                unsigned _mask, unsigned _gwarp_id) {
    kernel_id = _kernel_id;
    pc = _pc;
    mask = _mask;
    std::bitset<32> active_mask(mask);
    gwarp_id = _gwarp_id;
    // need to point to _inst_trace_t.
    inst_trace = NULL;

    valid = true;
  }
  compute_instn(unsigned _kernel_id, unsigned _pc,
                unsigned _mask, unsigned _gwarp_id,
                _inst_trace_t* _inst_trace) {
    kernel_id = _kernel_id;
    pc = _pc;
    mask = _mask;
    std::bitset<32> active_mask(mask);
    gwarp_id = _gwarp_id;
    // need to point to _inst_trace_t.
    inst_trace = _inst_trace;
    inst_trace->mask = _mask;
    
    // if (kernel_id == 2 && pc != 0) {
    //   // compute_instn's kernel_id starts from 0
    //   std::cout << "compute_instn: " << std::dec << kernel_id << " " 
    //             << std::hex << pc << " " << std::hex << mask << " " 
    //             << std::dec << gwarp_id << std::endl;
    //   // inst_trace's kernel_id starts from 0
    //   std::cout << "instn_str: " << std::dec << inst_trace->kernel_id << " " 
    //             << std::hex << inst_trace->m_pc << " " 
    //             << inst_trace->instn_str << std::endl;
    //   exit(0);
    // }
    
    valid = true;
  }
  ~compute_instn(){ /*delete trace_warp_inst;*/ }
  compute_instn(unsigned _kernel_id, unsigned _pc,
                unsigned _mask, unsigned _gwarp_id,
                _inst_trace_t* _inst_trace,
                trace_warp_inst_t* _trace_warp_inst) {
    kernel_id = _kernel_id;
    pc = _pc;
    mask = _mask;
    std::bitset<32> active_mask(mask);
    gwarp_id = _gwarp_id;
    // need to point to _inst_trace_t.
    inst_trace = _inst_trace;
    inst_trace->mask = _mask;
    // trace_warp_inst = _trace_warp_inst;
    trace_warp_inst = trace_warp_inst_t();
    // trace_warp_inst->parse_from_trace_struct(_inst_trace, &Volta_OpcodeMap, gwarp_id);
    // trace_warp_inst = *_trace_warp_inst;
    trace_warp_inst.parse_from_trace_struct(_inst_trace, &Volta_OpcodeMap, gwarp_id);
    
    // if (kernel_id == 2 && pc != 0) {
    //   // compute_instn's kernel_id starts from 0
    //   std::cout << "compute_instn: " << std::dec << kernel_id << " " 
    //             << std::hex << pc << " " << std::hex << mask << " " 
    //             << std::dec << gwarp_id << std::endl;
    //   // inst_trace's kernel_id starts from 0
    //   std::cout << "instn_str: " << std::dec << inst_trace->kernel_id << " " 
    //             << std::hex << inst_trace->m_pc << " " 
    //             << inst_trace->instn_str << std::endl;
    //   exit(0);
    // }

    inst_stt_ptr = inst_stt();
    
    valid = true;
  }

  bool valid = false;
  unsigned kernel_id, pc;
  unsigned mask;

  // unsigned cta_id_x;
  // unsigned cta_id_y;
  // unsigned cta_id_z;

  // unsigned warp_id;
  // unsigned sm_id;

  unsigned gwarp_id;

  _inst_trace_t* inst_trace;
  // trace_warp_inst_t* trace_warp_inst;
  trace_warp_inst_t trace_warp_inst;

  // inst_stt
  inst_stt inst_stt_ptr;

#ifdef USE_BOOST
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & pc;
    ar & kernel_id;
    // ar & cta_id_x;
    // ar & cta_id_y;
    // ar & cta_id_z;
    // ar & warp_id;
    // ar & sm_id;
    ar & mask;
    ar & gwarp_id;
    ar & valid;
  }
#endif
};


class trace_parser {
 public:
  trace_parser(const char *input_configs_filepath);
  trace_parser(const char *input_configs_filepath, hw_config* hw_cfg) {
    configs_filepath = input_configs_filepath;
    this->hw_cfg = hw_cfg;
    m_valid = true;
  }
  
  void parse_configs_file(bool PRINT_LOG);
  void process_configs_file(std::string config_path, int config_type, bool PRINT_LOG);
  void judge_concurrent_issue();

  void read_mem_instns(bool PRINT_LOG, std::vector<std::pair<int, int>>* x);
  void process_mem_instns(std::string mem_instns_filepath, bool PRINT_LOG, std::vector<std::pair<int, int>>* x);

  void read_compute_instns(bool PRINT_LOG, std::vector<std::pair<int, int>>* x);
  void process_compute_instns(std::string compute_instns_dir, bool PRINT_LOG, std::vector<std::pair<int, int>>* x);
  void process_compute_instns_fast(std::string compute_instns_dir, bool PRINT_LOG, std::vector<std::pair<int, int>>* x);

  // kerneltraces_filepath is path to kernel-1.traceg et al.
  kernel_trace_t* parse_kernel_info(const std::string &kerneltraces_filepath);
  kernel_trace_t* parse_kernel_info(int kernel_id, bool PRINT_LOG);

  void parse_memcpy_info(const std::string &memcpy_command, size_t &add,
                         size_t &count);

  std::vector<std::vector<inst_trace_t> *> get_next_threadblock_traces(
      unsigned trace_version, unsigned enable_lineinfo, std::ifstream *ifs,
      std::string kernel_name,
      unsigned kernel_id, unsigned num_warps_per_thread_block);

  void kernel_finalizer(kernel_trace_t *trace_info);

  // get cfg
  app_config* get_appcfg() { return &appcfg; }
  instn_config* get_instncfg() { return &instncfg; }
  issue_config* get_issuecfg() { return &issuecfg; }
  
  /* get mme_instns */
  std::vector<std::vector<std::vector<mem_instn>>>& get_mem_instns() { return mem_instns; }
  std::vector<std::vector<mem_instn>>& get_one_kernel_mem_instns(int kernel_id) { return mem_instns[kernel_id]; }
  std::vector<mem_instn>& get_one_kernel_one_threadblcok_mem_instns(int kernel_id, int block_id) { 
    return mem_instns[kernel_id][block_id]; 
  }

  /* get concurrent_kernels */
  std::vector<std::vector<int>>& get_concurrent_kernels() { return concurrent_kernels; }

  compute_instn* get_one_kernel_one_warp_one_instn(int kernel_id, int warp_id, int next_instn_id) {
    return &conpute_instns[kernel_id][warp_id][next_instn_id];
  }

  unsigned get_one_kernel_one_warp_one_instn_max_size(int kernel_id, int warp_id) {
    return conpute_instns[kernel_id][warp_id].size();
  }

  mem_instn* get_one_kernel_one_block_one_uid_mem_instn(int kernel_id, int gwarp_id, int uid) {
    unsigned block_id = (unsigned) (gwarp_id / appcfg.get_num_warp_per_block(kernel_id));
    return &mem_instns[kernel_id][block_id][uid];
  }

  unsigned get_one_kernel_one_warp_instn_count(int kernel_id, int warp_id) {
    return conpute_instns[kernel_id][warp_id].size();
  }

  unsigned get_one_kernel_one_warp_instn_size(int kernel_id, int warp_id) {
    return conpute_instns[kernel_id][warp_id].size();
  }

  bool get_m_valid() { return m_valid; }

  unsigned get_the_least_sm_id_of_all_blocks() {
    // find the least sm_id from issuecfg.get_trace_issued_sms_vector()
    std::vector<int> trace_issued_sms = issuecfg.get_trace_issued_sms_vector();
    unsigned least_sm_id = trace_issued_sms[0];
    for (auto iter = trace_issued_sms.begin(); iter != trace_issued_sms.end(); ++iter) {
      if (*iter < least_sm_id) {
        least_sm_id = *iter;
      }
    }
    return least_sm_id;
  }

 private:
  /* configs_filepath is path to kernelslist.g */
  std::string configs_filepath;
  std::string app_config_path;
  std::string instn_config_path;
  std::string issue_config_path;

  std::string mem_instns_dir;
  std::string compute_instns_dir;

  app_config appcfg;
  instn_config instncfg; 
  issue_config issuecfg;
  hw_config* hw_cfg;

  /* kernel_id -> block_id -> mem_instn */
  std::vector<std::vector<std::vector<mem_instn>>> mem_instns;
  /* kernel_id -> warp_id -> compute_instn */
  std::vector<std::vector<std::vector<compute_instn>>> conpute_instns;

  /* concurrent idx -> vector<kernel_id> */
  std::vector<std::vector<int>> concurrent_kernels;

  bool m_valid = false;
};

#endif
