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

// #include "option_parser.h"

#include "../ISA-Def/ampere_opcode.h"
#include "../ISA-Def/kepler_opcode.h"
#include "../ISA-Def/pascal_opcode.h"
#include "../ISA-Def/trace_opcode.h"
#include "../ISA-Def/turing_opcode.h"
#include "../ISA-Def/volta_opcode.h"

#include "../common/common_def.h"


#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#define WARP_SIZE 32
#define MAX_DST 1
#define MAX_SRC 4


#define MAX_KERNELS_NUM 300


enum command_type {
  kernel_launch = 1,
  cpu_gpu_mem_copy,
  gpu_cpu_mem_copy,
};

enum address_space { GLOBAL_MEM = 1, SHARED_MEM, LOCAL_MEM, TEX_MEM };

enum address_scope {
  L1_CACHE = 1,
  L2_CACHE,
  SYS_MEM,
};

enum address_format { list_all = 0, base_stride = 1, base_delta = 2 };

struct trace_command {
  std::string command_string;
  command_type m_type;
};

class inst_memadd_info_t {
 public:
  uint64_t addrs[WARP_SIZE];
  int32_t width = 0;
  bool empty = true;

  void base_stride_decompress(unsigned long long base_address, int stride,
                              const std::bitset<WARP_SIZE> &mask);
  void base_delta_decompress(unsigned long long base_address,
                             const std::vector<long long> &deltas,
                             const std::bitset<WARP_SIZE> &mask);
};

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
};

struct kernel_trace_t {
  kernel_trace_t();

  std::string kernel_name;
  unsigned kernel_id;
  unsigned grid_dim_x;
  unsigned grid_dim_y;
  unsigned grid_dim_z;
  unsigned tb_dim_x;
  unsigned tb_dim_y;
  unsigned tb_dim_z;
  unsigned shmem;
  unsigned nregs;
  unsigned long cuda_stream_id;
  unsigned binary_verion;
  unsigned enable_lineinfo;
  unsigned trace_verion;
  std::string nvbit_verion;
  unsigned long long shmem_base_addr;
  unsigned long long local_base_addr;
  // Reference to open filestream
  std::ifstream *ifs;
};

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
    return (int)(get_kernel_grid_dim_x(kernel_id) * get_kernel_grid_dim_y(kernel_id) * 
                 get_kernel_grid_dim_z(kernel_id) * get_kernel_tb_dim_x(kernel_id) * 
                 get_kernel_tb_dim_y(kernel_id) * get_kernel_tb_dim_z(kernel_id) / WARP_SIZE);
  }
  /* get the total number of warps per block */
  int get_num_warp_per_block(int kernel_id) {
    return (int)(get_kernel_tb_dim_x(kernel_id) * get_kernel_tb_dim_y(kernel_id) * 
                 get_kernel_tb_dim_z(kernel_id) / WARP_SIZE);
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
  ~instn_config() {
    for (auto iter = instn_info_vector.begin(); iter != instn_info_vector.end(); ++iter) {
      delete iter->second;
    }
  }
  void init(std::string config_path, bool PRINT_LOG);

 private:
  bool m_valid;

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

  int get_trace_issued_sms_num() { return trace_issued_sms_num; }

  int get_sm_id_of_one_block(unsigned kernel_id, unsigned block_id);

 private:
  bool m_valid;
  int trace_issued_sms_num;
  

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
  }

  unsigned pc;
  std::vector<unsigned long long> addr;
  unsigned time_stamp;
  bool valid = false;
  unsigned mask;
  std::string opcode;
  enum mem_instn_type mem_access_type;

  std::vector<int> distance;

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

    valid = true;
  }

  bool valid = false;
  unsigned kernel_id, pc;
  unsigned mask;
  
  unsigned cta_id_x;
  unsigned cta_id_y;
  unsigned cta_id_z;

  unsigned warp_id;
  unsigned sm_id;

  unsigned gwarp_id;

#ifdef USE_BOOST
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & pc;
    ar & kernel_id;
    ar & cta_id_x;
    ar & cta_id_y;
    ar & cta_id_z;
    ar & warp_id;
    ar & sm_id;
    ar & mask;
    ar & gwarp_id;
    ar & valid;
  }
#endif
};


class trace_parser {
 public:
  trace_parser(const char *input_configs_filepath);
  
  std::pair<std::vector<trace_command>, int> parse_commandlist_file();

  void parse_configs_file(bool PRINT_LOG);
  void process_configs_file(std::string config_path, int config_type, bool PRINT_LOG);
  void judge_concurrent_issue();

  void read_mem_instns(bool PRINT_LOG, std::vector<std::pair<int, int>>* x);
  void process_mem_instns(std::string mem_instns_filepath, bool PRINT_LOG, std::vector<std::pair<int, int>>* x);

  void read_compute_instns(bool PRINT_LOG, std::vector<std::pair<int, int>>* x);
  void process_compute_instns(std::string compute_instns_dir, bool PRINT_LOG, std::vector<std::pair<int, int>>* x);

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
  /* kernel_id -> block_id -> mem_instn */
  std::vector<std::vector<std::vector<mem_instn>>> mem_instns;
  /* kernel_id -> warp_id -> compute_instn */
  std::vector<std::vector<std::vector<compute_instn>>> conpute_instns;

  /* concurrent idx -> vector<kernel_id> */
  std::vector<std::vector<int>> concurrent_kernels;
};

/*
Here we are processing SASS instructions, reading the file kernel-x.traceg, one instruction per 
line. Here we need to create a data structure that stores the PC value to instruction of each 
line when reading the file. We use a dictionary pc_to_sassStr to storage this. In addition, for 
convenience, we use a vector to store the PC values that have read, so that it is convenient to 
see if there are duplicate PCs value.
*/
struct sass_inst_t {
  std::string insnStr;
  std::string kernel_name;
  unsigned kernel_id;

  unsigned line_num;
  unsigned m_pc;
  unsigned mask;
  unsigned reg_dsts_num;
  unsigned reg_dest[MAX_DST];
  std::string opcode;
  unsigned reg_srcs_num;
  unsigned reg_src[MAX_SRC];

  std::string m_source_file;
  unsigned m_source_line;

  const char *source_file() const { return m_source_file.c_str(); }
  unsigned source_line() const { return m_source_line; }
  
  bool m_empty=true;
};

extern std::map<unsigned, sass_inst_t> pc_to_sassStr;
extern std::vector<int> have_readed_insn_pcs;

sass_inst_t find_sass_inst_by_pc(unsigned pc);

#endif
