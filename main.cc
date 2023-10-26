// developed by Mahmoud Khairy, Purdue Univ
// abdallm@purdue.edu

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <vector>
#include <cxxabi.h>
#include <iomanip>

#include "./ISA-Def/trace_opcode.h"
#include "./trace-parser/trace-parser.h"
#include "./trace-driven/trace-driven.h"

#define gpgpu_concurrent_kernel_sm true

trace_kernel_info_t *create_kernel_info(kernel_trace_t* kernel_trace_info,
							                          trace_parser *parser){
  dim3 gridDim(kernel_trace_info->grid_dim_x, kernel_trace_info->grid_dim_y, kernel_trace_info->grid_dim_z);
  dim3 blockDim(kernel_trace_info->tb_dim_x, kernel_trace_info->tb_dim_y, kernel_trace_info->tb_dim_z);
  trace_kernel_info_t *kernel_info =
      new trace_kernel_info_t(gridDim, blockDim, parser, kernel_trace_info);

  return kernel_info;
}

int main(int argc, const char **argv) {
  std::cout << "Memory Model." << std::endl;
  
  if (argc < 2) printf("Error: plearse specify the kernel list argument.\n");
  else if (argc == 2) {
    // if (typeid(argv[0]) == typeid(std::string)) ;
    // else printf("Error: the kernel list argument must be string.\n");
  } else if (argc > 2) printf("Error: too many arguments.\n");

  trace_parser tracer(argv[1]);

  // for each kernel:
  //     load file
  //     parse and create kernel info
  //     check insns
  std::pair<std::vector<trace_command>, int> result = tracer.parse_commandlist_file();
  std::vector<trace_command> commandlist = result.first;
  int concurrent_kernel_kernel_nums = result.second;

  std::vector<unsigned long> busy_streams;
  std::vector<trace_kernel_info_t*> kernels_info;
  kernels_info.reserve(gpgpu_concurrent_kernel_sm ? concurrent_kernel_kernel_nums : 1);

  unsigned i = 0;
  //gulp up as many commands as possible - either cpu_gpu_mem_copy 
  //or kernel_launch - until the vector "kernels_info" has reached
  //the concurrent_kernel_kernel_nums or we have read every command 
  //from commandlist
  while (kernels_info.size() < concurrent_kernel_kernel_nums && i < commandlist.size()) {
    trace_kernel_info_t *kernel_info = NULL;
    if (commandlist[i].m_type == command_type::cpu_gpu_mem_copy) {
      size_t addre, Bcount;
      tracer.parse_memcpy_info(commandlist[i].command_string, addre, Bcount);
      std::cout << "Launching memcpy command : " << commandlist[i].command_string << std::endl;
      i++;
    } else if (commandlist[i].m_type == command_type::kernel_launch) {
      // Read trace header info for concurrent_kernel_kernel_nums number of kernels
      kernel_trace_t* kernel_trace_info = tracer.parse_kernel_info(commandlist[i].command_string);
      kernel_info = create_kernel_info(kernel_trace_info, &tracer);
      kernels_info.push_back(kernel_info);
      std::cout << "Header info loaded for kernel command : " << commandlist[i].command_string << std::endl;
      i++;
    }
    else{
      //unsupported commands will fail the simulation
      assert(0 && "Undefined Command.");
    }
  }

  std::cout << "Kernel nums waiting for processing : " << kernels_info.size() << std::endl;

  for (auto k : kernels_info) {
    std::vector<std::vector<inst_trace_t> *> threadblock_traces;
    unsigned start_warp = 0;
    unsigned end_warp = k->get_trace_info()->tb_dim_x * 
                        k->get_trace_info()->tb_dim_y * 
                        k->get_trace_info()->tb_dim_z / MAX_WARP_SIZE - 1;

    unsigned total_warps_per_thread_block = end_warp - start_warp + 1; // per block

    for (int i = 0; i < k->get_trace_info()->grid_dim_x * 
                        k->get_trace_info()->grid_dim_y * 
                        k->get_trace_info()->grid_dim_z; i++) {
      threadblock_traces = k->get_next_threadblock_traces(k->get_trace_info()->kernel_name, 
                                                          k->get_trace_info()->kernel_id,
                                                          total_warps_per_thread_block);
      for (int i = 0; i < threadblock_traces.size(); i++) {
        std::vector<inst_trace_t> *traces = threadblock_traces[i];
        
        std::cout << "warp = " << std::dec << i << std::endl;
        for (const auto &value : *traces) {
          std::cout << " pc[" 
                    << std::setw(4) << std::right << std::hex << value.m_pc 
                    << "] | mask[" 
                    << std::setw(8) << std::right << std::hex << value.mask 
                    << "] | dstnum[" 
                    << std::setw(2) << std::right << value.reg_dsts_num 
                    << "] | srcnum[" 
                    << std::setw(2) << std::right << value.reg_srcs_num 
                    << "] | opcode[" 
                    << std::setw(21) << std::right << value.opcode << "]" 
                    << std::endl;
        }
      }
    }

    for (int i = end_warp; i < end_warp; i++) {
      delete threadblock_traces[i];
    }
    threadblock_traces.clear();
  }

  fflush(stdout);

  return 0;
}

