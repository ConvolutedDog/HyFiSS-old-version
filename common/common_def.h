#include <iostream>
#include <fstream>


/* User define Start. */
#define USE_BOOST
#define gpgpu_concurrent_kernel_sm false
/* User define End. */


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
