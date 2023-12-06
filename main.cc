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
#include "./parda/parda.h"

#include <chrono>

trace_kernel_info_t *create_kernel_info(kernel_trace_t* kernel_trace_info,
							                          trace_parser *parser){
  dim3 gridDim(kernel_trace_info->grid_dim_x, kernel_trace_info->grid_dim_y, kernel_trace_info->grid_dim_z);
  dim3 blockDim(kernel_trace_info->tb_dim_x, kernel_trace_info->tb_dim_y, kernel_trace_info->tb_dim_z);
  trace_kernel_info_t *kernel_info =
      new trace_kernel_info_t(gridDim, blockDim, parser, kernel_trace_info);

  return kernel_info;
}

bool compare_stamp(const mem_instn a, const mem_instn b) {
  // std::cout << "    a.time_stamp: " << a.time_stamp << " ";
  // std::cout << "    b.time_stamp: " << b.time_stamp << " pc-" << b.pc << std::endl;
  return a.time_stamp < b.time_stamp;
}

void print_SM_traces(std::vector<mem_instn>* traces) {
  for (auto mem_ins : *traces) {
    std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
    std::cout << std::hex << mem_ins.time_stamp << " ";
    std::cout << std::hex << mem_ins.addr[0] << std::endl;
  }
}


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
    // std::cout << "rank-" << std::dec << world.rank() << ", " << "pass-" << pass << ", ";
    // std::cout << "curr_process_idx: " << curr_process_idx << std::endl;
    if (curr_process_idx < SM_traces_ptr_size) {
      // TODO: implement the private cache hit rate evaluate.
      for (auto mem_ins : (*SM_traces_ptr)[curr_process_idx]) {
        // std::cout << "rank-" << std::dec << world.rank() << ", " << "SM-" << curr_process_idx << " ";
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
void private_L1_cache_stack_distance_evaluate_boost_no_concurrent(int argc, 
                                                                  char **argv, 
                                                                  std::vector<std::map<int, std::vector<mem_instn>>>* SM_traces_all_passes, 
                                                                  int _tmp_print_, 
                                                                  std::string configs_dir,
                                                                  bool dump_histogram) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  /* gpu_config[V100].num_sm is the number of the SMs that have been used during the execution. */
  const int pass_num = int((gpu_config[V100].num_sm + world.size() - 1)/world.size());

  unsigned l1_cache_line_size = 32; // BUG: need configure
  unsigned l1_cache_size = 32 * 1024; // BUG: need configure
  unsigned l1_cache_associativity = 64; // BUG: need configure
  unsigned l1_cache_blocks = l1_cache_size / l1_cache_line_size; // BUG: need configure

  std::cout << std::endl;

  for (int pass = 0; pass < pass_num; pass++) {
    int curr_process_idx_rank = world.rank() + pass * world.size();
    int curr_process_idx;
    if (curr_process_idx_rank < gpu_config[V100].num_sm) {
      curr_process_idx = curr_process_idx_rank;
    } else continue;

    /* HKEY input should be char* of addr */
    HKEY input;
    long tim;
    program_data_t pdt_c;
    program_data_t* pdt;
    FILE* file;
    std::string parda_histogram_filepath;
    for (unsigned kid = 0; kid < (*SM_traces_all_passes).size() ; kid++) {
      unsigned miss_num_all_acc = 0;
      unsigned num_all_acc = 0;

      tim = 0;
      pdt_c = parda_init();
      /* Here we ONLY USE the curr_process_idx-th items of SM_traces_all_passes, so for every rank, 
       * we only load the curr_process_idx-th items for all kernels into SM_traces_all_passes, which 
       * significantly accelerate the simulation speed. */
      for (auto mem_ins : (*SM_traces_all_passes)[kid][curr_process_idx]) { 
        // if (world.rank() == 0) {
        //   std::cout << "rank-" << std::dec << world.rank() << ", " << "SM-" << curr_process_idx << " " << "kid-" << kid << " ";
        //   std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
        //   std::cout << std::hex << mem_ins.time_stamp << " ";
        //   std::cout << std::hex << mem_ins.addr[0] << std::endl;
        // }
        
        std::vector<unsigned long long> have_got_line_addr; // use have_got_line_addr will cause not accurate hit rate 
        for (unsigned j = 0; j < (mem_ins.addr).size(); j++) {
          unsigned long long cache_line_addr = mem_ins.addr[j] >> int(log2(l1_cache_line_size));
          /**/ // use have_got_line_addr will cause not accurate hit rate
          if(std::find(have_got_line_addr.begin(), have_got_line_addr.end(), cache_line_addr) != have_got_line_addr.end()) {
            mem_ins.distance[j] = 0;
            // num_all_acc++;
          } else {
          /**/ // use have_got_line_addr will cause not accurate hit rate 
            sprintf(input, "0x%llx", cache_line_addr);
            /* If you only want to dump the histogram of each SM for every kernel, you can also use: 
             *     process_one_access(input, &pdt_c, tim); */
            mem_ins.distance[j] = process_one_access_and_get_distance(input, &pdt_c, tim);
            if (curr_process_idx == 0 && kid == 0) if ((cache_line_addr & 3) == 0)
              // std::cout << "###SM-" << curr_process_idx << " " 
              //                       << std::hex << (mem_ins.addr[j]) << " " 
              //                       << std::hex << (mem_ins.addr[j] >> 7) << " " 
              //                       << std::hex << (mem_ins.addr[j] >> 5) << " " 
              //                       << input << " " 
              //                       << (cache_line_addr & 3) << std::dec << std::endl;
            if (mem_ins.distance[j] > (int)l1_cache_blocks) {
              miss_num_all_acc++;
            }
            num_all_acc++;
            
            tim++;
            have_got_line_addr.push_back(cache_line_addr); // use have_got_line_addr will cause not accurate hit rate 
          /**/ // use have_got_line_addr will cause not accurate hit rate 
          }
          /**/ // use have_got_line_addr will cause not accurate hit rate 
        }
      }

      // if (num_all_acc > 0) if (kid==0)
      // std::cout << "SM-" << curr_process_idx << " kid-" << kid
      //           << " num_all_acc-" << num_all_acc << " miss_num_all_acc-" << miss_num_all_acc 
      //           << " miss rate-" << (double)miss_num_all_acc / num_all_acc << std::endl;

      pdt = &pdt_c;
      pdt->histogram[B_INF] += narray_get_len(pdt->ga);

      if (dump_histogram) {
        if (configs_dir.back() == '/')
          parda_histogram_filepath = configs_dir + "../kernel_" + std::to_string(kid) + "_SM_" + std::to_string(curr_process_idx) + ".histogram";
        else
          parda_histogram_filepath = configs_dir + "/" + "../kernel_" + std::to_string(kid) + "_SM_" + std::to_string(curr_process_idx) + ".histogram";
          
        file = fopen(parda_histogram_filepath.c_str(), "w");
          
        if (file != NULL) {
          parda_fprintf_histogram(pdt->histogram, file);
          fclose(file);
        }
      }
        
      parda_free(pdt);
    }
  }
}
#else
void private_L1_cache_stack_distance_evaluate_no_boost_no_concurrent(int argc, 
                                                                  char **argv, 
                                                                  std::vector<std::map<int, std::vector<mem_instn>>>* SM_traces_all_passes, 
                                                                  int _tmp_print_, 
                                                                  std::string configs_dir,
                                                                  bool dump_histogram) {
  /* gpu_config[V100].num_sm is the number of the SMs that have been used during the execution. */
  const int pass_num = int(gpu_config[V100].num_sm);

  unsigned l1_cache_line_size = 32; // BUG: need configure
  unsigned l1_cache_size = 32 * 1024; // BUG: need configure
  unsigned l1_cache_associativity = 64; // BUG: need configure
  unsigned l1_cache_blocks = l1_cache_size / l1_cache_line_size; // BUG: need configure

  std::cout << std::endl;

  for (int pass = 0; pass < pass_num; pass++) {
    int curr_process_idx_rank = pass;
    int curr_process_idx;
    if (curr_process_idx_rank < gpu_config[V100].num_sm) {
      curr_process_idx = curr_process_idx_rank;
    } else continue;

    /* HKEY input should be char* of addr */
    HKEY input;
    long tim;
    program_data_t pdt_c;
    program_data_t* pdt;
    FILE* file;
    std::string parda_histogram_filepath;
    for (unsigned kid = 0; kid < (*SM_traces_all_passes).size() ; kid++) {
      unsigned miss_num_all_acc = 0;
      unsigned num_all_acc = 0;

      tim = 0;
      pdt_c = parda_init();
      /* Here we ONLY USE the curr_process_idx-th items of SM_traces_all_passes, so for every rank, 
       * we only load the curr_process_idx-th items for all kernels into SM_traces_all_passes, which 
       * significantly accelerate the simulation speed. */
      for (auto mem_ins : (*SM_traces_all_passes)[kid][curr_process_idx]) { 
        std::vector<unsigned long long> have_got_line_addr; // use have_got_line_addr will cause not accurate hit rate 
        for (unsigned j = 0; j < (mem_ins.addr).size(); j++) {
          unsigned long long cache_line_addr = mem_ins.addr[j] >> int(log2(l1_cache_line_size));
          /**/ // use have_got_line_addr will cause not accurate hit rate
          if(std::find(have_got_line_addr.begin(), have_got_line_addr.end(), cache_line_addr) != have_got_line_addr.end()) {
            mem_ins.distance[j] = 0;
            // num_all_acc++;
          } else {
          /**/ // use have_got_line_addr will cause not accurate hit rate 
            sprintf(input, "0x%llx", cache_line_addr);
            /* If you only want to dump the histogram of each SM for every kernel, you can also use: 
             *     process_one_access(input, &pdt_c, tim); */
            mem_ins.distance[j] = process_one_access_and_get_distance(input, &pdt_c, tim);
            if (mem_ins.distance[j] > (int)l1_cache_blocks) {
              miss_num_all_acc++;
            }
            num_all_acc++;
            
            tim++;
            have_got_line_addr.push_back(cache_line_addr); // use have_got_line_addr will cause not accurate hit rate 
          /**/ // use have_got_line_addr will cause not accurate hit rate 
          }
          /**/ // use have_got_line_addr will cause not accurate hit rate 
        }
      }

      // if (num_all_acc > 0) if (kid==0)
      // std::cout << "SM-" << curr_process_idx << " kid-" << kid
      //           << " num_all_acc-" << num_all_acc << " miss_num_all_acc-" << miss_num_all_acc 
      //           << " miss rate-" << (double)miss_num_all_acc / num_all_acc << std::endl;

      pdt = &pdt_c;
      pdt->histogram[B_INF] += narray_get_len(pdt->ga);

      if (dump_histogram) {
        if (configs_dir.back() == '/')
          parda_histogram_filepath = configs_dir + "../kernel_" + std::to_string(kid) + "_SM_" + std::to_string(curr_process_idx) + ".histogram";
        else
          parda_histogram_filepath = configs_dir + "/" + "../kernel_" + std::to_string(kid) + "_SM_" + std::to_string(curr_process_idx) + ".histogram";
          
        file = fopen(parda_histogram_filepath.c_str(), "w");
          
        if (file != NULL) {
          parda_fprintf_histogram(pdt->histogram, file);
          fclose(file);
        }
      }
        
      parda_free(pdt);
    }
  }
}
#endif

int main(int argc, char **argv) {

START_TIMER(0);

#ifdef USE_BOOST
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;
#endif
  
  CLI::App app{"Memory Model."};

  std::string configs;
  bool PRINT_LOG = true;
  bool sort = false;
  bool dump_histogram = false;

  bool PRINT_COMPUTE_LOG = false;

  app.add_option("--configs", configs, "The configs path, which is generated from our NVBit tool, "
                                       "e.g., \"./traces/vectoradd/configs\"");
  app.add_option("--sort", sort, "Simulate the order in which instructions are issued based on their "
                                 "timestamps");
  app.add_option("--log", PRINT_LOG, "Print the traces processing log or not");
  app.add_option("--dump_histogram", dump_histogram, "Dump the histogram of the private L1 cache hit "
                                                     "rate");
  app.add_option("--clog", PRINT_COMPUTE_LOG, "Print the computation traces processing log or not");

  int _tmp_print_;

  app.add_option("--tmp", _tmp_print_, "tmp");

  CLI11_PARSE(app, argc, argv);

  int passnum_concurrent_issue_to_sm = 1;

  trace_parser tracer(configs.c_str());

  tracer.parse_configs_file(PRINT_LOG);

START_TIMER(1);

  std::vector<int> need_to_read_mem_instns_sms;

  const int pass_num = int((gpu_config[V100].num_sm + world.size() - 1)/world.size());
  for (int _pass = 0; _pass < pass_num; _pass++) {
    int curr_process_idx_rank = world.rank() + _pass * world.size();
    /* curr_process_idx is the SM id that should be processed */
    int curr_process_idx = curr_process_idx_rank;
    if (curr_process_idx < gpu_config[V100].num_sm)
    need_to_read_mem_instns_sms.push_back(curr_process_idx);
  }

  // if (world.rank() == 29) for (auto x : need_to_read_mem_instns_sms) std::cout << "@@@ " << x << std::endl;

  std::vector<std::pair<int, int>> need_to_read_mem_instns_kernel_block_pair;
  /* Get pair<kernel_id, block_id> from tracer.get_issuecfg(), using need_to_read_mem_instns_sms. */
  for (auto sm_id : need_to_read_mem_instns_sms) {
    auto result = tracer.get_issuecfg()->get_kernel_block_by_smid(sm_id);
    for (auto pair : result) {
      need_to_read_mem_instns_kernel_block_pair.push_back(pair);
    }
  }

  tracer.read_mem_instns(PRINT_LOG, &need_to_read_mem_instns_kernel_block_pair);

  auto issuecfg = tracer.get_issuecfg();

STOP_AND_REPORT_TIMER_pass(-1, 1);
    
  /* V100 schedules 128 kernels for concurrent execution at a time, thus requiring (all_kernels_num + 127)/128 schedules. */
  passnum_concurrent_issue_to_sm = int((tracer.get_appcfg()->get_kernels_num() + 
                                       (gpgpu_concurrent_kernel_sm ? gpu_config[V100].max_concurrent_kernels_num : 1) - 1) / 
                                       (gpgpu_concurrent_kernel_sm ? gpu_config[V100].max_concurrent_kernels_num : 1));
  // std::cout << "get_kernels_num:" << tracer.get_appcfg()->get_kernels_num() << std::endl;
  // std::cout << "max_concurrent_kernels_num:" << gpu_config[V100].max_concurrent_kernels_num << std::endl;
  // std::cout << std::endl;


  /* SM_traces_all_passes will store all the SM_traces of all the passes, and the SM_traces_all_passes[pass_num]
   * means: pass_num->sm_id->std::vector<mem_instn>. */
  std::vector<std::map<int, std::vector<mem_instn>>> SM_traces_all_passes;
  
  SM_traces_all_passes.resize(passnum_concurrent_issue_to_sm);

START_TIMER(3);

    for (int pass = 0; pass < passnum_concurrent_issue_to_sm; pass++) {
      if (PRINT_LOG) std::cout << "Schedule pass: " << pass << std::endl;

START_TIMER(2);

      std::vector<trace_kernel_info_t*> single_pass_kernels_info;
      
      if (pass == passnum_concurrent_issue_to_sm - 1) {
        single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? tracer.get_appcfg()->get_kernels_num() - 
                                                                      gpu_config[V100].max_concurrent_kernels_num * pass : 1);
      } else if (pass == 0) {
        single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
                                                                               gpu_config[V100].max_concurrent_kernels_num) : 1);
      } else {
        single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? gpu_config[V100].max_concurrent_kernels_num : 1);
      }

      /* for the pass-th scheduling process, V100 will schedule min(128,tracer.kernels_num) kernels to SMs,
       * here we will find the kernels that will be scheduled in this pass and create their kernel_info_t objects,
       * and then to evaluate every L1D cache in SMs, and also will interleave their missed address to L2D cache,
       * and evaluate L2D cache. Here start_kernel_id and end_kernel_id is the range  of kernels that should be 
       * executed during this pass. */
      int start_kernel_id = pass * (gpgpu_concurrent_kernel_sm ? gpu_config[V100].max_concurrent_kernels_num : 1);
      int end_kernel_id = (pass + 1) * (gpgpu_concurrent_kernel_sm ? gpu_config[V100].max_concurrent_kernels_num : 1) - 1;
      
      /* Here we traversal all the kernels that should be executed during this pass, to create their kernel-info 
       * object. And then the kernels that belong to the same SM will to be used to evaluate L1D cache. */
      for (int kid = start_kernel_id; kid <= std::min(end_kernel_id, tracer.get_appcfg()->get_kernels_num() - 1); kid++) {
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

      

      /* pass_num is the SMs that the current rank should process */
      const int pass_num = int((gpu_config[V100].num_sm + world.size() - 1)/world.size());
      for (int _pass = 0; _pass < pass_num; _pass++) {
        int curr_process_idx_rank = world.rank() + _pass * world.size();
        /* curr_process_idx is the SM id that should be processed */
        int curr_process_idx = curr_process_idx_rank;
        if (curr_process_idx < gpu_config[V100].num_sm)
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

          unsigned kernel_id = k->get_trace_info()->kernel_id - 1;

          /* Traversal all the blocks of kernel k. */
          for (unsigned i = 0; i < num_threadblocks_current_kernel; i++) {
            /* Calculate the allocated SM index of current thread block i. */
            
            int sm_id = issuecfg->get_sm_id_of_one_block_fast(unsigned(kernel_id + 1), unsigned(i));
            if (sm_id == curr_process_idx) {
              /* The threadblock_traces[i] stores the memory traces that belong to k->get_trace_info()->kernel_id 
               * and thread block i. */
              threadblock_traces[i] = k->get_one_kernel_one_threadblock_traces(k->get_trace_info()->kernel_id - 1, i);

              // SM_traces[sm_id].insert(SM_traces[sm_id].end(), threadblock_traces[i].begin(), threadblock_traces[i].end()); // old
              (*SM_traces)[sm_id].insert((*SM_traces)[sm_id].end(), threadblock_traces[i].begin(), threadblock_traces[i].end());
              // std::cout << "sm_id-" << sm_id << " " << threadblock_traces[i].size() << std::endl;
            }
          }
          /* Next we will interleave the threadblock_traces[...] to the whole traces belong to kernel_id. */
        }
      }
      

      
      // if (pass == 0 && world.rank() == 0)
      // for (auto iter : (*SM_traces)) { if (iter.first == 0)
      //   for (int _i = 0; _i < iter.second.size(); _i++)
      //     std::cout << "###SM-" << iter.first << " " << std::hex << iter.second[_i].time_stamp << " " 
      //               << std::hex << (iter.second[_i].addr[0] >> 5) << std::dec << std::endl;
      // }
      // std::cout << "=================" << std::endl;
      
      /* Traversal the SM_traces, theoretically, SM_traces.size() is the total usage amount of SM. */
      // assert((int)(*SM_traces).size() == issuecfg->get_trace_issued_sms_num()); // BUG: only pass_num = 1 will be true
      /* for (unsigned i=0; i < (*SM_traces).size(); i++) { // BUG */
      for (auto iter : (*SM_traces)) {                      // fixed
        unsigned i = iter.first;                            // fixed
        if (PRINT_LOG) std::cout << std::dec << "        SM[" << i << "] | traces_num[" << (*SM_traces)[i].size() << "]:" << std::endl;
        /* Reorder SM_trace[i] by timestamp, from smallest to largest, so as to simulate the issue order 
         * of memory instructions. */
        if (sort) std::sort(iter.second.begin(), iter.second.end(), compare_stamp); // BUG: vector-add will be wrong
        /* Output the traces. */
        if (PRINT_LOG) print_SM_traces(&(*SM_traces)[i]);

        /* Now we can evaluate the hit rate of L1D cache of SM[i]. */
      }



      // if (pass == 0 && world.rank() == 0)
      // for (auto iter : (*SM_traces)) { if (iter.first == 0)
      //   for (int _i = 0; _i < iter.second.size(); _i++)
      //     std::cout << "###SM-" << iter.first << " " << iter.second[_i].time_stamp << " " << iter.second[_i].addr[0] << std::endl;
      // }

      // for (unsigned i = 0; i < SM_traces_all_passes.size(); i++) {
      //   for (auto& pair : SM_traces_all_passes[i]) {
      //     std::sort(pair.second.begin(), pair.second.end(), compare_stamp);
      //   }
      // }

      for (auto k : single_pass_kernels_info) {
        delete k;
      }
      single_pass_kernels_info.clear();
      // single_pass_kernels_info.reserve(gpgpu_concurrent_kernel_sm ? std::min(tracer.get_appcfg()->get_kernels_num(), 
      //                                                                        gpu_config[V100].max_concurrent_kernels_num) : 1);

STOP_AND_REPORT_TIMER_pass(pass, 2);

    }

STOP_AND_REPORT_TIMER_pass(-1, 3);

#ifdef USE_BOOST
  /* Set a barrier here to ensure all processes have received the data before proceeding. */
  // world.barrier();

  /* Print the SM_traces of every MPI rank. &SM_traces_all_passes[0] is the first pass. */
  /* std::vector<std::map<int, std::vector<mem_instn>>> SM_traces_all_passes; */
  for(int i = 0 ; i < passnum_concurrent_issue_to_sm; i++) { // BUG: need to merge all the SM_traces of all the passes.
    // private_L1_cache_stack_distance_evaluate_boost(argc, argv, &SM_traces_all_passes[i], i);
  }
  // std::cout << "###" << passnum_concurrent_issue_to_sm << std::endl;

  // if (passnum_concurrent_issue_to_sm > 1 && gpgpu_concurrent_kernel_sm) {
  //   /* Now we try to merge all the SM_traces of all the passes. Using boost::mpi, we can let rank-0 process 
  //   * 0+0*world.size(), 0+1*world.size() -th SM_id. */
  //   std::map<int, std::vector<mem_instn>> SM_traces_all_passes_merged;

  //   /* num_merge_sms_pass is int((SM number + world.size() - 1) / world.size()), means that every rank will process at most 
  //   * num_merge_sms_pass SMs. */
    
  //   // std::cout << "SM_traces_ptr_size: " << SM_traces_ptr_size << std::endl;

  //   int num_merge_sms_pass = int((SM_traces_ptr_size + world.size() - 1) / world.size());
  //   if (PRINT_LOG) std::cout << "num_merge_sms_pass: " << num_merge_sms_pass << std::endl;
  //   // std::cout << "num_merge_sms_pass: " << num_merge_sms_pass << std::endl;

  //   for (int pass = 0; pass < num_merge_sms_pass; pass++) {
  //     int curr_process_idx_rank = world.rank() + pass * world.size();
  //     int curr_process_idx = curr_process_idx_rank;

      
  //     if (PRINT_LOG) std::cout << "rank-" << std::dec << world.rank() << ", " << "pass-" << pass << ", ";
  //     if (PRINT_LOG) std::cout << "curr_process_idx: " << curr_process_idx << std::endl;

  //     /* merge SM_traces_all_passes[i=1...passnum_concurrent_issue_to_sm][curr_process_idx] to
  //      * SM_traces_all_passes_merged. */
  //     for (int i = 0; i < passnum_concurrent_issue_to_sm; i++) {
  //       // int curr_process_sm_id = getIthKey(&SM_traces_all_passes[i], curr_process_idx);
  //       if (SM_traces_all_passes[i].find(curr_process_idx) != SM_traces_all_passes[i].end())
  //         if (SM_traces_all_passes[i][curr_process_idx].size() > 0) {
  //           SM_traces_all_passes_merged[curr_process_idx].insert(SM_traces_all_passes_merged[curr_process_idx].end(), 
  //                                                                SM_traces_all_passes[i][curr_process_idx].begin(), 
  //                                                                SM_traces_all_passes[i][curr_process_idx].end());
  //         }
  //     }
  //   }

  //   // for (int i = 0; i < passnum_concurrent_issue_to_sm; i++) {
  //   //   for (auto iter : SM_traces_all_passes[i]) {
  //   //     std::cout << "@@@ sm_id-" << iter.first << " size" << iter.second.size() << std::endl;
  //   //   }
  //   // }

  //   if (world.rank() == _tmp_print_) {
  //     for (auto x : SM_traces_all_passes_merged) {
  //       // std::cout << "### rankkk-" << world.rank() << " sm_id-" << x.first << " size" << x.second.size() << std::endl;
  //       std::vector<mem_instn> mem_instns = x.second;
  //       for (auto mem_ins : mem_instns) {
  //         // std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
  //         // std::cout << std::hex << mem_ins.time_stamp << " ";
  //         // std::cout << std::hex << mem_ins.addr[0] << std::endl;
  //       }
  //     }
  //   }
    
  //   /* We have merged SM_traces_all_passes[i=1...passnum_concurrent_issue_to_sm][curr_process_idx] to
  //    * SM_traces_all_passes_merged, now SM_traces_all_passes_merged[curr_process_idx] stores all the
  //    * memory addrs from SM-curr_process_idx. We should sort the addrs by the time_stamp to simulator 
  //    * the order they are issued to L1 Cache. */
  //   if (sort) {
  //     for (int pass = 0; pass < num_merge_sms_pass; pass++) {
  //       int curr_process_idx_rank = world.rank() + pass * world.size();
  //       int curr_process_idx;
  //       if (curr_process_idx_rank < SM_traces_ptr_size) {
  //         curr_process_idx = SM_traces_sm_id[curr_process_idx_rank];
  //         if (world.rank() == 0) std::cout << world.rank() << " " << curr_process_idx << std::endl;
  //         if (world.rank() == 1) std::cout << world.rank() << " " << curr_process_idx << std::endl;
  //         if (world.rank() == 2) std::cout << world.rank() << " " << curr_process_idx << std::endl;
  //       } else continue;

  //       // std::cout << "### rank-" << std::dec << world.rank() << " curr_process_idx: " << curr_process_idx << std::endl;
  //       if (SM_traces_all_passes_merged.find(curr_process_idx) != SM_traces_all_passes_merged.end())
  //         if (SM_traces_all_passes_merged[curr_process_idx].size() > 0)
  //             std::sort(SM_traces_all_passes_merged[curr_process_idx].begin(), SM_traces_all_passes_merged[curr_process_idx].end(), compare_stamp);
  //     }
  //   }

  //   /* Process L1 Cache Hit rate. */
  //   private_L1_cache_stack_distance_evaluate_boost(argc, argv, &SM_traces_all_passes_merged, SM_traces_ptr_size, &SM_traces_sm_id, _tmp_print_, configs);
  // } else {
    /* In this case, passnum_concurrent_issue_to_sm == 1 */
    /* We have merged SM_traces_all_passes[i=1...passnum_concurrent_issue_to_sm][curr_process_idx] to
     * SM_traces_all_passes_merged, now SM_traces_all_passes_merged[curr_process_idx] stores all the
     * memory addrs from SM-curr_process_idx. We should sort the addrs by the time_stamp to simulator 
     * the order they are issued to L1 Cache. */

START_TIMER(4);

  /**********************************************************************************************/
  /***                                                                                        ***/
  /***                              Calculate the stack distance.                             ***/
  /***                                                                                        ***/
  /**********************************************************************************************/

  private_L1_cache_stack_distance_evaluate_boost_no_concurrent(argc, argv, &SM_traces_all_passes,  _tmp_print_, configs, dump_histogram);

STOP_AND_REPORT_TIMER_rank(world.rank(), 4);

  // }

#else
  /* Print the SM_traces of every MPI rank. &SM_traces_all_passes[0] is the first pass. */
  for(int i = 0 ; i < passnum_concurrent_issue_to_sm; i++) {
    //private_L1_cache_stack_distance_evaluate(argc, argv, &SM_traces_all_passes[i], i);
  }
#endif

#ifdef USE_BOOST
  // std::cout << "END: " << world.rank() << std::endl;
  // world.barrier();
#endif

START_TIMER(5);

  /**********************************************************************************************/
  /***                                                                                        ***/
  /***                              Read the computation instns.                              ***/
  /***                                                                                        ***/
  /**********************************************************************************************/

  /* Dueing the read_compute_instns process, we split the conpute traces into several parts by the 
   * global warp id, and every rank only read the traces whose global warp id equals to the SM id
   * that the rank corresponds to. For computation simulation, we also need to transfer the simple 
   * traces to trace_warp_inst_t objects, for the reason that in real conputation simulation, the 
   * class trace_warp_inst_t will provide more instruction details that are what we need. */
  tracer.read_compute_instns(PRINT_COMPUTE_LOG, &need_to_read_mem_instns_kernel_block_pair);
  
  if (world.rank() == 0) {
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;

    compute_instn* tmp = tracer.get_one_kernel_one_warp_one_instn(42, 2, 3);
    _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
    trace_warp_inst_t* tmp_trace_warp_inst = &(tmp->trace_warp_inst);

    std::cout << "kernel_id: " << std::dec << tmp_inst_trace->kernel_id << std::endl;
    std::cout << "m_pc: " << std::hex << tmp_inst_trace->m_pc << std::endl;
    std::cout << "instn_str: " << tmp_inst_trace->instn_str << std::endl;
    
    std::cout << "m_opcode: " << std::dec << tmp_trace_warp_inst->get_opcode() << " OP_IMAD: " << OP_IMAD << std::endl;
    std::cout << "m_uid: " << std::dec << tmp_trace_warp_inst->get_uid() << std::endl;
    std::cout << "m_empty: " << std::dec << tmp_trace_warp_inst->isempty() << std::endl;
    std::cout << "m_isatomic: " << std::dec << tmp_trace_warp_inst->isatomic() << std::endl;
    std::cout << "m_decoded: " << std::dec << tmp_trace_warp_inst->isdecoded() << std::endl;
    std::cout << "pc: " << std::hex << tmp_trace_warp_inst->get_pc() << std::endl;
    std::cout << "isize: " << std::dec << tmp_trace_warp_inst->get_isize() << std::endl;

    std::cout << "num_operands: " << std::dec << tmp_trace_warp_inst->get_num_operands() << std::endl;
    std::cout << "num_regs: " << std::dec << tmp_trace_warp_inst->get_num_regs() << std::endl;
    std::cout << "outcount: " << std::dec << tmp_trace_warp_inst->get_outcount() << std::endl;
    std::cout << "incount: " << std::dec << tmp_trace_warp_inst->get_incount() << std::endl;
    std::cout << "is_vectorin: " << std::dec << tmp_trace_warp_inst->get_is_vectorin() << std::endl;
    std::cout << "is_vectorout: " << std::dec << tmp_trace_warp_inst->get_is_vectorout() << std::endl;

    for (unsigned i = 0; i < tmp_trace_warp_inst->get_incount(); i++) {
      std::cout << "arch_reg.src[" << i << "]: " << std::dec << tmp_trace_warp_inst->get_arch_reg_src(i) << std::endl;
    }
    for (unsigned i = 0; i < tmp_trace_warp_inst->get_outcount(); i++) {
      std::cout << "arch_reg.dst[" << i << "]: " << std::dec << tmp_trace_warp_inst->get_arch_reg_dst(i) << std::endl;
    }

    std::cout << "op: " << std::dec << tmp_trace_warp_inst->get_op() << " INTP_OP: " << INTP_OP << std::endl;
    std::cout << "sp_op: " << std::dec << tmp_trace_warp_inst->get_sp_op() << " INT__OP: " << INT__OP << std::endl;
    std::cout << "mem_op: " << std::dec << tmp_trace_warp_inst->get_mem_op() << " NOT_TEX: " << NOT_TEX << std::endl;
    std::cout << "const_cache_operand: " << std::dec << tmp_trace_warp_inst->get_const_cache_operand() << std::endl;
    std::cout << "oprnd_type: " << std::dec << tmp_trace_warp_inst->get_oprnd_type_() << " INT_OP: " << INT_OP << std::endl;
    std::cout << "should_do_atomic: " << std::dec << tmp_trace_warp_inst->get_should_do_atomic() << std::endl;

    std::cout << "gwarp_id: " << std::dec << tmp_trace_warp_inst->get_gwarp_id() << std::endl;
    std::cout << "warp_id: " << std::dec << tmp_trace_warp_inst->get_warp_id() << std::endl;
    std::cout << "active_mask: " << std::dec << tmp_trace_warp_inst->get_active_mask() << std::endl;
    
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
  }

  /**********************************************************************************************/
  /***                                                                                        ***/
  /***                              The computation simulation.                               ***/
  /***                                                                                        ***/
  /**********************************************************************************************/
  /**********************************************************************************************
  The following events that may cause stalls:
  1      Issue: ibuffer_empty
  2      Issue: waiting for barrier
  3      Issue: ibuffer_empty and also waiting for barrier
  4      Issue: control hazard
  5      Issue: m_mem_out has no free slot
  6      Issue: previous_issued_inst_exec_type is MEM
  7      Issue: m_int_out has no free slot
  8      Issue: previous_issued_inst_exec_type is INT
  9      Issue: m_sp_out has no free slot
  10     Issue: previous_issued_inst_exec_type is SP
  11     Issue: m_dp_out has no free slot
  12     Issue: previous_issued_inst_exec_type is DP
  13     Issue: m_sfu_out has no free slot
  14     Issue: previous_issued_inst_exec_type is SFU
  15     Issue: m_tensor_core_out has no free slot
  16     Issue: previous_issued_inst_exec_type is TENSOR
  17     Issue: m_spec_cores_out has no free slot
  18     Issue: previous_issued_inst_exec_type is SPECIALIZED
  19     Issue: scoreboard
  20     Fetch: read miss an insn from L1I
  21     Fetch: reservation fail an insn from L1I
  22     Execute: m_memport of L1D is not free
  23     Execute: m_dispatch_reg of fu\[\d+\]-\w+\s is not empty
  24     Execute: result_bus has no slot for latency-\d+
  25     Execute: dispatch delay of insn is \d+ > 0
  26     Execute: l1_latency_queue\[\d+\]\[\d+\] is not free
  27     Execute: COAL_STALL occurs
  28     Execute: mf_next->get_inst()'s out_reg\[R\d+\] has \d+ pending writes
  29     Execute: icnt_injection_buffer is full
  30     Execute: m_next_wb's out_reg\[R\d+\] has \d+ pending writes
  31     Execute: m_next_global of ldst unit is not free
  32     Execute: fill_port of L1D is not free
  33     Execute: m_pipeline_reg\[\d+\] is not empty
  34     Execute: m_dispatch_reg has pending writes
  35     Execute: bank-\d+ of reg-\d+ is not idle
  36     ReadOperands: bank\[\d+\] reg-\d+ \(order:\d+\) belonged to was allocated for write
  37     ReadOperands: bank\[\d+\] reg-\d+ \(order:\d+\) belonged to was allocated for other regs
  38     ReadOperands: port_num-\d+/m_in_ports\[\d+\].m_in\[\d+\] fails as not found free cu
  39     Writeback: bank-\d+ of reg-\d+ is not idle
  **********************************************************************************************/
  
  
  

STOP_AND_REPORT_TIMER_rank(world.rank(), 5);

  fflush(stdout);

STOP_AND_REPORT_TIMER_pass(-1, 0);

  return 0;
}
