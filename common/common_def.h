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
