#include <iostream>
#include <fstream>
#include <bitset>


#ifndef COMMON_DEF_H
#define COMMON_DEF_H

/* User define Start. */
#define USE_BOOST
#define gpgpu_concurrent_kernel_sm false
/* User define End. */

#define WARP_SIZE 32
#define MAX_DST 1
#define MAX_SRC 4

#define MAX_WARP_PER_SHADER 64

// enum uarch_operand_type_t { UN_OP = -1, INT_OP, FP_OP };
// typedef enum uarch_operand_type_t types_of_operands;

// After expanding the vector input and output operands
#define MAX_INPUT_VALUES 24
#define MAX_OUTPUT_VALUES 8

// the maximum number of destination, source, or address uarch operands in a
// instruction
#define MAX_REG_OPERANDS 32

/***********************************************************************/

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

/***********************************************************************/

// the maximum number of destination, source, or address uarch operands in a
// instruction
#define MAX_REG_OPERANDS 32

#define MAX_KERNELS_NUM 300


#ifdef USE_BOOST
  /* See https://www.boost.org/doc/libs/1_68_0/doc/html/mpi/tutorial.html for tutorial of boost::mpi,
   * to use the boost::mpi to implement the multi-process running. */
  #include <boost/mpi.hpp>
  #include <boost/serialization/vector.hpp>
  #include <boost/serialization/map.hpp>
#endif


#ifdef USE_BOOST
void simple_mpi_test(int argc, char **argv);
#endif

extern int kernel_info_m_next_uid;

#define OPEN_MEMORY_TRACE_FILE() \
  std::ofstream outfile; \
  outfile.open("memory_trace.txt");

#define CLOSE_MEMORY_TRACE_FILE() \
  outfile.close();

#define PRINT_2_MEMORY_TRACE_FILE(s) \
  outfile << std::hex << s << std::endl;

#define PRINT_2_MEMORY_TRACE_FILE_0x() \
  outfile << "0x";

#define START_TIMER(no) auto start##no = std::chrono::system_clock::now();

#define STOP_AND_REPORT_TIMER_pass(pass, no) \
    if (pass != -1) std::cout << "    pass-" << pass << " "; \
    auto end##no = std::chrono::system_clock::now(); \
    auto duration##no = std::chrono::duration_cast<std::chrono::microseconds>(end##no - start##no); \
    auto cost##no = double(duration##no.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den; \
    std::cout << "Cost " << no << "-" << cost##no << " seconds." << std::endl;


#define STOP_AND_REPORT_TIMER_rank(rank, no) \
    std::cout << "    L1 rank-" << rank << " "; \
    auto end##no = std::chrono::system_clock::now(); \
    auto duration##no = std::chrono::duration_cast<std::chrono::microseconds>(end##no - start##no); \
    auto cost##no = double(duration##no.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den; \
    std::cout << "Cost " << no << "-" << cost##no << " seconds." << std::endl;

/* If the memory instruction does not specify a state space, the operation is performed using generic addressing. 
 * Spaces states: .const, .const, .param, .local and .shared. We models as a window within a generic address space. 
 * Each window is defined by its window-base and window-size equal to the size of that space state. With this part-
 * ition of address space, each thread can only have a maximum of 16 kB of local memory (LOCAL_MEM_SIZE_MAX). In 
 * CUDA Compute Power 1.3 and later, each thread can have up to 16 kB of local memory. In the case of CUDA Comput-
 * ing Power 2.0, this limit is increased to 512 kB. Users can add LOCAL_MEM_SIZE_MAX to support applications that 
 * require more than 16 kB of local memory per thread. However, it should always be ensured that: 
 *     GLOBAL_HEAP_START > (TOTAL_LOCAL_MEM + TOTAL_SHARED_MEM).
 * 
 * The starting address of the GLOBAL_HEAP in Volta GPU is: 0xC0000000.
 *     GLOBAL_HEAP_START              = 0xC0000000
 * 
 * The maximum SHARED_MEM space of a single SM is 96 *x 1024
 *     SHARED_MEM_SIZE_MAX            = 96 * (1 << 10)
 * 
 * A single thread can only have up to 16 kB of local memory.
 *     LOCAL_MEM_SIZE_MAX             = 1 << 14
 * 
 * The maximum number of SMs of Volta is up to 80.
 *     MAX_STREAMING_MULTIPROCESSORS  = 80
 * 
 * The maximum number of threads per SM.
 *     MAX_THREAD_PER_SM              = 1 << 11
 * 
 * The maximum number of warps per SM.
 *     MAX_WARP_PER_SM                = 1 << 6
 * 
 * TOTAL_LOCAL_MEM_PER_SM         = MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX = 0x2000000
 * TOTAL_SHARED_MEM               = MAX_STREAMING_MULTIPROCESSORS * SHARED_MEM_SIZE_MAX = 0x780000
 * TOTAL_LOCAL_MEM                = MAX_STREAMING_MULTIPROCESSORS * MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX = 0xA0000000
 * 
 * In the memory space, it is divided into multiple parts, the first is global memory - part1, then is local memory
 * - part2, and then is shared memory - part3, then is global memory - part4:
 *     the range of global memory - part1 is: addr < STATIC_ALLOC_LIMIT (addr < 0x1F880000)
 *     the range of global memory - part4 is: addr >= GLOBAL_HEAP_START (addr >= 0xC0000000)
 *     the range of local memory - part2 is: STATIC_ALLOC_LIMIT <= addr < SHARED_GENERIC_START (0x1F880000 <= addr < 0xBF880000)
 *     the range of shared memory - part3 is: SHARED_GENERIC_START <= addr < GLOBAL_HEAP_START (0xBF880000 <= addr < 0xC0000000)
 * 
 * where,
 *     SHARED_GENERIC_START           = GLOBAL_HEAP_START-TOTAL_SHARED_MEM = 0xBF880000
 *     LOCAL_GENERIC_START            = SHARED_GENERIC_START-TOTAL_LOCAL_MEM = 0x1F880000
 *     STATIC_ALLOC_LIMIT             = GLOBAL_HEAP_START - (TOTAL_LOCAL_MEM+TOTAL_SHARED_MEM) = 0x1F880000
*/

// start allocating from this address (lower values used for allocating globals
// in .ptx file)
extern unsigned long long GLOBAL_HEAP_START;
// Volta max shmem size is 96kB
extern unsigned long long SHARED_MEM_SIZE_MAX;
// Volta max local mem is 16kB
extern unsigned long long LOCAL_MEM_SIZE_MAX;
// Volta Titan V and V100 has 80 SMs
extern unsigned MAX_STREAMING_MULTIPROCESSORS;
// Max 2048 threads / SM
extern unsigned MAX_THREAD_PER_SM;
// MAX 64 warps / SM
extern unsigned MAX_WARP_PER_SM;
extern unsigned long long TOTAL_LOCAL_MEM_PER_SM;
extern unsigned long long TOTAL_SHARED_MEM;
extern unsigned long long TOTAL_LOCAL_MEM;
extern unsigned long long SHARED_GENERIC_START;
extern unsigned long long LOCAL_GENERIC_START;
extern unsigned long long STATIC_ALLOC_LIMIT;


#endif