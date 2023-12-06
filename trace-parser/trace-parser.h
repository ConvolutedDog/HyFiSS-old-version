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
#include "../common/vector_types.h"


#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#define WARP_SIZE 32
#define MAX_DST 1
#define MAX_SRC 4

// the maximum number of destination, source, or address uarch operands in a
// instruction
#define MAX_REG_OPERANDS 32

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


#define MAX_WARP_PER_SHADER 64

const unsigned MAX_WARP_SIZE = 32;
typedef std::bitset<MAX_WARP_SIZE> active_mask_t;

const unsigned MAX_ACCESSES_PER_INSN_PER_THREAD = 8;

typedef unsigned long long new_addr_type;

const unsigned MAX_MEMORY_ACCESS_SIZE = 128;
typedef std::bitset<MAX_MEMORY_ACCESS_SIZE> mem_access_byte_mask_t;

const unsigned SECTOR_CHUNCK_SIZE = 4;  // four sectors
const unsigned SECTOR_SIZE = 32;        // sector is 32 bytes width
typedef std::bitset<SECTOR_CHUNCK_SIZE> mem_access_sector_mask_t;

enum _memory_op_t { no_memory_op = 0, memory_load, memory_store };

enum mem_operation_t { NOT_TEX, TEX };
typedef enum mem_operation_t mem_operation;


// enum uarch_operand_type_t { UN_OP = -1, INT_OP, FP_OP };
// typedef enum uarch_operand_type_t types_of_operands;

// After expanding the vector input and output operands
#define MAX_INPUT_VALUES 24
#define MAX_OUTPUT_VALUES 8

// the maximum number of destination, source, or address uarch operands in a
// instruction
#define MAX_REG_OPERANDS 32

#define MEM_ACCESS_TYPE_TUP_DEF                                         \
  MA_TUP_BEGIN(mem_access_type)                                         \
  MA_TUP(GLOBAL_ACC_R), MA_TUP(LOCAL_ACC_R), MA_TUP(CONST_ACC_R),       \
      MA_TUP(TEXTURE_ACC_R), MA_TUP(GLOBAL_ACC_W), MA_TUP(LOCAL_ACC_W), \
      MA_TUP(L1_WRBK_ACC), MA_TUP(L2_WRBK_ACC), MA_TUP(INST_ACC_R),     \
      MA_TUP(L1_WR_ALLOC_R), MA_TUP(L2_WR_ALLOC_R),                     \
      MA_TUP(NUM_MEM_ACCESS_TYPE) MA_TUP_END(mem_access_type)

#define MA_TUP_BEGIN(X) enum X {
#define MA_TUP(X) X
#define MA_TUP_END(X) \
  }                   \
  ;
enum mem_access_type {
  GLOBAL_ACC_R,
  LOCAL_ACC_R,
  CONST_ACC_R,
  TEXTURE_ACC_R,
  GLOBAL_ACC_W,
  LOCAL_ACC_W,
  L1_WRBK_ACC,
  L2_WRBK_ACC,
  INST_ACC_R,
  L1_WR_ALLOC_R,
  L2_WR_ALLOC_R,
  NUM_MEM_ACCESS_TYPE
};
#undef MA_TUP_BEGIN
#undef MA_TUP
#undef MA_TUP_END

enum _memory_space_t {
  undefined_space = 0,
  reg_space,
  local_space,
  shared_space,
  sstarr_space,
  param_space_unclassified,
  param_space_kernel, /* global to all threads in a kernel : read-only */
  param_space_local,  /* local to a thread : read-writable */
  const_space,
  tex_space,
  surf_space, // render surfaces 
  global_space,
  generic_space,
  instruction_space
};

/*
缓存运算符的类型。
PTX ISA 2.0版在加载和存储指令上引入了可选的缓存运算符。缓存运算符需要sm_20或更高的目标体系结构。加
载或存储指令上的缓存运算符仅被视为性能提示。对ld或st指令使用缓存运算符不会改变程序的内存一致性行为。
对于sm_20及更高版本，缓存运算符具有以下定义和行为：
1. 内存Load指令的缓存运算符：
 .ca: Cache at all levels，所有级别的缓存，可能会再次访问。
      默认的Loaf指令缓存操作是ld.ca，它使用正常的逐出策略在所有级别（L1和L2）中分配缓存线。全局数
      据在L2级是一致的，但多个L1缓存对于全局数据来说是不一致的。如果一个线程通过一个L1缓存存储到全
      局内存，而第二个线程通过第二个L1缓存加载该地址，并使用ld.ca，则第二个可能会得到过时的L1缓存
      数据，而不是第一个线程存储的数据。驱动程序必须使并行线程的从属网格之间的全局L1缓存线无效。然
      后，第一个网格程序的存储由第二个网格程序正确获取，该程序发出L1中缓存的默认ld.ca加载。
 .cg  Cache at global level，全局级缓存（L2及以下缓存，而不是L1）。
      使用ld.cg全局性地加载，绕过L1缓存，并仅缓存在L2缓存中。
 .cs  Cache streaming，缓存流，可能会被访问一次。
      ld.cs加载缓存的流操作在L1和L2分配全局行，采用驱逐优先的策略在L1和L2分配全局行，以限制临时流
      数据对缓存污染，这些数据可能被访问一次或两次。当ld.cs被应用到一个本地窗口地址时，它执行ld.lu
      操作。
 .lu  Last use。
      编译器/程序员在恢复溢出的寄存器和弹出函数堆栈框架时可以使用ld.lu，以避免不必要的写回不会再使
      用的行。ld.lu指令在全局地址上执行一个加载缓存的流操作（ld.cs）。
 .cv  不要再次缓存和获取（考虑缓存的系统内存线已过时，再次获取）。应用于全局系统内存地址的ld.cv加载
      操作使匹配的L2行无效（丢弃），并在每次新加载时重新获取该行。
2. 内存Store指令的缓存运算符：
 .wb  Cache write-back all coherent levels，缓存写回所有级别一致。
      默认的存储指令缓存操作是st.wb，它使用正常的逐出策略写回一致缓存级别的缓存行。如果一个线程绕过
      其L1缓存存储到全局内存，而另一个SM中的第二个线程稍后通过具有ld.ca的不同L1缓存从该地址加载，则
      第二个可能会命中过时的L1缓存数据，而不是从第一个线程存储的L2或内存中获取数据。驱动程序必须使线
      程阵列的从属网格之间的全局L1缓存线无效。然后，第一个网格程序的存储在L1中正确丢失，并由发出默认
      ld.ca加载的第二个网格程序获取。
 .wt  Cache write-through (to system memory).
      st.wt存储写入操作应用于通过二级缓存写入的全局系统内存地址。
*/
enum cache_operator_type {
  CACHE_UNDEFINED,

  // loads
  CACHE_ALL,       // .ca
  CACHE_LAST_USE,  // .lu
  CACHE_VOLATILE,  // .cv
  CACHE_L1,        // .nc

  // loads and stores
  CACHE_STREAMING,  // .cs
  CACHE_GLOBAL,     // .cg

  // stores
  CACHE_WRITE_BACK,    // .wb
  CACHE_WRITE_THROUGH  // .wt
};


class memory_space_t {
 public:
  //构造函数。初始时，设置存储空间类型为 未定义的空间类型，设置 Bank 数为0。
  memory_space_t() {
    m_type = undefined_space;
    m_bank = 0;
  }
  //构造函数。设置存储空间类型为 传入参数的类型，设置 Bank 数为0。
  memory_space_t(const enum _memory_space_t &from) {
    m_type = from;
    m_bank = 0;
  }
  
  bool operator==(const memory_space_t &x) const {
    return (m_bank == x.m_bank) && (m_type == x.m_type);
  }
  bool operator!=(const memory_space_t &x) const { return !(*this == x); }
  bool operator<(const memory_space_t &x) const {
    if (m_type < x.m_type)
      return true;
    else if (m_type > x.m_type)
      return false;
    else if (m_bank < x.m_bank)
      return true;
    return false;
  }
  enum _memory_space_t get_type() const { return m_type; }
  void set_type(enum _memory_space_t t) { m_type = t; }
  unsigned get_bank() const { return m_bank; }
  void set_bank(unsigned b) { m_bank = b; }
  bool is_const() const {
    return (m_type == const_space) || (m_type == param_space_kernel);
  }
  bool is_local() const {
    return (m_type == local_space) || (m_type == param_space_local);
  }
  bool is_global() const { return (m_type == global_space); }

 private:
  enum _memory_space_t m_type;
  unsigned m_bank;  // n in ".const[n]"; note .const == .const[0] (see PTX 2.1
                    // manual, sec. 5.1.3)
};

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
 public:
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
  /*********************************************************************/ // TODO
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

  unsigned char is_vectorin;
  unsigned char is_vectorout;

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
  unsigned m_warp_id;
  unsigned m_dynamic_warp_id;
  // const core_config *m_config;
  active_mask_t m_warp_active_mask;  // dynamic active mask for timing model
                                     // (after predication)
  memory_space_t space;
  cache_operator_type cache_op;
  /*********************************************************************/
  
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

  std::map<std::pair<int, int>, _inst_trace_t*>* get_instn_info_vector() { return &instn_info_vector; }

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

  bool valid = false;
  unsigned kernel_id, pc;
  unsigned mask;

  unsigned cta_id_x;
  unsigned cta_id_y;
  unsigned cta_id_z;

  unsigned warp_id;
  unsigned sm_id;

  unsigned gwarp_id;

  _inst_trace_t* inst_trace;

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
