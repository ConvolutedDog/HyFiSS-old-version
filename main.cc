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

/////////////////////////////////////////////
#include "./common/vector_types.h"
#include "./trace-driven/mem-access.h"
#include "./trace-driven/kernel-info.h"
#include "./trace-driven/trace-warp-inst.h"
#include "./ISA-Def/ampere_opcode.h"
#include "./ISA-Def/kepler_opcode.h"
#include "./ISA-Def/pascal_opcode.h"
#include "./ISA-Def/trace_opcode.h"
#include "./ISA-Def/turing_opcode.h"
#include "./ISA-Def/volta_opcode.h"
#include "./ISA-Def/accelwattch_component_mapping.h"
/////////////////////////////////////////////

#include "./common/CLI/CLI.hpp"
#include "./parda/parda.h"
#include "./hw-parser/hw-parser.h"

#include "../hw-component/PrivateSM.h"

#include <chrono>

#include <cmath>

float ceil(float x, float s) {
  return s * std::ceil(x / s);
}

float floor(float x, float s) {
  return s * std::floor(x / s);
}

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
/* SM_traces_all_passes[kid][sm_id_corresponding_to_current_rank]: 
 *     mem_instn0, mem_instn1, ...
 */
void private_L1_cache_stack_distance_evaluate_boost_no_concurrent(int argc, 
                                                                  char **argv, 
                                                                  std::vector<std::map<int, 
                                                                                       std::vector<mem_instn>
                                                                                      >
                                                                             >* SM_traces_all_passes,
                                                                  std::map<std::tuple<int, 
                                                                                      int, 
                                                                                      unsigned long long
                                                                                     >, 
                                                                           std::map<unsigned, bool>
                                                                          >* mem_instn_distance_overflow_flag,
                                                                  int _tmp_print_, 
                                                                  std::string configs_dir,
                                                                  bool dump_histogram,
                                                                  stat_collector* stat_coll,
                                                                  unsigned KERNEL_EVALUATION,
                                                                  std::vector<unsigned>* MEM_ACCESS_LATENCY) {
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  /* gpu_config[V100].num_sm is the number of the SMs that have been used during the execution. */
  const int pass_num = int((gpu_config[V100].num_sm + world.size() - 1)/world.size());

  // unsigned l1_cache_line_size = 32; // BUG: need configure
  // unsigned l1_cache_size = 96 * 1024; // BUG: need configure
  // unsigned l1_cache_associativity = 64; // BUG: need configure
  // unsigned l1_cache_blocks = l1_cache_size / l1_cache_line_size; // BUG: need configure
  // unsigned l2_cache_line_size = 64; // BUG: need configure
  // unsigned l2_cache_size = 96 * 1024; // BUG: need configure
  // unsigned l2_cache_associativity = 24; // BUG: need configure
  // unsigned l2_cache_blocks = l2_cache_size / l2_cache_line_size; // BUG: need configure
  
  unsigned l1_cache_line_size = 128; // BUG: need configure
  unsigned l1_cache_size = 32 * 1024; // BUG: need configure
  unsigned l1_cache_associativity = 64; // BUG: need configure
  unsigned l1_cache_blocks = l1_cache_size / l1_cache_line_size; // BUG: need configure
  unsigned l2_cache_line_size = 128; // BUG: need configure
  unsigned l2_cache_size = 96 * 1024 * 64; // BUG: need configure
  unsigned l2_cache_associativity = 24; // BUG: need configure
  unsigned l2_cache_blocks = l2_cache_size / l2_cache_line_size; // BUG: need configure

  std::cout << std::endl;

  float L1_hit_rate = 0.0;

  /* pass_num is the number of SMs that this current process should calculate. */
  for (int pass = 0; pass < pass_num; pass++) {
    int curr_process_idx_rank = world.rank() + pass * world.size();
    int curr_process_idx;
    if (curr_process_idx_rank < gpu_config[V100].num_sm) {
      curr_process_idx = curr_process_idx_rank;
    } else continue;

    /* curr_process_idx = curr_process_idx_rank, is the sm_id that the current loop should calculate. */

    /* HKEY input should be char* of addr */
    HKEY input;
    long tim;
    program_data_t pdt_c;
    program_data_t* pdt;
    FILE* file;
    std::string parda_histogram_filepath;

    /* SM_traces_all_passes[kid=0...kernels_num-1][0...num_sms_of_kis-1][] */
    for (unsigned kid = 0; kid < (*SM_traces_all_passes).size() ; kid++) {
      if ((unsigned)KERNEL_EVALUATION != kid) continue;

      unsigned miss_num_all_acc = 0;
      unsigned num_all_acc = 0;

      tim = 0;
      pdt_c = parda_init();
      /* Here we ONLY USE the curr_process_idx-th items of SM_traces_all_passes, so for every rank, 
       * we only load the curr_process_idx-th items for all kernels into SM_traces_all_passes, which 
       * significantly accelerate the simulation speed. */

      std::vector<std::vector<unsigned long long>> L1_miss_instns; // that need to read/write from/to L2

      unsigned LDG_requests = 0;
      unsigned LDG_transactions = 0;
      unsigned STG_requests = 0;
      unsigned STG_transactions = 0;

      unsigned Global_atomic_requests = 0;
      unsigned Global_reduction_requests = 0;
      unsigned Global_atomic_and_reduction_transactions = 0;

      unsigned L2_read_transactions = 0;
      unsigned L2_write_transactions = 0;
      unsigned L2_total_transactions = 0;
      

      for (auto mem_ins : (*SM_traces_all_passes)[kid][curr_process_idx]) { 
        /*
          SM_traces_all_passes:  kid=0  curr_process_idx=0  mem_instn0, 
                                                            mem_instn1, <---- mem_ins
                                                            ...
                                        curr_process_idx=1  mem_instn0, 
                                                            mem_instn1, 
                                                            ...
                                        curr_process_idx=2  mem_instn0, 
                                                            mem_instn1, 
                                                            ...
                                        ...
                                        curr_process_idx=79 mem_instn0, 
                                                            mem_instn1, 
                                                            ...
                                 kid=1  curr_process_idx=0  mem_instn0, 
                                                            mem_instn1, 
                                                            ...
                                        curr_process_idx=1  mem_instn0, 
                                                            mem_instn1, 
                                                            ...
                                        curr_process_idx=2  mem_instn0, 
                                                            mem_instn1, 
                                                            ...
                                        ...
                                        curr_process_idx=79 mem_instn0, 
                                                            mem_instn1, 
                                                            ...
        
        */
        // if (world.rank() == 0) {
        //   std::cout << "rank-" << std::dec << world.rank() << ", " << "SM-" << curr_process_idx << " " << "kid-" << kid << " ";
        //   std::cout << std::setw(18) << std::right << std::hex << mem_ins.pc << " ";
        //   std::cout << std::hex << mem_ins.time_stamp << " ";
        //   std::cout << std::hex << mem_ins.addr[0] << std::endl;
        // }
        std::map<unsigned, bool> distance_overflow_flag_vector;
        std::vector<unsigned long long> have_got_line_addr; // use have_got_line_addr will cause not accurate hit rate 

        // std::cout << mem_ins.has_mem_instn_type() << " " << mem_ins.opcode << std::endl;
        
        
        L1_miss_instns.push_back(std::vector<unsigned long long>());
        
        if (mem_ins.has_mem_instn_type() == LDG || mem_ins.has_mem_instn_type() == STG)

        for (unsigned j = 0; j < (mem_ins.addr).size(); j++) {
          unsigned long long cache_line_addr = mem_ins.addr[j] >> int(log2(l1_cache_line_size));
          /**/ // use have_got_line_addr will cause not accurate hit rate
          if(std::find(have_got_line_addr.begin(), 
                       have_got_line_addr.end(), 
                       cache_line_addr) != have_got_line_addr.end()) {
            mem_ins.distance[j] = 0;
            mem_ins.miss[j] = false;
            
            
            distance_overflow_flag_vector[mem_ins.addr[j]] = false;
            // num_all_acc++;
          } else {
          /**/ // use have_got_line_addr will cause not accurate hit rate 
            sprintf(input, "0x%llx", cache_line_addr);
            /* If you only want to dump the histogram of each SM for every kernel, you can also use: 
             *     process_one_access(input, &pdt_c, tim); */
            mem_ins.distance[j] = process_one_access_and_get_distance(input, &pdt_c, tim);
            if (curr_process_idx == 0 && kid == 0) if ((cache_line_addr & 3) == 0);
              // std::cout << "###SM-" << curr_process_idx << " " 
              //                       << std::hex << (mem_ins.addr[j]) << " " 
              //                       << std::hex << (mem_ins.addr[j] >> 7) << " " 
              //                       << std::hex << (mem_ins.addr[j] >> 5) << " " 
              //                       << input << " " 
              //                       << (cache_line_addr & 3) << std::dec << std::endl;
            if (mem_ins.distance[j] > (int)l1_cache_blocks) {
              // std::cout << mem_ins.distance[j] << " " ;
              miss_num_all_acc++;
              mem_ins.miss[j] = true;
              L1_miss_instns.back().push_back(mem_ins.addr[j]);
              // L1_miss_instns.push_back(mem_ins.addr[j]);
              /* Here we have got that if a mem instn will hit or miss L1 cache, and now we 
               * should add intns to the L2 cache, and should pass hit or miss L1 cache to 
               * the compute model. */
              /* The reason  */

              distance_overflow_flag_vector[mem_ins.addr[j]] = true;

              // L2 read trans
              if (mem_ins.has_mem_instn_type() == LDG) {
                L2_read_transactions += 1;
                L2_total_transactions += 1;
              }
              if (mem_ins.has_mem_instn_type() == STG) {
                L2_write_transactions += 1;
                L2_total_transactions += 1;
              }
            } else {
              L1_miss_instns.back().push_back(mem_ins.addr[j]);
              distance_overflow_flag_vector[mem_ins.addr[j]] = false;
            }

            num_all_acc++;

            
            mem_instn_distance_overflow_flag->insert(std::make_pair(std::make_tuple(kid, curr_process_idx, mem_ins.pc), 
                                                      distance_overflow_flag_vector));
            
            
            tim++;
            have_got_line_addr.push_back(cache_line_addr); // use have_got_line_addr will cause not accurate hit rate 
          /**/ // use have_got_line_addr will cause not accurate hit rate 
          }
          /**/ // use have_got_line_addr will cause not accurate hit rate 
        }

        if (mem_ins.has_mem_instn_type() == LDG) {
          LDG_requests++;
          LDG_transactions += have_got_line_addr.size();
        }
        if (mem_ins.has_mem_instn_type() == STG) {
          STG_requests++;
          STG_transactions += have_got_line_addr.size();
        }
        if (mem_ins.has_mem_instn_type() == ATOM) {
          Global_atomic_requests++;
          Global_atomic_and_reduction_transactions += have_got_line_addr.size();
        }
        if (mem_ins.has_mem_instn_type() == RED) {
          Global_reduction_requests++;
          Global_atomic_and_reduction_transactions += have_got_line_addr.size();
        }
      }

      
      stat_coll->set_L2_read_transactions(L2_read_transactions, curr_process_idx);
      stat_coll->set_L2_write_transactions(L2_write_transactions, curr_process_idx);
      stat_coll->set_L2_total_transactions(L2_total_transactions, curr_process_idx);


      stat_coll->set_GEMM_read_requests(LDG_requests, curr_process_idx);
      stat_coll->set_GEMM_write_requests(STG_requests, curr_process_idx);
      stat_coll->set_GEMM_total_requests(LDG_requests + STG_requests, curr_process_idx);
      stat_coll->set_GEMM_read_transactions(LDG_transactions, curr_process_idx);
      stat_coll->set_GEMM_write_transactions(STG_transactions, curr_process_idx);
      stat_coll->set_GEMM_total_transactions(LDG_transactions + STG_transactions, curr_process_idx);
      stat_coll->set_Number_of_read_transactions_per_read_requests((float)((float)LDG_transactions / (float)LDG_requests), curr_process_idx);
      stat_coll->set_Number_of_write_transactions_per_write_requests((float)((float)STG_transactions / (float)STG_requests), curr_process_idx);

      stat_coll->set_Total_number_of_global_atomic_requests(Global_atomic_requests, curr_process_idx);
      stat_coll->set_Total_number_of_global_reduction_requests(Global_reduction_requests, curr_process_idx);
      stat_coll->set_Global_memory_atomic_and_reduction_transactions(Global_atomic_and_reduction_transactions, curr_process_idx);

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
          L1_hit_rate = parda_fprintf_histogram_r(pdt->histogram, file, false);
          fclose(file);
        } else {
          L1_hit_rate = parda_fprintf_histogram_r(pdt->histogram, NULL, false);
        }
      } else {
        L1_hit_rate = parda_fprintf_histogram_r(pdt->histogram, NULL, false);
      }
      
      // std::cout << "L1_hit_rate: " << L1_hit_rate << std::endl;
      // std::cout << "@@@: " << curr_process_idx << " " << (float)(((float)num_all_acc-(float)miss_num_all_acc)/(float)num_all_acc) << std::endl;
      stat_coll->set_Unified_L1_cache_hit_rate(L1_hit_rate, curr_process_idx);
      stat_coll->set_Unified_L1_cache_requests(num_all_acc, curr_process_idx);
      // std::cout << "rank: " << world.rank() << " sm_id: " << curr_process_idx 
      //           << " num_all_acc: " << num_all_acc << " miss_num_all_acc: " 
      //           << miss_num_all_acc << " L1_hit_rate: " << L1_hit_rate << std::endl;
      
      parda_free(pdt);


      /*  LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL  */
      // tim = 0;
      // pdt_c = parda_init();

      // unsigned l2_miss_num_all_acc = 0;
      // unsigned l2_num_all_acc = 0;

      // for (auto mem_ins : (*SM_traces_all_passes)[kid][curr_process_idx]) {
      //   std::vector<unsigned long long> have_got_line_addr;
      //   std::vector<unsigned long long> L1_have_got_line_addr;
      //   for (unsigned j = 0; j < (mem_ins.addr).size(); j++) {
      //     unsigned long long cache_line_addr = mem_ins.addr[j] >> int(log2(l2_cache_line_size));
      //     if(std::find(have_got_line_addr.begin(), 
      //                  have_got_line_addr.end(), 
      //                  cache_line_addr) != have_got_line_addr.end()) {
      //       ;
      //     } else {
      //       sprintf(input, "0x%llx", cache_line_addr);
      //       mem_ins.distance[j] = process_one_access_and_get_distance(input, &pdt_c, tim);
      //       if (mem_ins.distance[j] > (int)l2_cache_blocks) {
      //         l2_miss_num_all_acc++;
      //       }
      //       l2_num_all_acc++;
      //       tim++;
      //       // have_got_line_addr.push_back(cache_line_addr);
      //     }
      //   }
      // }
      
      // std::cout << "L2_hit_rate: " << l2_num_all_acc << " " << l2_miss_num_all_acc << " ";
      // std::cout << "@@@: " << curr_process_idx 
      //           << " " << (float)(((float)l2_num_all_acc-(float)l2_miss_num_all_acc)/(float)l2_num_all_acc) 
      //           << std::endl;
      /*  LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL  */
    }
  }

  float L2_hit_rate = 0.0;
  // std::cout << "Process " << world.rank() << " got L2_hit_rate = " << L2_hit_rate << std::endl;

  // HERE DO L2 CACHE HIT RATE EVALUATION
  if (world.rank() == 0) {
    unsigned DRAM_total_transactions = 0;

    bool index_instn_go_on = true;
    for (unsigned kid = 0; kid < (*SM_traces_all_passes).size() ; kid++) {
      if ((unsigned)KERNEL_EVALUATION != kid) continue;
      unsigned max_instn_size = 0;
      for (unsigned sm_id = 0; sm_id < gpu_config[V100].num_sm; sm_id++) {
        if ((*SM_traces_all_passes)[kid][sm_id].size() > max_instn_size) {
          max_instn_size = (*SM_traces_all_passes)[kid][sm_id].size();
        }
      }

      HKEY input;
      long tim;
      program_data_t pdt_c;

      tim = 0;
      pdt_c = parda_init();

      unsigned l2_miss_num_all_acc = 0;
      unsigned l2_num_all_acc = 0;

      for (unsigned instn_index = 0; instn_index < max_instn_size; instn_index++) {
        for (unsigned sm_id = 0; sm_id < gpu_config[V100].num_sm; sm_id++) {
          // std::cout << "sm_id: " << sm_id << " " << (*SM_traces_all_passes)[kid][sm_id].size() << std::endl;
          
          if (instn_index < (*SM_traces_all_passes)[kid][sm_id].size()) {
            auto mem_ins = (*SM_traces_all_passes)[kid][sm_id][instn_index];

            if (!((*SM_traces_all_passes)[kid][sm_id][instn_index].has_mem_instn_type() == LDG || 
                (*SM_traces_all_passes)[kid][sm_id][instn_index].has_mem_instn_type() == STG)) continue;

            std::vector<unsigned long long> have_got_line_addr;
            std::vector<unsigned long long> L1_have_got_line_addr;

            for (unsigned j = 0; j < (mem_ins.addr).size(); j++) {
              
              unsigned long long L1_cache_line_addr = mem_ins.addr[j] >> int(log2(l1_cache_line_size));
              unsigned long long cache_line_addr = mem_ins.addr[j] >> int(log2(l2_cache_line_size));

              // std::cout << std::hex << mem_ins.addr[j] << " ";
              
              // if(std::find(have_got_line_addr.begin(), 
              //              have_got_line_addr.end(), 
              //              cache_line_addr) != have_got_line_addr.end()) {
              //   ;
              // } else {
              //   sprintf(input, "0x%llx", cache_line_addr);
              //   mem_ins.distance_L2[j] = process_one_access_and_get_distance(input, &pdt_c, tim);
              //   if (mem_ins.distance_L2[j] > (int)l2_cache_blocks) {
              //     l2_miss_num_all_acc++;
              //   }
              //   l2_num_all_acc++;
              //   tim++;
              //   have_got_line_addr.push_back(cache_line_addr);
              // }

              if(std::find(L1_have_got_line_addr.begin(), 
                           L1_have_got_line_addr.end(), 
                           L1_cache_line_addr) != L1_have_got_line_addr.end()) {
                ;
              } else {
                if(std::find(have_got_line_addr.begin(), 
                             have_got_line_addr.end(), 
                             cache_line_addr) != have_got_line_addr.end()) {
                  l2_num_all_acc++;
                } else {
                  sprintf(input, "0x%llx", cache_line_addr);
                  mem_ins.distance_L2[j] = process_one_access_and_get_distance(input, &pdt_c, tim);
                  if (mem_ins.distance_L2[j] > (int)l2_cache_blocks) {
                    l2_miss_num_all_acc++;
                    if (mem_ins.has_mem_instn_type() == LDG) {
                      DRAM_total_transactions++;
                    }
                  }
                  l2_num_all_acc++;
                  tim++;
                  have_got_line_addr.push_back(cache_line_addr);
                }
                L1_have_got_line_addr.push_back(L1_cache_line_addr);
              }
            }
          }
        }
      }

      

      std::cout << "L2_hit_rate: " << l2_num_all_acc << " " << l2_miss_num_all_acc << " " 
                << (float)(((float)l2_num_all_acc-(float)l2_miss_num_all_acc)/(float)l2_num_all_acc) << std::endl;

      L2_hit_rate = (float)(((float)l2_num_all_acc-(float)l2_miss_num_all_acc)/(float)l2_num_all_acc);

      
      program_data_t* pdt = &pdt_c;
      pdt->histogram[B_INF] += narray_get_len(pdt->ga);
      L2_hit_rate = parda_fprintf_histogram_r(pdt->histogram, NULL, false);

      stat_coll->set_L2_cache_hit_rate(L2_hit_rate);
      stat_coll->set_L2_cache_requests(l2_num_all_acc);

      stat_coll->set_DRAM_total_transactions(DRAM_total_transactions);
      
      parda_free(pdt);
    }
  }

  boost::mpi::broadcast(world, L2_hit_rate, 0);

  world.barrier();

  // Memory Units Latencies TODO: for Volta
  unsigned dram_mem_access = 302;
  unsigned l1_cache_access = 33;
  unsigned l2_cache_access = 213;

  unsigned l1_cache_access_latency = l1_cache_access;
  unsigned l2_cache_access_latency = l2_cache_access;
  unsigned l2_cache_from_l1_access_latency = l2_cache_access_latency - l1_cache_access_latency;
  unsigned dram_mem_access_latency = dram_mem_access;
  unsigned l2_cache_from_dram_access_latency = 
    dram_mem_access_latency - l2_cache_access_latency - l1_cache_access_latency;

  // unsigned l1_cycles_no_contention = stat_coll->get_GEMM_total_transactions(0) * l1_cache_access_latency;
  
  

  for (int pass = 0; pass < pass_num; pass++) {
    int curr_process_idx_rank = world.rank() + pass * world.size();
    int curr_process_idx;
    if (curr_process_idx_rank < gpu_config[V100].num_sm) {
      curr_process_idx = curr_process_idx_rank;
    } else continue;

    float _L1_hit_rate = stat_coll->get_Unified_L1_cache_hit_rate(curr_process_idx);

    (*MEM_ACCESS_LATENCY)[curr_process_idx] = _L1_hit_rate * l1_cache_access_latency + 
                                           (1 - _L1_hit_rate) * (
                                             L2_hit_rate * l2_cache_from_l1_access_latency + 
                                             (1 - L2_hit_rate) * l2_cache_from_dram_access_latency);

    std::cout << "MEM_ACCESS_LATENCY[" << curr_process_idx << "]: " << (*MEM_ACCESS_LATENCY)[curr_process_idx] << std::endl; 

  }


  // std::cout << "Process " << world.rank() << " got L2_hit_rate = " << L2_hit_rate << std::endl;

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

  unsigned KERNEL_EVALUATION = 0;

  std::string hw_config_file = "./DEV-Def/QV100.config";

  app.add_option("--configs", configs, "The configs path, which is generated from our NVBit tool, "
                                       "e.g., \"./traces/vectoradd/configs\"");
  app.add_option("--sort", sort, "Simulate the order in which instructions are issued based on their "
                                 "timestamps");
  app.add_option("--log", PRINT_LOG, "Print the traces processing log or not");
  app.add_option("--dump_histogram", dump_histogram, "Dump the histogram of the private L1 cache hit "
                                                     "rate");
  app.add_option("--clog", PRINT_COMPUTE_LOG, "Print the computation traces processing log or not");
  app.add_option("--config_file", hw_config_file, "The config file, e.g., \"../DEV-Def/QV100.config\"");
  app.add_option("--kernel_id", KERNEL_EVALUATION, "The kernel id that you want to simulate");

  int _tmp_print_;

  app.add_option("--tmp", _tmp_print_, "tmp");

  CLI11_PARSE(app, argc, argv);

  int passnum_concurrent_issue_to_sm = 1;

  hw_config hw_cfg(hw_config_file);

  trace_parser tracer(configs.c_str(), &hw_cfg);

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

  stat_collector stat_coll(gpu_config[V100].num_sm, KERNEL_EVALUATION);

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

  // std::cout << "@@@ 000" << std::endl;

  app_config* appcfg = tracer.get_appcfg();
  assert(appcfg->get_kernel_block_size((int)KERNEL_EVALUATION) <= stat_coll.get_max_block_size());
  assert(appcfg->get_kernel_num_registers((int)KERNEL_EVALUATION) <= stat_coll.get_max_registers_per_thread());
  assert(appcfg->get_kernel_shared_mem_bytes((int)KERNEL_EVALUATION) <= stat_coll.get_shared_mem_size());

  stat_coll.set_total_num_workloads(appcfg->get_kernel_grid_dim_x((int)KERNEL_EVALUATION) * 
                                    appcfg->get_kernel_grid_dim_y((int)KERNEL_EVALUATION) * 
                                    appcfg->get_kernel_grid_dim_z((int)KERNEL_EVALUATION));
  // std::cout << "@@@ 111" << std::endl;
  stat_coll.set_active_SMs(std::min(stat_coll.get_m_num_sm(), stat_coll.get_active_SMs()));
  
  // std::cout << appcfg->get_kernel_block_size((int)KERNEL_EVALUATION) << std::endl;
  // std::cout << stat_coll.get_warp_size() << std::endl;

  stat_coll.set_allocated_active_warps_per_block(
    (unsigned)(ceil(appcfg->get_kernel_block_size((int)KERNEL_EVALUATION) / stat_coll.get_warp_size(), 1)) );

  if (stat_coll.get_allocated_active_warps_per_block() == 0)
    stat_coll.set_allocated_active_warps_per_block(1);

  // std::cout << "@@@ 222" << std::endl;

  // std::cout << stat_coll.get_max_active_blocks_per_SM() << std::endl;
  // std::cout << stat_coll.get_max_active_threads_per_SM() << std::endl;
  // std::cout << stat_coll.get_warp_size() << std::endl;
  // std::cout << stat_coll.get_allocated_active_warps_per_block() << std::endl;

  stat_coll.set_Thread_block_limit_warps( std::min( stat_coll.get_max_active_blocks_per_SM(), 
                                                    (unsigned)floor(stat_coll.get_max_active_threads_per_SM()/
                                                     stat_coll.get_warp_size()/stat_coll.get_allocated_active_warps_per_block(),
                                                     1) ) );
  // std::cout << "@@@ 333" << std::endl;
  if (appcfg->get_kernel_num_registers((int)KERNEL_EVALUATION) == 0) {
    stat_coll.set_Thread_block_limit_registers(stat_coll.get_max_active_blocks_per_SM());
  } else {
    // std::cout << "get_kernel_num_registers: " << appcfg->get_kernel_num_registers((int)KERNEL_EVALUATION) << std::endl;
    // std::cout << "stat_coll.get_warp_size(): " << stat_coll.get_warp_size() << std::endl;
    // std::cout << "stat_coll.get_register_allocation_size(): " << stat_coll.get_register_allocation_size() << std::endl;
    unsigned allocated_regs_per_warp = (unsigned)( ceil(appcfg->get_kernel_num_registers((int)KERNEL_EVALUATION) * 
                                                    stat_coll.get_warp_size(), 
                                                    stat_coll.get_register_allocation_size()) );
    // std::cout << "allocated_regs_per_warp: " << allocated_regs_per_warp << std::endl;

    // std::cout << "stat_coll.get_max_registers_per_block(): " << stat_coll.get_max_registers_per_block() << std::endl;
    // std::cout << "hw_cfg.get_num_sched_per_sm(): " << hw_cfg.get_num_sched_per_sm() << std::endl;

    unsigned allocated_regs_per_SM = (unsigned)( floor(stat_coll.get_max_registers_per_block() / allocated_regs_per_warp, 
                                                 hw_cfg.get_num_sched_per_sm()) );
    // std::cout << "allocated_regs_per_SM: " << allocated_regs_per_SM << std::endl;

    // std::cout << "stat_coll.get_allocated_active_warps_per_block(): " << stat_coll.get_allocated_active_warps_per_block() << std::endl;
    // std::cout << "stat_coll.get_max_registers_per_SM(): " << stat_coll.get_max_registers_per_SM() << std::endl;
    // std::cout << "stat_coll.get_max_registers_per_block(): " << stat_coll.get_max_registers_per_block() << std::endl;
    stat_coll.set_Thread_block_limit_registers( floor(allocated_regs_per_SM / stat_coll.get_allocated_active_warps_per_block(), 1) * 
                                                floor(stat_coll.get_max_registers_per_SM() / stat_coll.get_max_registers_per_block(), 1) );
  }
  
  // std::cout << "@@@ 333" << std::endl;
  
  if (appcfg->get_kernel_shared_mem_bytes((int)KERNEL_EVALUATION) == 0) {
    // std::cout << "stat_coll.get_max_active_blocks_per_SM(): " << stat_coll.get_max_active_blocks_per_SM() << std::endl;
    stat_coll.set_Thread_block_limit_shared_memory(stat_coll.get_max_active_blocks_per_SM());
  } else {
    // std::cout << appcfg->get_kernel_shared_mem_bytes((int)KERNEL_EVALUATION) << " " << stat_coll.get_smem_allocation_size() << std::endl;
    float smem_per_block = ceil(appcfg->get_kernel_shared_mem_bytes((int)KERNEL_EVALUATION), stat_coll.get_smem_allocation_size());
    // std::cout << smem_per_block << std::endl;

    stat_coll.set_Thread_block_limit_shared_memory(floor(stat_coll.get_shared_mem_size()/smem_per_block, 1));
    // std::cout << stat_coll.get_shared_mem_size() << " " << stat_coll.get_Thread_block_limit_shared_memory() << std::endl;
  }

  // std::cout << "#@#@# " << stat_coll.get_Thread_block_limit_warps() << " " 
  //           << stat_coll.get_Thread_block_limit_registers() << " " << stat_coll.get_Thread_block_limit_shared_memory() << std::endl;
  
  stat_coll.set_allocated_active_blocks_per_SM( std::min(std::min(stat_coll.get_Thread_block_limit_warps(),
                                                                  stat_coll.get_Thread_block_limit_registers()),
                                                         stat_coll.get_Thread_block_limit_shared_memory()) );
  unsigned th_active_blocks = stat_coll.get_allocated_active_blocks_per_SM();
  stat_coll.set_Theoretical_max_active_warps_per_SM(th_active_blocks * stat_coll.get_allocated_active_warps_per_block());
  // std::cout << "#Theoretical_max_active_warps_per_SM: " << stat_coll.get_Theoretical_max_active_warps_per_SM() << std::endl;
  // std::cout << "#max_active_threads_per_SM: " << stat_coll.get_max_active_threads_per_SM() << std::endl;
  // std::cout << (float)stat_coll.get_Theoretical_max_active_warps_per_SM() / 
  //                                                     (float)(stat_coll.get_max_active_threads_per_SM() / 
  //                                                      stat_coll.get_warp_size()) * 100. << std::endl;
  stat_coll.set_Theoretical_occupancy((unsigned)(ceil((float)stat_coll.get_Theoretical_max_active_warps_per_SM() / 
                                                      (float)(stat_coll.get_max_active_threads_per_SM() / 
                                                       stat_coll.get_warp_size()) * 100., 1)));
  
  



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

  // std::cout << "@@@ 777" << std::endl;

START_TIMER(3);

    /* When gpgpu_concurrent_kernel_sm is configured to false, 
     * passnum_concurrent_issue_to_sm is just the number of kernels.
     * And the SM_traces_all_passes's size if also the number of kernels,
     * pass is the kernel id.
     */
    /* 算法：
     *   对于所有的rank，每个rank都会遍历所有的kernel，找到所有的kernel的kernel_info_t对象，
     *   存储在single_pass_kernels_info中，以下是对应于单个kernel:{
     *     对于kernel-kid，遍历其所有的thread block，找到当前rank所持有的thread block(这要
     *     通过block的sm_id判断)，并将其加入到
     *     SM_traces_all_passes[kid][sm_id_corresponding_to_current_rank]中，
     *     SM_traces_all_passes[kid][sm_id_corresponding_to_current_rank]: 
     *     mem_instn0, mem_instn1, ...
     *   }
     */
    for (int pass = 0; pass < passnum_concurrent_issue_to_sm; pass++) {
      if (pass != (int)KERNEL_EVALUATION) continue;

      // pass is kernel_id
      if (PRINT_LOG) std::cout << "Schedule pass: " << pass << std::endl;

// START_TIMER(2);

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
      
      // when gpgpu_concurrent_kernel_sm = 0, start_kernel_id = start_kernel_id = pass = kernel_id

      /* Here we traversal all the kernels that should be executed during this pass, to create their kernel-info 
       * object. And then the kernels that belong to the same SM will to be used to evaluate L1D cache. */

      // when gpgpu_concurrent_kernel_sm = 0, kid = start_kernel_id = end_kernel_id = pass
      for (int kid = start_kernel_id; kid <= std::min(end_kernel_id, tracer.get_appcfg()->get_kernels_num() - 1); kid++) {
        kernel_trace_t * kernel_trace_info = tracer.parse_kernel_info(kid, PRINT_LOG);
        trace_kernel_info_t *kernel_info = create_kernel_info(kernel_trace_info, &tracer);
        single_pass_kernels_info.push_back(kernel_info);
      }

      // single_pass_kernels_info has only one item, and it is the kernel_info_t object of the 
      // start_kernel_id = start_kernel_id = pass = kernel_id -th kernel.

      if (PRINT_LOG) std::cout << "    Kernel nums waiting for processing: " << single_pass_kernels_info.size() << std::endl;

      /* Now we have got the kernel-info object of all the kernels that should be executed during this pass,
       * what we should do now is traversal these kernels, and find out all the thread blocks belong to the 
       * same SM, with the usage of issue.config. The SM_traces stores all the memory traces that belong to
       * the same SM. */
      std::map<int, std::vector<mem_instn>>* SM_traces = &SM_traces_all_passes[pass];

      /* pass_num is the SMs that the current rank should process. */
      const int pass_num = int((gpu_config[V100].num_sm + world.size() - 1)/world.size());
      /* _pass is the index of SMs that the current rank should process. 
       * If we have 4 ranks, and 8 SMs:
       *          _pass-0  1
       *   rank-0 => SM-0, 4, 
       *   rank-1 => SM-1, 5, <-- curr_process_idx
       *   rank-2 => SM-2, 6,
       *   rank-3 => SM-3, 7.
       */

      // pass_num is the total number of SMs that are calculated by the current rank.
      // _pass is the index of SMs that are calculated by the current rank.
      for (int _pass = 0; _pass < /*pass_num*/1; _pass++) {
        /* curr_process_idx_rank is the SM id that should be processed. */
        int curr_process_idx_rank = world.rank() + _pass * world.size();
        /* curr_process_idx is the SM id that should be processed. */
        int curr_process_idx = curr_process_idx_rank;
        if (curr_process_idx < gpu_config[V100].num_sm)
        for (auto k : single_pass_kernels_info) {
          // k is only single_pass_kernels_info[0]

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
            // if (sm_id == curr_process_idx) { // yangjianchao16 20240306
              /* The threadblock_traces[i] stores the memory traces that belong to k->get_trace_info()->kernel_id 
               * and thread block i. */
              threadblock_traces[i] = k->get_one_kernel_one_threadblock_traces(k->get_trace_info()->kernel_id - 1, i);

              // SM_traces[sm_id].insert(SM_traces[sm_id].end(), threadblock_traces[i].begin(), threadblock_traces[i].end()); // old
              (*SM_traces)[sm_id].insert((*SM_traces)[sm_id].end(), threadblock_traces[i].begin(), threadblock_traces[i].end());
              // std::cout << "sm_id-" << sm_id << " " << threadblock_traces[i].size() << std::endl;
            // } // yangjianchao16 20240306
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

// STOP_AND_REPORT_TIMER_pass(pass, 2);

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
  /* SM_traces_all_passes[kid=0...kernels_num-1][0...num_sms_of_kis-1][] 
   * Note that the second dim of SM_traces_all_passes is the SM id, and if the kernels.
   * 
   * We also need a map to store the distance of each memory instruction, the key is the
   * <k_id, sm_id, pc>, and the value is the distance vector of 1~32 addresses in the 
   * instruction. */
  std::map<std::tuple<int, int, unsigned long long>, std::map<unsigned, bool>> mem_instn_distance_overflow_flag;

  // std::cout << "@@@ 888" << std::endl;

  std::vector<unsigned> MEM_ACCESS_LATENCY;
  MEM_ACCESS_LATENCY.resize(80);

  auto start_memory_timer = std::chrono::system_clock::now();
  private_L1_cache_stack_distance_evaluate_boost_no_concurrent(argc, 
                                                               argv, 
                                                               &SM_traces_all_passes, 
                                                               &mem_instn_distance_overflow_flag, 
                                                               _tmp_print_, 
                                                               configs, 
                                                               dump_histogram,
                                                               &stat_coll,
                                                               KERNEL_EVALUATION,
                                                               &MEM_ACCESS_LATENCY);
  auto end_memory_timer = std::chrono::system_clock::now();
  auto duration_memory_timer = 
    std::chrono::duration_cast<std::chrono::microseconds>(end_memory_timer - start_memory_timer);
  auto cost_memory_timer = (double)(double(duration_memory_timer.count()) * 
    (double)(std::chrono::microseconds::period::num) / (double)(std::chrono::microseconds::period::den));
  stat_coll.set_Simulation_time_memory_model(cost_memory_timer, world.rank());

  // std::cout << "@@@ 999" << std::endl;

  stat_coll.print_Unified_L1_cache_hit_rate();

  
  // abort();
  

  // the 0-th rank should merge results from multiple ranks
  if (world.rank() == 0) {
    


  }

  /* Print mem_instn_distance_overflow_flag. */
  /*
  std::cout << "mem_instn_distance_overflow_flag START: " << std::endl;
  for (auto iter : mem_instn_distance_overflow_flag) {
    std::cout << "kernel_id: " << std::dec << std::get<0>(iter.first) 
              << " sm_id: " << std::dec << std::get<1>(iter.first) 
              << " pc: " << std::hex << std::get<2>(iter.first) << "";
    for (auto iter2 : iter.second) {
      std::cout << "    address: " << std::hex << iter2.first 
                << " miss: " << std::dec << iter2.second << std::endl;
    }
  }
  std::cout << "mem_instn_distance_overflow_flag END." << std::endl;
  */

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
    if (_DEBUG_LOG_)
      std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;

    // compute_instn* tmp = tracer.get_one_kernel_one_warp_one_instn(42, 2, 3);
    // _inst_trace_t* tmp_inst_trace = tmp->inst_trace;
    // trace_warp_inst_t* tmp_trace_warp_inst = &(tmp->trace_warp_inst);

    // std::cout << "kernel_id: " << std::dec << tmp_inst_trace->kernel_id << std::endl;
    // std::cout << "m_pc: " << std::hex << tmp_inst_trace->m_pc << std::endl;
    // std::cout << "instn_str: " << tmp_inst_trace->instn_str << std::endl;
    
    // std::cout << "m_opcode: " << std::dec << tmp_trace_warp_inst->get_opcode() << " OP_IMAD: " << OP_IMAD << std::endl;
    // std::cout << "m_uid: " << std::dec << tmp_trace_warp_inst->get_uid() << std::endl;
    // std::cout << "m_empty: " << std::dec << tmp_trace_warp_inst->isempty() << std::endl;
    // std::cout << "m_isatomic: " << std::dec << tmp_trace_warp_inst->isatomic() << std::endl;
    // std::cout << "m_decoded: " << std::dec << tmp_trace_warp_inst->isdecoded() << std::endl;
    // std::cout << "pc: " << std::hex << tmp_trace_warp_inst->get_pc() << std::endl;
    // std::cout << "isize: " << std::dec << tmp_trace_warp_inst->get_isize() << std::endl;

    // std::cout << "num_operands: " << std::dec << tmp_trace_warp_inst->get_num_operands() << std::endl;
    // std::cout << "num_regs: " << std::dec << tmp_trace_warp_inst->get_num_regs() << std::endl;
    // std::cout << "outcount: " << std::dec << tmp_trace_warp_inst->get_outcount() << std::endl;
    // std::cout << "incount: " << std::dec << tmp_trace_warp_inst->get_incount() << std::endl;
    // std::cout << "is_vectorin: " << std::dec << tmp_trace_warp_inst->get_is_vectorin() << std::endl;
    // std::cout << "is_vectorout: " << std::dec << tmp_trace_warp_inst->get_is_vectorout() << std::endl;

    // for (unsigned i = 0; i < tmp_trace_warp_inst->get_incount(); i++) {
    //   std::cout << "arch_reg.src[" << i << "]: " << std::dec << tmp_trace_warp_inst->get_arch_reg_src(i) << std::endl;
    // }
    // for (unsigned i = 0; i < tmp_trace_warp_inst->get_outcount(); i++) {
    //   std::cout << "arch_reg.dst[" << i << "]: " << std::dec << tmp_trace_warp_inst->get_arch_reg_dst(i) << std::endl;
    // }

    // std::cout << "op: " << std::dec << tmp_trace_warp_inst->get_op() << " INTP_OP: " << INTP_OP << std::endl;
    // std::cout << "sp_op: " << std::dec << tmp_trace_warp_inst->get_sp_op() << " INT__OP: " << INT__OP << std::endl;
    // std::cout << "mem_op: " << std::dec << tmp_trace_warp_inst->get_mem_op() << " NOT_TEX: " << NOT_TEX << std::endl;
    // std::cout << "const_cache_operand: " << std::dec << tmp_trace_warp_inst->get_const_cache_operand() << std::endl;
    // std::cout << "oprnd_type: " << std::dec << tmp_trace_warp_inst->get_oprnd_type_() << " INT_OP: " << INT_OP << std::endl;
    // std::cout << "should_do_atomic: " << std::dec << tmp_trace_warp_inst->get_should_do_atomic() << std::endl;

    // std::cout << "gwarp_id: " << std::dec << tmp_trace_warp_inst->get_gwarp_id() << std::endl;
    // std::cout << "warp_id: " << std::dec << tmp_trace_warp_inst->get_warp_id() << std::endl;
    // std::cout << "active_mask: " << std::dec << tmp_trace_warp_inst->get_active_mask() << std::endl;
    
    if (_DEBUG_LOG_)
      std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
  }

STOP_AND_REPORT_TIMER_rank(world.rank(), 5);

START_TIMER(6);
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

  auto start_compute_timer = std::chrono::system_clock::now();
  

  for (int _pass = 0; _pass < pass_num; _pass++) {
    int curr_process_idx_rank = world.rank() + _pass * world.size();
    /* curr_process_idx is the SM id that should be processed */
    unsigned smid = curr_process_idx_rank;
    // std::cout << "### Rank-" << world.rank() << ", processing SM-" << smid << std::endl;
    // if (smid < gpu_config[V100].num_sm) {
    
    if (smid == 0) {
      std::cout << "### Rank-" << world.rank() 
                << ", processing SM-" << smid << std::endl;
      PrivateSM private_sm = PrivateSM(smid, &tracer, &hw_cfg);
      if (_DEBUG_LOG_)
        std::cout << "private_sm.get_cycle(): " 
                  << private_sm.get_cycle() << std::endl;
      // traverse blocks_per_kernel
      for (auto pair : *(private_sm.get_blocks_per_kernel())) {
        unsigned kid = pair.first;
        std::vector<unsigned> block_ids = pair.second;
        if (_DEBUG_LOG_)
          for (unsigned block_id : block_ids) {
            std::cout << "kid: " << kid << ", block_id: " << block_id << std::endl;
          }
      }
      std::cout << " ...run START... " << std::endl;

      std::cout << "@@@@ " << MEM_ACCESS_LATENCY[smid] << std::endl;
      while (private_sm.get_active()) {
        private_sm.run(KERNEL_EVALUATION, MEM_ACCESS_LATENCY[smid]);
      }
      
      
      std::cout << "private_sm.get_active_cycles(): " << private_sm.get_active_cycles() << std::endl;
      //active_warps_id_size_sum
      std::cout << "private_sm.get_active_warps_id_size_sum(): " << private_sm.get_active_warps_id_size_sum() << std::endl;

      /*
      unsigned get_active_warps() { return m_active_warps; }
      unsigned get_max_warps_init() { return max_warps_init; }
      */
      float achieved_occupancy = (float)private_sm.get_active_warps_id_size_sum() / 
                                 (float)private_sm.get_active_cycles() / 
                                 (float)private_sm.get_max_warps_init();
      stat_coll.set_Achieved_occupancy(achieved_occupancy, smid);
      // std::cout << "achieved_occupancy: " << achieved_occupancy << std::endl;
      float achieved_active_warps_per_SM = achieved_occupancy * (float)stat_coll.get_Theoretical_max_active_warps_per_SM();
      // std::cout << "achieved_active_warps_per_SM: " << achieved_active_warps_per_SM << std::endl;
      stat_coll.set_Achieved_active_warps_per_SM(achieved_active_warps_per_SM, smid);


      std::cout << " ...run EXIT... " << std::endl;
      stat_coll.set_GPU_active_cycles(private_sm.get_cycle(), smid);
      stat_coll.set_SM_active_cycles(private_sm.get_cycle(), smid);
      stat_coll.set_Warp_instructions_executed(private_sm.get_num_warp_instns_executed(), smid);
      stat_coll.set_Instructions_executed_per_clock_cycle_IPC(
        (float)private_sm.get_num_warp_instns_executed() / (float)private_sm.get_cycle(), smid);
      stat_coll.set_Total_instructions_executed_per_seconds( // MIPS
        (float)( (float)private_sm.get_num_warp_instns_executed() / 1e6 ) / 
        (float)( (float)private_sm.get_cycle() / (float)hw_cfg.get_core_clock_mhz() ), smid);
      stat_coll.set_Kernel_execution_time( // ns
        (float)((float)private_sm.get_cycle() / (float)hw_cfg.get_core_clock_mhz() * 1e9), smid);
    }
  }

  auto end_compute_timer = std::chrono::system_clock::now();
  auto duration_compute_timer = 
    std::chrono::duration_cast<std::chrono::microseconds>(end_compute_timer - start_compute_timer);
  auto cost_compute_timer = (double)(double(duration_compute_timer.count()) * 
    (double)(std::chrono::microseconds::period::num) / (double)(std::chrono::microseconds::period::den));
  // std::cout << "Cost " << "-" << cost_compute_timer << " seconds." << std::endl;
  stat_coll.set_Simulation_time_compute_model(cost_compute_timer, world.rank());


  stat_coll.dump_output(configs, world.rank());

STOP_AND_REPORT_TIMER_rank(world.rank(), 6);

  fflush(stdout);

STOP_AND_REPORT_TIMER_pass(-1, 0);

  return 0;
}


/*
make clean && make -j && mpirun -np 1 ./gpu-simulator.x --configs ./traces/hotspot/configs/ --sort 0 --log 0 --dump_histogram 0 --clog 0 --config_file DEV-Def/QV100.config --tmp 0 --kernel_id 0 > tmp.txt

vs: tmp_tune.bak.txt/tmp_tune.txt  hotspot
    tmp.txt
*/