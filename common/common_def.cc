#include "common_def.h"

int kernel_info_m_next_uid = 0;

#ifdef USE_BOOST
void simple_mpi_test(int argc, char **argv) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;
  if (world.rank() == 0) {
    /* Send data in rank 0. */
    int data = 123;
    for (int i = 1; i < world.size(); i++) {
      world.send(i, i, data);
      std::cout << "rank " << 0 << " send data: " << data << " to rank: " << i << std::endl;
    }
  } else {
    /* Recieve data in rank > 0. */
    int recv_data;
    world.recv(0, world.rank(), recv_data);
    std::cout << "rank " << world.rank() << " recv data: " << recv_data << " from rank: " << 0 << std::endl;
  }
}
#endif


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
unsigned long long GLOBAL_HEAP_START = 0xC0000000;
// Volta max shmem size is 96kB
unsigned long long SHARED_MEM_SIZE_MAX = 96 * (1 << 10);
// Volta max local mem is 16kB
unsigned long long LOCAL_MEM_SIZE_MAX = 1 << 14;
// Volta Titan V and V100 has 80 SMs
unsigned MAX_STREAMING_MULTIPROCESSORS = 80;
// Max 2048 threads / SM
unsigned MAX_THREAD_PER_SM = 1 << 11;
// MAX 64 warps / SM
unsigned MAX_WARP_PER_SM = 1 << 6;
unsigned long long TOTAL_LOCAL_MEM_PER_SM = MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX;
unsigned long long TOTAL_SHARED_MEM = MAX_STREAMING_MULTIPROCESSORS * SHARED_MEM_SIZE_MAX;
unsigned long long TOTAL_LOCAL_MEM = MAX_STREAMING_MULTIPROCESSORS * MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX;
unsigned long long SHARED_GENERIC_START = GLOBAL_HEAP_START - TOTAL_SHARED_MEM;
unsigned long long LOCAL_GENERIC_START = SHARED_GENERIC_START - TOTAL_LOCAL_MEM;
unsigned long long STATIC_ALLOC_LIMIT = GLOBAL_HEAP_START - (TOTAL_LOCAL_MEM + TOTAL_SHARED_MEM);
