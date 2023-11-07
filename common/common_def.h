#include <iostream>
#include <fstream>

extern int kernel_info_m_next_uid;

#define OPEN_MEMORY_TRACE_FILE() \
  std::ofstream outfile; \
  outfile.open("memor_trace.txt");

#define CLOSE_MEMORY_TRACE_FILE() \
  outfile.close();

#define PRINT_2_MEMORY_TRACE_FILE(s) \
  outfile << std::hex << s << std::endl;

#define PRINT_2_MEMORY_TRACE_FILE_0x() \
  outfile << "0x";