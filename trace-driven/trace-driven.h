// Copyright (c) 2018-2021, Mahmoud Khairy, Vijay Kandiah, Timothy Rogers, Tor M. Aamodt, Nikos Hardavellas
// Northwestern University, Purdue University, The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer;
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution;
// 3. Neither the names of Northwestern University, Purdue University,
//    The University of British Columbia nor the names of their contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include <list>
#include <string.h>
#include <map>
#include <string>

#ifndef TRACE_DRIVEN_H
#define TRACE_DRIVEN_H

#include "../ISA-Def/trace_opcode.h"
#include "../trace-parser/trace-parser.h"
#include "../common/vector_types.h"


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


// Let's just upgrade to C++11 so we can use constexpr here...
// start allocating from this address (lower values used for allocating globals
// in .ptx file)
const unsigned long long GLOBAL_HEAP_START = 0xC0000000;
// Volta max shmem size is 96kB
const unsigned long long SHARED_MEM_SIZE_MAX = 96 * (1 << 10);
// Volta max local mem is 16kB
const unsigned long long LOCAL_MEM_SIZE_MAX = 1 << 14;
// Volta Titan V has 80 SMs
const unsigned MAX_STREAMING_MULTIPROCESSORS = 80;
// Max 2048 threads / SM
const unsigned MAX_THREAD_PER_SM = 1 << 11;
// MAX 64 warps / SM
const unsigned MAX_WARP_PER_SM = 1 << 6;
const unsigned long long TOTAL_LOCAL_MEM_PER_SM =
    MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX;
const unsigned long long TOTAL_SHARED_MEM =
    MAX_STREAMING_MULTIPROCESSORS * SHARED_MEM_SIZE_MAX;
const unsigned long long TOTAL_LOCAL_MEM =
    MAX_STREAMING_MULTIPROCESSORS * MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX;
const unsigned long long SHARED_GENERIC_START =
    GLOBAL_HEAP_START - TOTAL_SHARED_MEM;
const unsigned long long LOCAL_GENERIC_START =
    SHARED_GENERIC_START - TOTAL_LOCAL_MEM;
const unsigned long long STATIC_ALLOC_LIMIT =
    GLOBAL_HEAP_START - (TOTAL_LOCAL_MEM + TOTAL_SHARED_MEM);

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

class mem_access_t {
 public:
  //构造函数。mem_access_t类有一个私有的 gpgpu_context *gpgpu_ctx 对象，初始化该对象。
  mem_access_t() {}
  //构造函数。
  //new_addr_type定义：typedef unsigned long long new_addr_type;
  mem_access_t(mem_access_type type, new_addr_type address, 
               unsigned size, bool wr) {
    //mem_access_type定义了在时序模拟器中对不同类型的存储器进行不同的访存类型：
    //    MA_TUP(GLOBAL_ACC_R), 从global memory读
    //    MA_TUP(LOCAL_ACC_R), 从local memory读
    //    MA_TUP(CONST_ACC_R), 从常量缓存读
    //    MA_TUP(TEXTURE_ACC_R), 从纹理缓存读
    //    MA_TUP(GLOBAL_ACC_W), 向global memory写
    //    MA_TUP(LOCAL_ACC_W), 向local memory写
    //在V100中，L1 cache的m_write_policy为WRITE_THROUGH，实际上L1_WRBK_ACC也不会用到：
    //    MA_TUP(L1_WRBK_ACC), L1缓存write back
    //在V100中，当L2 cache写不命中时，采取lazy_fetch_on_read策略，当找到一个cache block
    //逐出时，如果这个cache block是被MODIFIED，则需要将这个cache block写回到下一级存储，
    //因此会产生L2_WRBK_ACC访问，这个访问就是为了写回被逐出的MODIFIED cache block。
    //    MA_TUP(L2_WRBK_ACC), L2缓存write back
    //    MA_TUP(INST_ACC_R), 从指令缓存（I-Cache）读
    //L1_WR_ALLOC_R/L2_WR_ALLOC_R在V100配置中暂时用不到：
    //    MA_TUP(L1_WR_ALLOC_R), L1缓存write-allocate（对cache写不命中，将主存中块调入cache，写入
    //                           该cache块）
    //L1_WR_ALLOC_R/L2_WR_ALLOC_R在V100配置中暂时用不到：
    //    MA_TUP(L2_WR_ALLOC_R), L2缓存write-allocate
    //    MA_TUP(NUM_MEM_ACCESS_TYPE), 存储器访问的类型总数
    m_type = type;
    //访存的地址。
    m_addr = address;
    //访存的数据大小，以字节为单位。
    m_req_size = size;
    //该访存是写/读，1-写，0-读。
    m_write = wr;
  }
  //构造函数。
  //active_mask_t 活跃掩码定义：
  //    typedef std::bitset<MAX_WARP_SIZE> active_mask_t; 
  //用于在处理一个warp内的线程分支，标记每个线程是否执行某个分支。
  //mem_access_byte_mask_t 访存数据字节掩码定义：
  //    typedef std::bitset<MAX_MEMORY_ACCESS_SIZE> mem_access_byte_mask_t;
  //用于标记一次访存操作中的数据字节掩码，MAX_MEMORY_ACCESS_SIZE设置为128，即每次访存最大数据128字节。
  //mem_access_sector_mask_t 扇区掩码定义：
  //    typedef std::bitset<SECTOR_CHUNCK_SIZE> mem_access_sector_mask_t;
  //用于标记一次访存操作中的扇区掩码，4个扇区，每个扇区32个字节数据。
  mem_access_t(mem_access_type type, new_addr_type address, unsigned size,
               bool wr, const active_mask_t &active_mask,
               const mem_access_byte_mask_t &byte_mask,
               const mem_access_sector_mask_t &sector_mask)
      : m_warp_mask(active_mask),
        m_byte_mask(byte_mask),
        m_sector_mask(sector_mask) {
    m_type = type;
    m_addr = address;
    m_req_size = size;
    m_write = wr;
  }
  //返回访存地址。
  new_addr_type get_addr() const { return m_addr; }
  //设置访存地址。
  void set_addr(new_addr_type addr) { m_addr = addr; }
  //返回访存数据大小，以字节为单位。
  unsigned get_size() const { return m_req_size; }
  //返回访存的线程活跃掩码。
  const active_mask_t &get_warp_mask() const { return m_warp_mask; }
  //返回该访存是写/读，1-写，0-读。
  bool is_write() const { return m_write; }
  //返回对存储器进行的访存类型，见构造函数注释。
  enum mem_access_type get_type() const { return m_type; }
  //返回访存的数据字节掩码。
  mem_access_byte_mask_t get_byte_mask() const { return m_byte_mask; }
  //返回访存的扇区掩码。
  mem_access_sector_mask_t get_sector_mask() const { return m_sector_mask; }

  //将访存的 地址、store或load、数据大小、访存类型打印到文件。
  void print(FILE *fp) const {
    fprintf(fp, "addr=0x%llx, %s, size=%u, ", m_addr,
            m_write ? "store" : "load ", m_req_size);
    switch (m_type) {
      case GLOBAL_ACC_R:
        fprintf(fp, "GLOBAL_R");
        break;
      case LOCAL_ACC_R:
        fprintf(fp, "LOCAL_R ");
        break;
      case CONST_ACC_R:
        fprintf(fp, "CONST   ");
        break;
      case TEXTURE_ACC_R:
        fprintf(fp, "TEXTURE ");
        break;
      case GLOBAL_ACC_W:
        fprintf(fp, "GLOBAL_W");
        break;
      case LOCAL_ACC_W:
        fprintf(fp, "LOCAL_W ");
        break;
      case L2_WRBK_ACC:
        fprintf(fp, "L2_WRBK ");
        break;
      case INST_ACC_R:
        fprintf(fp, "INST    ");
        break;
      case L1_WRBK_ACC:
        fprintf(fp, "L1_WRBK ");
        break;
      default:
        fprintf(fp, "unknown ");
        break;
    }
  }

 private:
  //该次访存操作的唯一ID。
  unsigned m_uid;
  //访存地址。
  new_addr_type m_addr;  // request address
  //该访存是写/读，1-写，0-读。
  bool m_write;
  //访存数据大小，以字节为单位。
  unsigned m_req_size;  // bytes
  //对不同类型的存储器进行的访存类型，见构造函数注释。
  mem_access_type m_type;
  //访存的线程活跃掩码。
  active_mask_t m_warp_mask;
  //访存的数据字节掩码。
  mem_access_byte_mask_t m_byte_mask;
  //访存的扇区掩码。
  mem_access_sector_mask_t m_sector_mask;
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


class trace_warp_inst_t {
 public:
  trace_warp_inst_t() {
    m_opcode = 0;
    m_uid = 0;
    m_empty = true;
    m_isatomic = false;

    m_decoded = false;
    pc = (address_type)-1;
    isize = 0;

    num_operands = 0;
    num_regs = 0;

    memset(out, 0, sizeof(unsigned));
    outcount = 0;
    memset(in, 0, sizeof(unsigned));
    incount = 0;

    is_vectorin = 0;
    is_vectorout = 0;

    for (unsigned i = 0; i < MAX_REG_OPERANDS; i++) {
      arch_reg.src[i] = -1;
      arch_reg.dst[i] = -1;
    }

    op = NO_OP;
    sp_op = OTHER_OP;
    mem_op = NOT_TEX;

    const_cache_operand = 0;

    oprnd_type = UN_OP;

    m_per_scalar_thread_valid = false;
    m_mem_accesses_created = false;
    m_is_printf = false;
    should_do_atomic = false;

    m_mem_accesses_created = false;

    space = memory_space_t();
    cache_op = CACHE_UNDEFINED;
  }

  // trace_warp_inst_t(const class core_config *config) : warp_inst_t(config) {
  //   m_opcode = 0;
  //   should_do_atomic = false;
  // }

  bool parse_from_trace_struct(
      const inst_trace_t &trace,
      const std::unordered_map<std::string, OpcodeChar> *OpcodeMap,
    //   const class trace_config *tconfig,
      const class kernel_trace_t *kernel_trace_info);
  
  void set_active(const active_mask_t &active);

  void set_addr(unsigned n, new_addr_type addr) {
    if (!m_per_scalar_thread_valid) {
      m_per_scalar_thread.resize(MAX_WARP_SIZE);
      m_per_scalar_thread_valid = true;
    }
    m_per_scalar_thread[n].memreqaddr[0] = addr;
  }

  void set_addr(unsigned n, new_addr_type *addr, unsigned num_addrs) {
    if (!m_per_scalar_thread_valid) {
      m_per_scalar_thread.resize(MAX_WARP_SIZE);
      m_per_scalar_thread_valid = true;
    }
    assert(num_addrs <= MAX_ACCESSES_PER_INSN_PER_THREAD);
    for (unsigned i = 0; i < num_addrs; i++)
      m_per_scalar_thread[n].memreqaddr[i] = addr[i];
  }

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
  
  struct per_thread_info {
    per_thread_info() {
      for (unsigned i = 0; i < MAX_ACCESSES_PER_INSN_PER_THREAD; i++)
        memreqaddr[i] = 0;
    }
    //MAX_ACCESSES_PER_INSN_PER_THREAD为单个线程中允许的最大访存次数。设置为8。
    //memreqaddr[]存储了单条指令的所有访存地址。
    new_addr_type
        memreqaddr[MAX_ACCESSES_PER_INSN_PER_THREAD];  // effective address,
                                                       // upto 8 different
                                                       // requests (to support
                                                       // 32B access in 8 chunks
                                                       // of 4B each)
  };
  bool m_per_scalar_thread_valid;
  //m_per_scalar_thread是线程信息的向量，每个warp有一个m_per_scalar_thread。
  std::vector<per_thread_info> m_per_scalar_thread;
  bool m_mem_accesses_created;
  //当前指令的访存操作的列表。
  std::list<mem_access_t> m_accessq;

  memory_space_t space;
  cache_operator_type cache_op;
};


/*
内核函数的信息。kernel_info_t 对象包含GPU网格和块维度、与内核入口点关联的 function_info 对象以
及为内核参数分配的内存。
*/
class kernel_info_t {
 public:
  kernel_info_t(dim3 gridDim, dim3 blockDim);
  ~kernel_info_t(){};

  //返回CUDA代码中的Grid中的所有线程块的总数。
  size_t num_blocks() const {
    return m_grid_dim.x * m_grid_dim.y * m_grid_dim.z;
  }
  //返回每个线程块中的线程数量，threads_per_cta=m_block_dim.x * m_block_dim.y * m_block_dim.z
  size_t threads_per_cta() const {
    return m_block_dim.x * m_block_dim.y * m_block_dim.z;
  }
  //返回CUDA代码中的Grid的三个维度，一个dim3数据类型。
  dim3 get_grid_dim() const { return m_grid_dim; }
  //返回CTA的三个维度，一个dim3数据类型。
  dim3 get_cta_dim() const { return m_block_dim; }

  //返回当前 kernel_info_t 对象的唯一标识号。
  unsigned get_uid() const { return m_uid; }

  //kernel_info_t对象的唯一标识符。
  unsigned m_uid;
  //Grid和Block的维度。
  dim3 m_grid_dim;
  dim3 m_block_dim;
};

class trace_kernel_info_t : public kernel_info_t{
 public:
  trace_kernel_info_t(dim3 gridDim, dim3 blockDim,
                      trace_parser *parser, 
                    //   class trace_config *config,
                      kernel_trace_t *kernel_trace_info);

  std::vector<std::vector<inst_trace_t> *> get_next_threadblock_traces(std::string kernel_name,
                                                                       unsigned kernel_id,
                                                                       unsigned num_warps_per_thread_block);
  std::vector<mem_instn>& get_one_kernel_one_threadblock_traces(unsigned kernel_id, unsigned block_id);

  unsigned long get_cuda_stream_id() {
    return m_kernel_trace_info->cuda_stream_id;
  }

  kernel_trace_t *get_trace_info() { return m_kernel_trace_info; }

 private:
  const std::unordered_map<std::string, OpcodeChar> *OpcodeMap;
  trace_parser *m_parser;
  kernel_trace_t *m_kernel_trace_info;
};


types_of_operands get_oprnd_type(op_type op, special_ops sp_op);

#endif