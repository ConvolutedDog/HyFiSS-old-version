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
    std::cout << "Cost " << no << "-" << cost##no << " ms." << std::endl;


#define STOP_AND_REPORT_TIMER_rank(rank, no) \
    std::cout << "    L1 rank-" << rank << " "; \
    auto end##no = std::chrono::system_clock::now(); \
    auto duration##no = std::chrono::duration_cast<std::chrono::microseconds>(end##no - start##no); \
    auto cost##no = double(duration##no.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den; \
    std::cout << "Cost " << no << "-" << cost##no << " seconds." << std::endl;
