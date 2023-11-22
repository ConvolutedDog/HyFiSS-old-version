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
#include <algorithm>

#include "./ISA-Def/trace_opcode.h"
#include "./DEV-Def/compute_capability_config.h"
#include "./trace-parser/trace-parser.h"
#include "./trace-driven/trace-driven.h"
#include "./common/CLI/CLI.hpp"
#include "./common/common_def.h"


trace_kernel_info_t *create_kernel_info(kernel_trace_t* kernel_trace_info,
							                          trace_parser *parser){
  dim3 gridDim(kernel_trace_info->grid_dim_x, kernel_trace_info->grid_dim_y, kernel_trace_info->grid_dim_z);
  dim3 blockDim(kernel_trace_info->tb_dim_x, kernel_trace_info->tb_dim_y, kernel_trace_info->tb_dim_z);
  trace_kernel_info_t *kernel_info =
      new trace_kernel_info_t(gridDim, blockDim, parser, kernel_trace_info);

  return kernel_info;
}


bool compare(const mem_instn &a, const mem_instn &b) {
  return a.time_stamp <= b.time_stamp;
}

void print_SM_traces(std::vector<mem_instn>* traces) {
  for (auto mem_ins : *traces) {
    std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
    std::cout << std::hex << mem_ins.time_stamp << " ";
    std::cout << std::hex << mem_ins.addr[0] << std::endl;
  }
}


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


#ifdef USE_BOOST
void private_L1_cache_hit_rate_evaluate(int argc, char **argv, std::map<int, std::vector<mem_instn>>* SM_traces_ptr) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  const int SM_traces_ptr_size = (int)(*SM_traces_ptr).size();
  /* Every rank process one *(SM_traces_ptr)[i], it will have pass_num passes to complete, 
   * which also means that every rank should process at most pass_num traces. */
  const int pass_num = int((SM_traces_ptr_size + world.size() - 1)/world.size());

  for (int pass = 0; pass < pass_num; pass++) {
    /* During pass,  */
    int curr_process_idx = world.rank() + pass * world.size();
    if (curr_process_idx < SM_traces_ptr_size) {
      // TODO: implement the private cache hit rate evaluate.
      for (auto mem_ins : (*SM_traces_ptr)[world.rank()]) {
        std::cout << "rank-" << std::dec << world.rank() << ", " << "SM-" << curr_process_idx << " ";
        std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
        std::cout << std::hex << mem_ins.time_stamp << " ";
        std::cout << std::hex << mem_ins.addr[0] << std::endl;
      }
    }
  }
}
#endif

int main(int argc, char **argv) {
#ifdef USE_BOOST
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;
#endif
  
  CLI::App app{"Memory Model."};

  std::string configs;
  bool PRINT_LOG = true;
  bool sort = false;

  app.add_option("--configs", configs, "The configs path, which is generated from our NVBit tool, "
                                       "e.g., \"./traces/vectoradd/configs\"");
  app.add_option("--sort", sort, "Simulate the order in which instructions are issued based on their "
                                 "timestamps");
  app.add_option("--log", PRINT_LOG, "Print the traces processing log or not");
  CLI11_PARSE(app, argc, argv);

  int passnum_concurrent_issue_to_sm = -1;

  trace_parser tracer(configs.c_str());

  std::vector<trace_kernel_info_t*> single_pass_kernels_info;

#ifdef USE_BOOST
  if (world.rank() == 0) {
#endif

    std::cout << "Memory Model." << std::endl << std::endl;

    /* parse the config files */
    tracer.parse_configs_file(PRINT_LOG);

    /* read all memory traces */
    tracer.read_mem_instns(PRINT_LOG);

    /* create all_kernels_info and single_pass_kernels_info */
    std::vector<trace_kernel_info_t*> all_kernels_info;
    all_kernels_info.reserve(tracer.get_appcfg()->get_kernels_num());

    single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
                                                                           cc_config[SM70].max_concurrent_kernels_num) : 1);

    /* V100 schedules 128 kernels for concurrent execution at a time, thus requiring (all_kernels_num + 127)/128 schedules. */
    passnum_concurrent_issue_to_sm = int((tracer.get_appcfg()->get_kernels_num() + 
                                         cc_config[SM70].max_concurrent_kernels_num - 1) / 
                                         cc_config[SM70].max_concurrent_kernels_num);

    std::cout << std::endl;
#ifdef USE_BOOST
  } /* end rank = 0 */
#endif

  /* SM_traces_all_passes will store all the SM_traces of all the passes, and the SM_traces_all_passes[pass_num]
   * means: pass_num -> sm_id -> std::vector<mem_instn>. */
  std::vector<std::map<int, std::vector<mem_instn>>> SM_traces_all_passes;

#ifdef USE_BOOST
  if (world.rank() == 0) {
#endif

    SM_traces_all_passes.resize(passnum_concurrent_issue_to_sm);

    for (int pass = 0; pass < passnum_concurrent_issue_to_sm; pass++) {
      if (PRINT_LOG) std::cout << "Schedule pass: " << pass << std::endl;
      
      /* for the pass-th scheduling process, V100 will schedule min(128,tracer.kernels_num) kernels to SMs,
       * here we will find the kernels that will be scheduled in this pass and create their kernel_info_t objects,
       * and then to evaluate every L1D cache in SMs, and also will interleave their missed address to L2D cache,
       * and evaluate L2D cache. Here start_kernel_id and end_kernel_id is the range  of kernels that should be 
       * executed during this pass. */
      int start_kernel_id = pass * cc_config[SM70].max_concurrent_kernels_num;
      int end_kernel_id = std::min((pass + 1) * cc_config[SM70].max_concurrent_kernels_num - 1, 
                                   tracer.get_appcfg()->get_kernels_num() - 1);
      
      /* Here we traversal all the kernels that should be executed during this pass, to create their kernel-info 
       * object. And then the kernels that belong to the same SM will to be used to evaluate L1D cache. */
      for (int kid = start_kernel_id; kid <= end_kernel_id; kid++) {
        kernel_trace_t * kernel_trace_info = tracer.parse_kernel_info(kid, PRINT_LOG);
        trace_kernel_info_t *kernel_info = create_kernel_info(kernel_trace_info, &tracer);
        single_pass_kernels_info.push_back(kernel_info);
      }

      if (PRINT_LOG) std::cout << "    Kernel nums waiting for processing: " << single_pass_kernels_info.size() << std::endl;

      /* Now we have got the kernel-info object of all the kernels that should be executed during this pass,
       * what we should do now is traversal these kernels, and find out all the thread blocks belong to the 
       * same SM, with the usage of issue.config. The SM_traces stores all the memory traces that belong to
       * the same SM. */
      std::map<int, std::vector<mem_instn>>* SM_traces = &SM_traces_all_passes[pass];

      auto issuecfg = tracer.get_issuecfg();

      for (auto k : single_pass_kernels_info) {

        if (PRINT_LOG) std::cout << "      kernel_id[" << k->get_trace_info()->kernel_id 
                                 << "] | kernel_name[" << k->get_trace_info()->kernel_name << "]" << std::endl;
        
        /* unsigned start_warp = 0;
         * unsigned num_threads_per_thread_block = k->get_trace_info()->tb_dim_x * 
         *                                         k->get_trace_info()->tb_dim_y * 
         *                                         k->get_trace_info()->tb_dim_z;
         * unsigned end_warp = int(num_threads_per_thread_block / MAX_WARP_SIZE) - 1;
         * unsigned num_warps_per_thread_block = end_warp - start_warp + 1; // per block */

        unsigned num_threadblocks_current_kernel = k->get_trace_info()->grid_dim_x * 
                                                   k->get_trace_info()->grid_dim_y * 
                                                   k->get_trace_info()->grid_dim_z;
        /* The threadblock_traces[i] stores the memory traces that belong to k->get_trace_info()->kernel_id 
         * and thread block i. */
        std::vector<std::vector<mem_instn>> threadblock_traces;
        threadblock_traces.resize(num_threadblocks_current_kernel);

        /* if (PRINT_LOG) std::cout << "        start_warp: " << start_warp << std::endl;
         * if (PRINT_LOG) std::cout << "        end_warp: " << end_warp << std::endl;
         * if (PRINT_LOG) std::cout << "        num_threads_per_thread_block: " << num_threads_per_thread_block << std::endl;
         * if (PRINT_LOG) std::cout << "        num_warps_per_thread_block: " << num_warps_per_thread_block << std::endl; */
        if (PRINT_LOG) std::cout << "        num_threadblocks_current_kernel: " << num_threadblocks_current_kernel << std::endl;

        /* Traversal all the blocks of kernel k. */
        for (unsigned i = 0; i < num_threadblocks_current_kernel; i++) {
          /* The threadblock_traces[i] stores the memory traces that belong to k->get_trace_info()->kernel_id 
           * and thread block i. */
          threadblock_traces[i] = k->get_one_kernel_one_threadblock_traces(k->get_trace_info()->kernel_id - 1, i);

          /* Calculate the allocated SM index of current thread block i. */
          unsigned kernel_id = k->get_trace_info()->kernel_id - 1;
          int sm_id = issuecfg->get_sm_id_of_one_block(unsigned(kernel_id + 1), unsigned(i));
          
          // SM_traces[sm_id].insert(SM_traces[sm_id].end(), threadblock_traces[i].begin(), threadblock_traces[i].end()); // old
          (*SM_traces)[sm_id].insert((*SM_traces)[sm_id].end(), threadblock_traces[i].begin(), threadblock_traces[i].end());
        }

        /* Next we will interleave the threadblock_traces[...] to the whole traces belong to kernel_id. */
      }

      /* Traversal the SM_traces, theoretically, SM_traces.size() is the total usage amount of SM. */
      assert((int)(*SM_traces).size() == issuecfg->get_trace_issued_sms_num());
      for (unsigned i=0; i < (*SM_traces).size(); i++) {
        if (PRINT_LOG) std::cout << "        SM[" << i << "] | traces_num[" << (*SM_traces)[i].size() << "]:" << std::endl;
        /* Reorder SM_trace[i] by timestamp, from smallest to largest, so as to simulate the issue order 
         * of memory instructions. */
        if (sort) std::sort((*SM_traces)[i].begin(), (*SM_traces)[i].end(), compare);

        /* Output the traces. */
        if (PRINT_LOG) print_SM_traces(&(*SM_traces)[i]);

        /* Now we can evaluate the hit rate of L1D cache of SM[i]. */
      }

      for (auto k : single_pass_kernels_info) {
        delete k;
      }
      single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
                                                                             cc_config[SM70].max_concurrent_kernels_num) : 1);
    }

#ifdef USE_BOOST
    /* Now we need to broadcast the data in SM_traces_all_passes. */
    boost::mpi::broadcast(world, SM_traces_all_passes, 0);
#endif

#ifdef USE_BOOST
  } /* end rank = 0 */
#endif

#ifdef USE_BOOST
  /* Now we need to recieve the broadcasted data to SM_traces_all_passes for all rank > 0. */
  if (world.rank() != 0)  boost::mpi::broadcast(world, SM_traces_all_passes, 0);
#endif

#ifdef USE_BOOST
  /* Just a simple test for MPI. */
  /* simple_mpi_test(argc, argv); */
  
  /* Set a barrier here to ensure all processes have received the data before proceeding. */
  world.barrier();

  /* Print the SM_traces of every MPI rank. &SM_traces_all_passes[0] is the first pass. */
  private_L1_cache_hit_rate_evaluate(argc, argv, &SM_traces_all_passes[0]);
#endif

  fflush(stdout);

  return 0;
}
