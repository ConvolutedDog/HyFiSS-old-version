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
#include "./parda/parda.h"

trace_kernel_info_t *create_kernel_info(kernel_trace_t* kernel_trace_info,
							                          trace_parser *parser){
  dim3 gridDim(kernel_trace_info->grid_dim_x, kernel_trace_info->grid_dim_y, kernel_trace_info->grid_dim_z);
  dim3 blockDim(kernel_trace_info->tb_dim_x, kernel_trace_info->tb_dim_y, kernel_trace_info->tb_dim_z);
  trace_kernel_info_t *kernel_info =
      new trace_kernel_info_t(gridDim, blockDim, parser, kernel_trace_info);

  return kernel_info;
}

bool compare_stamp(const mem_instn &a, const mem_instn &b) {
  // std::cout << "    a.time_stamp: " << a.time_stamp << " ";
  // std::cout << "    b.time_stamp: " << b.time_stamp << " pc-" << b.pc << std::endl;
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
void simple_print_test(int argc, char **argv, std::map<int, std::vector<mem_instn>>* SM_traces_ptr, int pass_issue) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  const int SM_traces_ptr_size = (int)(*SM_traces_ptr).size();
  /* Every rank process one *(SM_traces_ptr)[i], it will have pass_num passes to complete, 
   * which also means that every rank should process at most pass_num traces. */
  const int pass_num = int((SM_traces_ptr_size + world.size() - 1)/world.size());

  for (int pass = 0; pass < pass_num; pass++) {
    int curr_process_idx = world.rank() + pass * world.size();
    std::cout << "rank-" << std::dec << world.rank() << ", " << "pass-" << pass << ", ";
    std::cout << "curr_process_idx: " << curr_process_idx << std::endl;
    if (curr_process_idx < SM_traces_ptr_size) {
      // TODO: implement the private cache hit rate evaluate.
      for (auto mem_ins : (*SM_traces_ptr)[curr_process_idx]) {
        std::cout << "rank-" << std::dec << world.rank() << ", " << "SM-" << curr_process_idx << " ";
        std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
        std::cout << std::hex << mem_ins.time_stamp << " ";
        std::cout << std::hex << mem_ins.addr[0] << std::endl;
      }
    }
  }
}
#endif

int getIthKey(std::map<int, std::vector<mem_instn>>* SM_traces_ptr, int i) {
  auto it = (*SM_traces_ptr).begin();
  std::advance(it, i);
  /* std::cout << "@@@ " << (*SM_traces_ptr).size() << " " << i << " " << it->first << std::endl; */
  return it->first;
}

#ifdef USE_BOOST
void private_L1_cache_hit_rate_evaluate_boost(int argc, char **argv, std::map<int, std::vector<mem_instn>>* SM_traces_all_passes_merged, 
                                              int SM_traces_ptr_size, std::vector<int>* SM_traces_sm_id, int _tmp_print_) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  // Every rank process one *(SM_traces_all_passes_merged)[i], it will have pass_num passes to complete, 
  // which also means that every rank should process at most pass_num traces. 
  const int pass_num = int((SM_traces_ptr_size + world.size() - 1)/world.size());
  // std::cout << "SM_traces_ptr_size: " << SM_traces_ptr_size << std::endl;

  unsigned l1_cache_line_size = 32; // BUG: need configure

  for (int pass = 0; pass < pass_num; pass++) {
    int curr_process_idx_rank = world.rank() + pass * world.size();
    int curr_process_idx;
    if (curr_process_idx_rank < SM_traces_ptr_size) {
      curr_process_idx = (*SM_traces_sm_id)[curr_process_idx_rank];
      if (_tmp_print_ == world.rank()) std::cout << "  L1-" << world.rank() << " " << curr_process_idx << std::endl;
    } else continue;

    if (_tmp_print_ == world.rank()) std::cout << "rank-" << std::dec << world.rank() << ", " << "pass-" << pass << ", ";
    if (_tmp_print_ == world.rank()) std::cout << "curr_process_idx_rank: " << curr_process_idx_rank << std::endl;
    if (_tmp_print_ == world.rank()) std::cout << "curr_process_sm_id: " << curr_process_idx << " " << (int)(*SM_traces_all_passes_merged).size() << std::endl;
    if (curr_process_idx_rank < SM_traces_ptr_size) {
      HKEY input;
      long tim = 0;
      program_data_t pdt_c = parda_init();

      // TODO: implement the private cache hit rate evaluate.
      for (auto mem_ins : (*SM_traces_all_passes_merged)[curr_process_idx]) {
        if (world.rank() == 0) {
          // std::cout << "rank-" << std::dec << world.rank() << ", " << "SM-" << curr_process_idx << " ";
          // std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
          // std::cout << std::hex << mem_ins.time_stamp << " ";
          // std::cout << std::hex << mem_ins.addr[0] << std::endl;
        }
        // HKEY input should be char* of addr
        for (unsigned j = 0; j < (mem_ins.addr).size(); j++) { // BUG: mask + merge
          sprintf(input, "0x%llx", mem_ins.addr[j] >> int(log2(l1_cache_line_size)));
          // std::cout << input << std::endl;
          process_one_access(input, &pdt_c, tim);
          tim++;
        }
      }

      program_data_t* pdt = &pdt_c;
      pdt->histogram[B_INF] += narray_get_len(pdt->ga);
      if (_tmp_print_ == world.rank()) parda_print_histogram(pdt->histogram);
      parda_free(pdt);
    }
  }
}
#else
void private_L1_cache_hit_rate_evaluate(int argc, char **argv, std::map<int, std::vector<mem_instn>>* SM_traces_ptr, int pass_issue) { // BUG， have not modified
  const int SM_traces_ptr_size = (int)(*SM_traces_ptr).size();
  /* Every rank process one *(SM_traces_ptr)[i], it will have pass_num passes to complete, 
   * which also means that every rank should process at most pass_num traces. */
  const int pass_num = SM_traces_ptr_size;

  HKEY input;
  long tim = 0;
  program_data_t pdt_c = parda_init();

  unsigned l1_cache_line_size = 32; // BUG: need configure

  for (int pass = 0; pass < pass_num; pass++) {
    // TODO: implement the private cache hit rate evaluate.
    int curr_process_sm_id = getIthKey(SM_traces_ptr, pass);
    for (auto mem_ins : (*SM_traces_ptr)[curr_process_sm_id]) {
      // std::cout << "SM-" << pass << " ";
      // std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
      // std::cout << std::hex << mem_ins.time_stamp << " ";
      // std::cout << std::hex << mem_ins.addr[0] << std::endl;
      for (unsigned j = 0; j < (mem_ins.addr).size(); j++) { // BUG: mask + merge
        sprintf(input, "0x%llx", mem_ins.addr[j] >> int(log2(l1_cache_line_size)));
        process_one_access(input, &pdt_c, tim);
        tim++;
      }
    }
  }

  program_data_t* pdt = &pdt_c;
  pdt->histogram[B_INF] += narray_get_len(pdt->ga);
  parda_print_histogram(pdt->histogram);
  parda_free(pdt);
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

  int _tmp_print_;

  app.add_option("--tmp", _tmp_print_, "tmp");

  CLI11_PARSE(app, argc, argv);

  int passnum_concurrent_issue_to_sm = -1;

  trace_parser tracer(configs.c_str());


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


    /* V100 schedules 128 kernels for concurrent execution at a time, thus requiring (all_kernels_num + 127)/128 schedules. */
    passnum_concurrent_issue_to_sm = int((tracer.get_appcfg()->get_kernels_num() + 
                                         cc_config[SM70].max_concurrent_kernels_num - 1) / 
                                         cc_config[SM70].max_concurrent_kernels_num);
    // std::cout << "get_kernels_num:" << tracer.get_appcfg()->get_kernels_num() << std::endl;
    // std::cout << "max_concurrent_kernels_num:" << cc_config[SM70].max_concurrent_kernels_num << std::endl;
    std::cout << std::endl;
#ifdef USE_BOOST
  } /* end rank = 0 */
#endif

  /* SM_traces_all_passes will store all the SM_traces of all the passes, and the SM_traces_all_passes[pass_num]
   * means: pass_num -> sm_id -> std::vector<mem_instn>. */
  std::vector<std::map<int, std::vector<mem_instn>>> SM_traces_all_passes;
  
  /*  */
  std::vector<std::vector<block_info_t>> trace_issued_sm_id_blocks;

  std::vector<int> SM_traces_sm_id;
  int SM_traces_ptr_size;

#ifdef USE_BOOST
  if (world.rank() == 0) {
#endif
    trace_issued_sm_id_blocks = *(tracer.get_issuecfg()->get_trace_issued_sm_id_blocks());

    SM_traces_all_passes.resize(passnum_concurrent_issue_to_sm);

    for (int pass = 0; pass < passnum_concurrent_issue_to_sm; pass++) {
      if (PRINT_LOG) std::cout << "Schedule pass: " << pass << std::endl;

      
      std::vector<trace_kernel_info_t*> single_pass_kernels_info;
      
      if (pass == passnum_concurrent_issue_to_sm - 1) {
        single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? tracer.get_appcfg()->get_kernels_num() - 
                                                                      cc_config[SM70].max_concurrent_kernels_num * pass : 1);
      } else if (pass == 0) {
        single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
                                                                               cc_config[SM70].max_concurrent_kernels_num) : 1);
      } else {
        single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? cc_config[SM70].max_concurrent_kernels_num : 1);
      }

      // single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
      //                                                                        cc_config[SM70].max_concurrent_kernels_num) : 1);
      
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
          // std::cout << "sm_id-" << sm_id << " " << threadblock_traces[i].size() << std::endl;
        }

        /* Next we will interleave the threadblock_traces[...] to the whole traces belong to kernel_id. */
      }

      // for (auto iter : (*SM_traces)) {
      //   std::cout << "###SM-" << iter.first << " " << iter.second.size() << std::endl;
      // }
      
      /* Traversal the SM_traces, theoretically, SM_traces.size() is the total usage amount of SM. */
      // assert((int)(*SM_traces).size() == issuecfg->get_trace_issued_sms_num()); // BUG: only pass_num = 1 will be true
      /* for (unsigned i=0; i < (*SM_traces).size(); i++) { // BUG */
      for (auto iter : (*SM_traces)) {                      // fixed
        unsigned i = iter.first;                            // fixed
        if (PRINT_LOG) std::cout << std::dec << "        SM[" << i << "] | traces_num[" << (*SM_traces)[i].size() << "]:" << std::endl;
        /* Reorder SM_trace[i] by timestamp, from smallest to largest, so as to simulate the issue order 
         * of memory instructions. */
        if (sort) std::sort((*SM_traces)[i].begin(), (*SM_traces)[i].end(), compare_stamp); // BUG: vector-add will be wrong
        
        /* Output the traces. */
        //if (PRINT_LOG) print_SM_traces(&(*SM_traces)[i]);

        /* Now we can evaluate the hit rate of L1D cache of SM[i]. */
      }

      // for (unsigned i = 0; i < SM_traces_all_passes.size(); i++) {
      //   for (auto& pair : SM_traces_all_passes[i]) {
      //     std::sort(pair.second.begin(), pair.second.end(), compare_stamp);
      //   }
      // }

      // for (auto k : single_pass_kernels_info) {
      //   delete k;
      // }
      // single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
      //                                                                        cc_config[SM70].max_concurrent_kernels_num) : 1);
    }

    
    for (int _pass = 0; _pass < passnum_concurrent_issue_to_sm; _pass++) {
      for (auto _sm_id_map : SM_traces_all_passes[_pass]) {
        if (std::find(SM_traces_sm_id.begin(), SM_traces_sm_id.end(), _sm_id_map.first) == SM_traces_sm_id.end()) {
          SM_traces_sm_id.push_back(_sm_id_map.first);
        }
      }
    }

    // for (auto x : SM_traces_sm_id) std::cout << x << " ";
    std::cout << std::endl;

    SM_traces_ptr_size = SM_traces_sm_id.size();
    
#ifdef USE_BOOST
    /* Now we need to broadcast the data in SM_traces_all_passes. */
    if (world.size() > 0) boost::mpi::broadcast(world, SM_traces_all_passes, 0);
    /* Also we need to broadcast the variable passnum_concurrent_issue_to_sm. */
    if (world.size() > 0) boost::mpi::broadcast(world, passnum_concurrent_issue_to_sm, 0);
    /* Also we need to broadcast the variable trace_parser for all rank > 0. */
    if (world.size() > 0) boost::mpi::broadcast(world, trace_issued_sm_id_blocks, 0);
    if (world.size() > 0) boost::mpi::broadcast(world, SM_traces_sm_id, 0);
    if (world.size() > 0) boost::mpi::broadcast(world, SM_traces_ptr_size, 0);
#endif

#ifdef USE_BOOST
  } /* end rank = 0 */
#endif

#ifdef USE_BOOST
  /* Now we need to recieve the broadcasted data to SM_traces_all_passes for all rank > 0. */
  if (world.rank() != 0) boost::mpi::broadcast(world, SM_traces_all_passes, 0);
  /* Also we need to recieve the variable passnum_concurrent_issue_to_sm for all rank > 0. */
  if (world.rank() != 0) boost::mpi::broadcast(world, passnum_concurrent_issue_to_sm, 0);
  /* Also we need to recieve the variable trace_parser for all rank > 0. */
  if (world.rank() != 0) boost::mpi::broadcast(world, trace_issued_sm_id_blocks, 0);
  if (world.rank() != 0) boost::mpi::broadcast(world, SM_traces_sm_id, 0);
  if (world.rank() != 0) boost::mpi::broadcast(world, SM_traces_ptr_size, 0);
#endif

#ifdef USE_BOOST
  /* Just a simple test for MPI. */
  /* simple_mpi_test(argc, argv); */
  
  /* Set a barrier here to ensure all processes have received the data before proceeding. */
  world.barrier();

  /* Print the SM_traces of every MPI rank. &SM_traces_all_passes[0] is the first pass. */
  /* std::vector<std::map<int, std::vector<mem_instn>>> SM_traces_all_passes; */
  for(int i = 0 ; i < passnum_concurrent_issue_to_sm; i++) { // BUG: need to merge all the SM_traces of all the passes.
    // private_L1_cache_hit_rate_evaluate_boost(argc, argv, &SM_traces_all_passes[i], i);
  }
  // std::cout << "###" << passnum_concurrent_issue_to_sm << std::endl;


  
  if (passnum_concurrent_issue_to_sm > 1) {
    /* Now we try to merge all the SM_traces of all the passes. Using boost::mpi, we can let rank-0 process 
    * 0+0*world.size(), 0+1*world.size() -th SM_id. */
    std::map<int, std::vector<mem_instn>> SM_traces_all_passes_merged;

    /* num_merge_sms_pass is int((SM number + world.size() - 1) / world.size()), means that every rank will process at most 
    * num_merge_sms_pass SMs. */
    
    // std::cout << "SM_traces_ptr_size: " << SM_traces_ptr_size << std::endl;

    int num_merge_sms_pass = int((SM_traces_ptr_size + world.size() - 1) / world.size());
    if (PRINT_LOG) std::cout << "num_merge_sms_pass: " << num_merge_sms_pass << std::endl;
    // std::cout << "num_merge_sms_pass: " << num_merge_sms_pass << std::endl;

    for (int pass = 0; pass < num_merge_sms_pass; pass++) {
      int curr_process_idx_rank = world.rank() + pass * world.size();
      int curr_process_idx;
      if (curr_process_idx_rank < SM_traces_ptr_size) 
        /* curr_process_idx is the SM index that world.rank() should process. SM_traces_sm_id stores
         * all the SM indexes that have been issued by the trace parser. */
        curr_process_idx = SM_traces_sm_id[curr_process_idx_rank];
      else
        continue;
      
      if (PRINT_LOG) std::cout << "rank-" << std::dec << world.rank() << ", " << "pass-" << pass << ", ";
      if (PRINT_LOG) std::cout << "curr_process_idx: " << curr_process_idx << std::endl;

      /* merge SM_traces_all_passes[i=1...passnum_concurrent_issue_to_sm][curr_process_idx] to
       * SM_traces_all_passes_merged. */
      for (int i = 0; i < passnum_concurrent_issue_to_sm; i++) {
        // int curr_process_sm_id = getIthKey(&SM_traces_all_passes[i], curr_process_idx);
        if (SM_traces_all_passes[i].find(curr_process_idx) != SM_traces_all_passes[i].end())
          if (SM_traces_all_passes[i][curr_process_idx].size() > 0) {
            SM_traces_all_passes_merged[curr_process_idx].insert(SM_traces_all_passes_merged[curr_process_idx].end(), 
                                                                 SM_traces_all_passes[i][curr_process_idx].begin(), 
                                                                 SM_traces_all_passes[i][curr_process_idx].end());
          }
      }
    }

    // for (int i = 0; i < passnum_concurrent_issue_to_sm; i++) {
    //   for (auto iter : SM_traces_all_passes[i]) {
    //     std::cout << "@@@ sm_id-" << iter.first << " size" << iter.second.size() << std::endl;
    //   }
    // }

    if (world.rank() == _tmp_print_) {
      for (auto x : SM_traces_all_passes_merged) {
        // std::cout << "### rankkk-" << world.rank() << " sm_id-" << x.first << " size" << x.second.size() << std::endl;
        std::vector<mem_instn> mem_instns = x.second;
        for (auto mem_ins : mem_instns) {
          // std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
          // std::cout << std::hex << mem_ins.time_stamp << " ";
          // std::cout << std::hex << mem_ins.addr[0] << std::endl;
        }
      }
    }
    
    /* We have merged SM_traces_all_passes[i=1...passnum_concurrent_issue_to_sm][curr_process_idx] to
     * SM_traces_all_passes_merged, now SM_traces_all_passes_merged[curr_process_idx] stores all the
     * memory addrs from SM-curr_process_idx. We should sort the addrs by the time_stamp to simulator 
     * the order they are issued to L1 Cache. */
    if (sort) {
      for (int pass = 0; pass < num_merge_sms_pass; pass++) {
        int curr_process_idx_rank = world.rank() + pass * world.size();
        int curr_process_idx;
        if (curr_process_idx_rank < SM_traces_ptr_size) {
          curr_process_idx = SM_traces_sm_id[curr_process_idx_rank];
          if (world.rank() == 0) std::cout << world.rank() << " " << curr_process_idx << std::endl;
          if (world.rank() == 1) std::cout << world.rank() << " " << curr_process_idx << std::endl;
          if (world.rank() == 2) std::cout << world.rank() << " " << curr_process_idx << std::endl;
        } else continue;

        // std::cout << "### rank-" << std::dec << world.rank() << " curr_process_idx: " << curr_process_idx << std::endl;
        if (curr_process_idx == 18) 
          std::cout << (SM_traces_all_passes_merged.find(curr_process_idx) != SM_traces_all_passes_merged.end()) 
                    << /*" " << SM_traces_all_passes_merged[curr_process_idx].size() <<*/ std::endl;
        if (SM_traces_all_passes_merged.find(curr_process_idx) != SM_traces_all_passes_merged.end())
          if (SM_traces_all_passes_merged[curr_process_idx].size() > 0)
              std::sort(SM_traces_all_passes_merged[curr_process_idx].begin(), SM_traces_all_passes_merged[curr_process_idx].end(), compare_stamp);
      }
    }

    /* Process L1 Cache Hit rate. */
    private_L1_cache_hit_rate_evaluate_boost(argc, argv, &SM_traces_all_passes_merged, SM_traces_ptr_size, &SM_traces_sm_id, _tmp_print_);
  } else {
    /* In this case, passnum_concurrent_issue_to_sm == 1 */
    /* We have merged SM_traces_all_passes[i=1...passnum_concurrent_issue_to_sm][curr_process_idx] to
     * SM_traces_all_passes_merged, now SM_traces_all_passes_merged[curr_process_idx] stores all the
     * memory addrs from SM-curr_process_idx. We should sort the addrs by the time_stamp to simulator 
     * the order they are issued to L1 Cache. */
    if (sort) {
      int num_merge_sms_pass = int((SM_traces_ptr_size + world.size() - 1) / world.size());
      for (int pass = 0; pass < num_merge_sms_pass; pass++) {
        int curr_process_idx_rank = world.rank() + pass * world.size();
        int curr_process_idx;
        if (curr_process_idx_rank < SM_traces_ptr_size) 
          /* curr_process_idx is the SM index that world.rank() should process. SM_traces_sm_id stores
           * all the SM indexes that have been issued by the trace parser. */
          curr_process_idx = SM_traces_sm_id[curr_process_idx_rank];
        else
          continue;
        
        if (PRINT_LOG) std::cout << "rank-" << std::dec << world.rank() << ", " << "pass-" << pass << ", ";
        if (PRINT_LOG) std::cout << "curr_process_idx: " << curr_process_idx << std::endl;

        
        if (SM_traces_all_passes[0].find(curr_process_idx) != SM_traces_all_passes[0].end())
          if (SM_traces_all_passes[0][curr_process_idx].size() > 0)
              std::sort(SM_traces_all_passes[0][curr_process_idx].begin(), SM_traces_all_passes[0][curr_process_idx].end(), compare_stamp);
      }
    }
  }

#else
  /* Print the SM_traces of every MPI rank. &SM_traces_all_passes[0] is the first pass. */
  for(int i = 0 ; i < passnum_concurrent_issue_to_sm; i++) {
    //private_L1_cache_hit_rate_evaluate(argc, argv, &SM_traces_all_passes[i], i);
  }
#endif

#ifdef USE_BOOST
  // std::cout << "END: " << world.rank() << std::endl;
  world.barrier();
#endif

  fflush(stdout);
  
  return 0;
}
