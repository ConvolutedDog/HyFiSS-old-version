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

// #include "gpgpu_context.h"
// #include "abstract_hardware_model.h"
// #include "cuda-sim/cuda-sim.h"
// #include "gpgpu-sim/gpu-sim.h"
// #include "gpgpu-sim/icnt_wrapper.h"
// #include "gpgpusim_entrypoint.h"
// #include "option_parser.h"
#include "./ISA-Def/trace_opcode.h"
// #include "trace_driven.h"
#include "./trace-parser/trace_parser.h"
// #include "accelsim_version.h"

#define gpgpu_concurrent_kernel_sm true

// gpgpu_sim *gpgpu_trace_sim_init_perf_model(int argc, const char *argv[],
//                                            gpgpu_context *m_gpgpu_context,
//                                            class trace_config *m_config);

// trace_kernel_info_t *create_kernel_info( kernel_trace_t* kernel_trace_info,
// 		                      gpgpu_context *m_gpgpu_context, class trace_config *config,
// 							  trace_parser *parser);


int main(int argc, const char **argv) {
  std::cout << "Memory Model." << std::endl;
//   gpgpu_context *m_gpgpu_context = new gpgpu_context();
//   trace_config tconfig;

//   gpgpu_sim *m_gpgpu_sim =
//       gpgpu_trace_sim_init_perf_model(argc, argv, m_gpgpu_context, &tconfig);
//   m_gpgpu_sim->init();
  
  if (argc != 1) printf("Error: plearse specify the kernel list argument.\n");
  else if (argc == 1) {
    if (typeid(argv[0]) == typeid(std::string)) trace_parser tracer(argv[0]);
    else printf("Error: the kernel list argument must be string.\n");
  } else printf("Error: too many arguments.\n");

  // tconfig.parse_config();

  // for each kernel:
  //     load file
  //     parse and create kernel info
  //     launch
  //     while loop till the end of the end kernel execution
  //     prints stats
  // bool concurrent_kernel_sm =  m_gpgpu_sim->getShaderCoreConfig()->gpgpu_concurrent_kernel_sm;
  // unsigned window_size = concurrent_kernel_sm ? m_gpgpu_sim->get_config().get_max_concurrent_kernel() : 1;
  // assert(window_size > 0);
  std::pair<std::vector<trace_command>, int> result = tracer.parse_commandlist_file();
  std::vector<trace_command> commandlist = result.first;
  int concurrent_kernel_kernel_nums = result.second;

  // std::vector<unsigned long> busy_streams;
  std::vector<trace_kernel_info_t*> kernels_info;
  kernels_info.reserve(gpgpu_concurrent_kernel_sm ? concurrent_kernel_kernel_nums : 1);

  unsigned i = 0;
  while (i < commandlist.size() || !kernels_info.empty()) {
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
        // m_gpgpu_sim->perf_memcpy_to_gpu(addre, Bcount);
        i++;
      } else if (commandlist[i].m_type == command_type::kernel_launch) {
        // Read trace header info for concurrent_kernel_kernel_nums number of kernels
        kernel_trace_t* kernel_trace_info = tracer.parse_kernel_info(commandlist[i].command_string);
        kernel_info = create_kernel_info(kernel_trace_info, m_gpgpu_context, &tconfig, &tracer);
        kernels_info.push_back(kernel_info);
        std::cout << "Header info loaded for kernel command : " << commandlist[i].command_string << std::endl;
        i++;
      }
      else{
        //unsupported commands will fail the simulation
        assert(0 && "Undefined Command.");
      }
    }
  }

  fflush(stdout);

  return 0;
}


// trace_kernel_info_t *create_kernel_info( kernel_trace_t* kernel_trace_info,
// 		                      gpgpu_context *m_gpgpu_context, class trace_config *config,
// 							  trace_parser *parser){

//   gpgpu_ptx_sim_info info;
//   info.smem = kernel_trace_info->shmem;
//   info.regs = kernel_trace_info->nregs;
//   dim3 gridDim(kernel_trace_info->grid_dim_x, kernel_trace_info->grid_dim_y, kernel_trace_info->grid_dim_z);
//   dim3 blockDim(kernel_trace_info->tb_dim_x, kernel_trace_info->tb_dim_y, kernel_trace_info->tb_dim_z);
//   trace_function_info *function_info =
//       new trace_function_info(info, m_gpgpu_context);
//   function_info->set_name(kernel_trace_info->kernel_name.c_str());
//   trace_kernel_info_t *kernel_info =
//       new trace_kernel_info_t(gridDim, blockDim, function_info,
//     		  parser, config, kernel_trace_info);

//   return kernel_info;
// }

// gpgpu_sim *gpgpu_trace_sim_init_perf_model(int argc, const char *argv[],
//                                            gpgpu_context *m_gpgpu_context,
//                                            trace_config *m_config) {
//   srand(1);
//   print_splash();

//   option_parser_t opp = option_parser_create();

//   m_gpgpu_context->ptx_reg_options(opp);
//   m_gpgpu_context->func_sim->ptx_opcocde_latency_options(opp);

//   //读取互连网络的配置信息。
//   icnt_reg_options(opp);

//   m_gpgpu_context->the_gpgpusim->g_the_gpu_config =
//       new gpgpu_sim_config(m_gpgpu_context);
//   m_gpgpu_context->the_gpgpusim->g_the_gpu_config->reg_options(
//       opp); // register GPU microrachitecture options
//   m_config->reg_options(opp);

//   option_parser_cmdline(opp, argc, argv); // parse configuration options
//   fprintf(stdout, "GPGPU-Sim: Configuration options:\n\n");
//   option_parser_print(opp, stdout);
//   // Set the Numeric locale to a standard locale where a decimal point is a
//   // "dot" not a "comma" so it does the parsing correctly independent of the
//   // system environment variables
//   assert(setlocale(LC_NUMERIC, "C"));
//   m_gpgpu_context->the_gpgpusim->g_the_gpu_config->init();

//   m_gpgpu_context->the_gpgpusim->g_the_gpu = new trace_gpgpu_sim(
//       *(m_gpgpu_context->the_gpgpusim->g_the_gpu_config), m_gpgpu_context);

//   m_gpgpu_context->the_gpgpusim->g_stream_manager =
//       new stream_manager((m_gpgpu_context->the_gpgpusim->g_the_gpu),
//                          m_gpgpu_context->func_sim->g_cuda_launch_blocking);

//   m_gpgpu_context->the_gpgpusim->g_simulation_starttime = time((time_t *)NULL);

//   return m_gpgpu_context->the_gpgpusim->g_the_gpu;
// }
