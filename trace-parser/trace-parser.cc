// developed by Mahmoud Khairy, Purdue Univ
// abdallm@purdue.edu

#include <bits/stdc++.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "trace-parser.h"

std::map<unsigned, sass_inst_t> pc_to_sassStr;
std::vector<int> have_readed_insn_pcs;

bool have_print_sass_during_this_execution = false;


sass_inst_t find_sass_inst_by_pc(unsigned pc) {
  std::map<unsigned, sass_inst_t>::iterator iter;
  iter = pc_to_sassStr.find(pc);
  if (iter != pc_to_sassStr.end()) {
    return iter->second;
  } else {
    std::cout << "Can't find sass inst by pc: " << std::hex << pc << std::endl;
    sass_inst_t null_ = sass_inst_t();
    return null_;
  }
}

void inst_memadd_info_t::base_stride_decompress(unsigned long long base_address, 
                                                int stride,
                                                const std::bitset<WARP_SIZE> &mask) {
  bool first_bit1_found = false;
  bool last_bit1_found = false;
  unsigned long long addra = base_address;
  for (int s = 0; s < WARP_SIZE; s++) {
    if (mask.test(s) && !first_bit1_found) {
      first_bit1_found = true;
      addrs[s] = base_address;
    } else if (first_bit1_found && !last_bit1_found) {
      if (mask.test(s)) {
        addra += stride;
        addrs[s] = addra;
      } else
        last_bit1_found = true;
    } else
      addrs[s] = 0;
  }
  empty = false;
}

void inst_memadd_info_t::base_delta_decompress(
    unsigned long long base_address, const std::vector<long long> &deltas,
    const std::bitset<WARP_SIZE> &mask) {
  bool first_bit1_found = false;
  long long last_address = 0;
  unsigned delta_index = 0;
  for (int s = 0; s < 32; s++) {
    if (mask.test(s) && !first_bit1_found) {
      addrs[s] = base_address;
      first_bit1_found = true;
      last_address = base_address;
    } else if (mask.test(s) && first_bit1_found) {
      assert(delta_index < deltas.size());
      addrs[s] = last_address + deltas[delta_index++];
      last_address = addrs[s];
    } else
      addrs[s] = 0;
  }
  empty = false;
}

inst_trace_t::inst_trace_t() { memadd_info = NULL; }

inst_trace_t::~inst_trace_t() {
  if (memadd_info != NULL) delete memadd_info;
}

inst_trace_t::inst_trace_t(const inst_trace_t &b) {
  if (memadd_info != NULL) {
    memadd_info = new inst_memadd_info_t();
    memadd_info = b.memadd_info;
  }
}

bool inst_trace_t::check_opcode_contain(const std::vector<std::string> &opcode,
                                        std::string param) const {
  for (unsigned i = 0; i < opcode.size(); ++i)
    if (opcode[i] == param) return true;

  return false;
}

std::vector<std::string> inst_trace_t::get_opcode_tokens() const {
  std::istringstream iss(opcode);
  std::vector<std::string> opcode_tokens;
  std::string token;
  while (std::getline(iss, token, '.')) {
    if (!token.empty()) opcode_tokens.push_back(token);
  }
  return opcode_tokens;
}

bool is_number(const std::string &s) {
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

unsigned inst_trace_t::get_datawidth_from_opcode(
    const std::vector<std::string> &opcode) const {
  for (unsigned i = 0; i < opcode.size(); ++i) {
    if (is_number(opcode[i])) {
      return (std::stoi(opcode[i], NULL) / 8);
    } else if (opcode[i][0] == 'U' && is_number(opcode[i].substr(1))) {
      // handle the U* case
      unsigned bits;
      sscanf(opcode[i].c_str(), "U%u", &bits);
      return bits / 8;
    }
  }

  return 4;  // default is 4 bytes
}

std::string intToHex(unsigned num) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(4) << std::hex << num;
  return stream.str();
}

int get_line_num_by_pc(const std::string filename, const std::string targetNumber) {
    std::ifstream file(filename);
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
      lineNumber++;
      size_t firstSpace = line.find(" ");
      if (firstSpace != std::string::npos) {
          size_t secondSpace = line.find(" ", firstSpace + 1);
          if (secondSpace != std::string::npos) {
              if (line.substr(firstSpace + 1, secondSpace - firstSpace - 1) == targetNumber) {
                file.close();
                return lineNumber;
              }
          }
      }
    }

    file.close();
    return -1;
}

bool inst_trace_t::parse_from_string(std::string trace,
                                     unsigned trace_version,
                                     unsigned enable_lineinfo,
                                     std::string kernel_name,
                                     unsigned kernel_id) {
  std::stringstream ss;
  ss.str(trace);

  std::string temp;

  // Start Parsing

  if (trace_version < 3) {
    // for older trace version, read the tb ids and ignore
    unsigned threadblock_x = 0, threadblock_y = 0, threadblock_z = 0,
             warpid_tb = 0;

    ss >> std::dec >> threadblock_x >> threadblock_y >> threadblock_z >>
        warpid_tb;
  }
  if (enable_lineinfo) {
    ss >> std::dec >> line_num;
  }

  ss >> std::hex >> m_pc;
  
  ss >> std::hex >> mask;

  std::bitset<WARP_SIZE> mask_bits(mask);

  ss >> std::dec >> reg_dsts_num;
  assert(reg_dsts_num <= MAX_DST);
  for (unsigned i = 0; i < reg_dsts_num; ++i) {
    ss >> temp;
    sscanf(temp.c_str(), "R%d", &reg_dest[i]);
  }

  ss >> opcode;

  ss >> reg_srcs_num;
  assert(reg_srcs_num <= MAX_SRC);
  for (unsigned i = 0; i < reg_srcs_num; ++i) {
    ss >> temp;
    sscanf(temp.c_str(), "R%d", &reg_src[i]);
  }

  if (!count(have_readed_insn_pcs.begin(), have_readed_insn_pcs.end(), m_pc)) {
    pc_to_sassStr[m_pc] = sass_inst_t();
    pc_to_sassStr[m_pc].insnStr = trace;
    pc_to_sassStr[m_pc].kernel_name = kernel_name;
    pc_to_sassStr[m_pc].kernel_id = kernel_id;
    pc_to_sassStr[m_pc].line_num = line_num;
    pc_to_sassStr[m_pc].m_pc = m_pc;
    pc_to_sassStr[m_pc].mask = mask;
    pc_to_sassStr[m_pc].reg_dsts_num = reg_dsts_num;
    pc_to_sassStr[m_pc].opcode = opcode;
    pc_to_sassStr[m_pc].reg_srcs_num = reg_srcs_num;

    pc_to_sassStr[m_pc].m_empty = false;

    for (unsigned i = 0; i < reg_dsts_num; ++i) {
      pc_to_sassStr[m_pc].reg_dest[i] = reg_dest[i];
    }
    for (unsigned i = 0; i < reg_srcs_num; ++i) {
      pc_to_sassStr[m_pc].reg_src[i] = reg_src[i];
    }
    
    pc_to_sassStr[m_pc].m_source_file = std::string("./traces/kernel-") + 
                                        std::to_string(kernel_id) + std::string(".traceg");
    pc_to_sassStr[m_pc].m_source_line = get_line_num_by_pc(pc_to_sassStr[m_pc].m_source_file, intToHex(m_pc));
    have_readed_insn_pcs.push_back(m_pc);
  }

  // parse mem info
  unsigned address_mode = 0;
  unsigned mem_width = 0;

  ss >> mem_width;

  if (mem_width > 0)  // then it is a memory inst
  {
    memadd_info = new inst_memadd_info_t();

    // read the memory width from the opcode, as nvbit can report it incorrectly
    std::vector<std::string> opcode_tokens = get_opcode_tokens();
    memadd_info->width = get_datawidth_from_opcode(opcode_tokens);

    ss >> std::dec >> address_mode;
    if (address_mode == address_format::list_all) {
      // read addresses one by one from the file
      for (int s = 0; s < WARP_SIZE; s++) {
        if (mask_bits.test(s))
          ss >> std::hex >> memadd_info->addrs[s];
        else
          memadd_info->addrs[s] = 0;
      }
    } else if (address_mode == address_format::base_stride) {
      // read addresses as base address and stride
      unsigned long long base_address = 0;
      int stride = 0;
      ss >> std::hex >> base_address;
      ss >> std::dec >> stride;
      memadd_info->base_stride_decompress(base_address, stride, mask_bits);
    } else if (address_mode == address_format::base_delta) {
      unsigned long long base_address = 0;
      std::vector<long long> deltas;
      // read addresses as base address and deltas
      ss >> std::hex >> base_address;
      for (int s = 0; s < WARP_SIZE; s++) {
        if (mask_bits.test(s)) {
          long long delta = 0;
          ss >> std::dec >> delta;
          deltas.push_back(delta);
        }
      }
      memadd_info->base_delta_decompress(base_address, deltas, mask_bits);
    }
  } else { 
    memadd_info = new inst_memadd_info_t();
    memadd_info->empty = true;
    memadd_info->width = -1;
  }
  // Finish Parsing

  return true;
}

kernel_trace_t::kernel_trace_t() {
  kernel_name = "Empty";
  shmem_base_addr = 0;
  local_base_addr = 0;
  binary_verion = 0;
  trace_verion = 0;
}

void app_config::init(std::string config_path, bool PRINT_LOG) {
    // named app config options (unordered)
    // option_parser_t app_config_opp = option_parser_create();
    
    // option_parser_register(
    //   app_config_opp, "-app_kernels_id", OPT_CSTR, &app_kernels_id_string,
    //   "kernels ID list (default=1)", "1");
    
    std::stringstream ss;

    // read the "-app_kernels_id" config in the app config file
    // and register the kernel config options
    std::ifstream inputFile;
    inputFile.open(config_path);
    if (!inputFile.good()) {
      fprintf(stderr, "\n\nOptionParser ** ERROR: Cannot open config file '%s'\n",
              config_path.c_str());
      exit(1);
    }

    std::string target = "-app_kernels_id";
    
    std::string line;
    size_t commentStart;
    size_t found;
    std::string result;
    int comma_count;
    while (inputFile.good()) {
      getline(inputFile, line);
      commentStart = line.find_first_of("#");
      if (commentStart != line.npos) continue;
      found = line.find(target);
      if (found != std::string::npos) {
        result = line.substr(found + target.length());
        comma_count = std::count(result.begin(), result.end(), ',');
        kernels_num = comma_count + 1;
        // std::cout << ">>>" << result << "<<<" << std::endl;
        app_kernels_id_string = line.substr(found + target.length() + 1);
      }
    }
    inputFile.close();

    kernel_name.resize(kernels_num);
    kernel_num_registers.resize(kernels_num);
    kernel_shared_mem_bytes.resize(kernels_num);
    kernel_grid_size.resize(kernels_num);
    kernel_block_size.resize(kernels_num);
    kernel_cuda_stream_id.resize(kernels_num);

    kernel_grid_dim_x.resize(kernels_num);
    kernel_grid_dim_y.resize(kernels_num);
    kernel_grid_dim_z.resize(kernels_num);
    kernel_tb_dim_x.resize(kernels_num);
    kernel_tb_dim_y.resize(kernels_num);
    kernel_tb_dim_z.resize(kernels_num);
    kernel_shmem_base_addr.resize(kernels_num);
    kernel_local_base_addr.resize(kernels_num);
    
    /*************************************************************/
    inputFile.open(config_path);
    for (int j = 0; j < kernels_num; ++j) {
      while (inputFile.good()) {
        getline(inputFile, line);
        commentStart = line.find_first_of("#");
        if (commentStart != line.npos) continue;

        std::stringstream ss;

        ss.str("");
        ss << "-kernel_" << j + 1 << "_kernel_name";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_name[j] = line.substr(found + ss.str().length());
          // std::cout << "kernel_name[" << j << "]: " << kernel_name[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_num_registers";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_num_registers[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_num_registers[" << j << "]: " << kernel_num_registers[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_shared_mem_bytes";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_shared_mem_bytes[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_shared_mem_bytes[" << j << "]: " << kernel_shared_mem_bytes[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_grid_size";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_grid_size[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_grid_size[" << j << "]: " << kernel_grid_size[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_block_size";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_block_size[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_block_size[" << j << "]: " << kernel_block_size[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_cuda_stream_id";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_cuda_stream_id[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_cuda_stream_id[" << j << "]: " << kernel_cuda_stream_id[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_grid_dim_x";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_grid_dim_x[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_grid_dim_x[" << j << "]: " << kernel_grid_dim_x[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_grid_dim_y";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_grid_dim_y[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_grid_dim_y[" << j << "]: " << kernel_grid_dim_y[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_grid_dim_z";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_grid_dim_z[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_grid_dim_z[" << j << "]: " << kernel_grid_dim_z[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_tb_dim_x";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_tb_dim_x[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_tb_dim_x[" << j << "]: " << kernel_tb_dim_x[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_tb_dim_y";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_tb_dim_y[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_tb_dim_y[" << j << "]: " << kernel_tb_dim_y[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_tb_dim_z";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_tb_dim_z[j] = std::stoi(line.substr(found + ss.str().length()));
          // std::cout << "kernel_tb_dim_z[" << j << "]: " << kernel_tb_dim_z[j] << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_shmem_base_addr";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_shmem_base_addr[j] = std::stoull(line.substr(found + ss.str().length()), 0, 16);
          // std::cout << "kernel_shmem_base_addr[" << j << "]: " << std::hex << kernel_shmem_base_addr[j] << std::dec << std::endl;
        }

        ss.str("");
        ss << "-kernel_" << j + 1 << "_local_base_addr";
        found = line.find(ss.str());
        if (found != std::string::npos) {
          kernel_local_base_addr[j] = std::stoull(line.substr(found + ss.str().length()), 0, 16);
          // std::cout << "kernel_local_base_addr[" << j << "]: " << std::hex << kernel_local_base_addr[j] << std::dec << std::endl;
        }
      }
      inputFile.clear();
      inputFile.seekg(0, std::ios::beg);
    }
    /*************************************************************/

    // for (int j = 0; j < kernels_num; ++j) {
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_kernel_name";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_CSTR,
    //                          &kernel_name[j],
    //                          "kernel name",
    //                          "None");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_num_registers";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_num_registers[j],
    //                          "number of registers used by kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_shared_mem_bytes";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_shared_mem_bytes[j],
    //                          "number of bytes of shared memory used by kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_grid_size";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_grid_size[j],
    //                          "grid size of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_block_size";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_block_size[j],
    //                          "block size of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_cuda_stream_id";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_cuda_stream_id[j],
    //                          "cuda stream id of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_grid_dim_x";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_grid_dim_x[j],
    //                          "grid dim x of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_grid_dim_y";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_grid_dim_y[j],
    //                          "grid dim y of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_grid_dim_z";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_grid_dim_z[j],
    //                          "grid dim z of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_tb_dim_x";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_tb_dim_x[j],
    //                          "tb dim x of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_tb_dim_y";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_tb_dim_y[j],
    //                          "tb dim y of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_tb_dim_z";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_INT32,
    //                          &kernel_tb_dim_z[j],
    //                          "tb dim z of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_shmem_base_addr";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_UINT64,
    //                          &kernel_shmem_base_addr[j],
    //                          "shmem base addr of kernel",
    //                          "0");
    //   ss.str("");
    //   ss << "-kernel_" << j + 1 << "_local_base_addr";
    //   option_parser_register(app_config_opp, ss.str().c_str(), OPT_UINT64,
    //                          &kernel_local_base_addr[j],
    //                          "local base addr of kernel",
    //                          "0");
    //   ss.str("");
    // }

    // option_parser_cfgfile(app_config_opp, config_path.c_str());

    char *toks = new char[2048];
    char *tokd = toks;
    strcpy(toks, app_kernels_id_string.c_str());

    app_kernels_id.resize(kernels_num);

    toks = strtok(toks, ",");

    for (int i = 0; i < kernels_num; i++) {
      // std::cout << "=> " << " " << toks << std::endl;
      assert(toks);
      int ntok = sscanf(toks, "%d", &app_kernels_id[i]);
      assert(ntok == 1);
      toks = strtok(NULL, ",");
    }

    delete[] tokd;
    delete[] toks;

    if (PRINT_LOG) fprintf(stdout, ">>> APP config Options <<<:\n");
    // if (PRINT_LOG) option_parser_print(app_config_opp, stdout); //BUG: del this will cause fault
    // option_parser_destroy(app_config_opp);

    m_valid = true;
}

void instn_config::init(std::string config_path, bool PRINT_LOG) {
  std::ifstream inputFile;
  // open config file, stream every line into a continuous buffer
  // get rid of comments in the process
  inputFile.open(config_path);
  if (!inputFile.good()) {
    fprintf(stderr, "\n\nOptionParser ** ERROR: Cannot open config file '%s'\n",
            config_path.c_str());
    exit(1);
  }
  
  size_t first_blank_pos; // before it => kernel_id
  size_t second_blank_pos; // before it => pc, after it => instn_str
  unsigned kernel_id, pc;
  std::string kernel_id_str, pc_str, instn_str;

  while (inputFile.good()) {
    std::string line;
    getline(inputFile, line);
    size_t commentStart = line.find_first_of("#");
    if (commentStart != line.npos) continue;
    if (!line.empty()) {
      first_blank_pos = line.find(' ');
      second_blank_pos = line.find(' ', first_blank_pos + 1);
      kernel_id_str = line.substr(0, first_blank_pos);
      pc_str = line.substr(first_blank_pos + 1, second_blank_pos - first_blank_pos - 1);
      
      std::istringstream iss_1(kernel_id_str);
      std::istringstream iss_2(pc_str);

      iss_1 >> kernel_id;
      iss_2 >> std::hex >> pc;

      instn_str = line.substr(second_blank_pos + 1);

      // std::cout << "|||>>>" << line << "<<<|||" << std::endl;
      // std::cout << "kernel_id: " << std::hex << kernel_id << std::dec << std::endl;
      // std::cout << "pc: " << std::hex << pc << std::dec << std::endl;
      // std::cout << "instn_str: " << instn_str << std::endl << std::endl;

      instn_info_t instn_info;
      instn_info.kernel_id = kernel_id;
      instn_info.pc = pc;
      instn_info.instn_str = instn_str;

      instn_info_vector.push_back(instn_info);
    }
  }
  inputFile.close();

  if (PRINT_LOG) fprintf(stdout, ">>> INSTN config Options <<<:\n");
  // traversal instn_info_vector
  for (unsigned i = 0; i < instn_info_vector.size() && PRINT_LOG; ++i) {
    std::cout << "-instn " << std::setw(5) << std::right 
              << std::dec << instn_info_vector[i].kernel_id << std::dec;
    std::cout << " " << std::setw(8) << std::left 
              << std::hex << instn_info_vector[i].pc << std::dec;
    std::cout << " " << std::setw(55) << std::left 
              << instn_info_vector[i].instn_str << std::endl;
  }
}

int issue_config::get_sm_id_of_one_block(unsigned kernel_id, unsigned block_id) {
  // std::cout << "kernel_id: " << kernel_id << " block_id: " << block_id << std::endl;
  for (unsigned i = 0; i < trace_issued_sm_id_blocks.size(); i++) {
    for (unsigned j = 0; j < trace_issued_sm_id_blocks[i].size(); j++) {
      // std::cout << kernel_id << ", ";
      // std::cout << block_id << " | ";
      // std::cout << trace_issued_sm_id_blocks[i][j].kernel_id << ", "
      //           << trace_issued_sm_id_blocks[i][j].block_id << std::endl;

      if (trace_issued_sm_id_blocks[i][j].kernel_id == kernel_id && 
          trace_issued_sm_id_blocks[i][j].block_id == block_id) {
        return trace_issued_sm_id_blocks[i][j].sm_id;
      }
    }
  }
  return -1;
}

std::vector<block_info_t> issue_config::parse_blocks_info(const std::string& blocks_info_str) {
    std::vector<block_info_t> result;
    size_t start = 0;
    size_t end = blocks_info_str.find(',', start);
    int total_tuples = std::stoi(blocks_info_str.substr(start, end - start));

    start = end + 1;
    end = blocks_info_str.find(',', end + 1);
    int sm_id = std::stoi(blocks_info_str.substr(start, end - start));

    // std::cout << "  total_tuples: " << total_tuples << std::endl;
    // std::cout << "  sm_id: " << sm_id << std::endl;
    
    /* template: -trace_issued_sm_id_0 1,0,(1,0,bfbd36210c36), */
    for (int i = 0; i < total_tuples; ++i) {
        /* template: 4,x,(1,80,a7140c84716c),(1,0,a7140c847156),(2,80,a7142bc9ecc1),(2,0,a7142bc9ecc5), */
        start = end + 1;
        end = blocks_info_str.find('(', start);
        size_t comma = blocks_info_str.find(',', end);
        
        unsigned kernel_id = std::stoi(blocks_info_str.substr(end + 1, comma - end - 1));
        // std::cout << "       kernel_id: " << kernel_id << std::endl;
        end = blocks_info_str.find(',', comma+1);
        
        unsigned block_id = std::stoi(blocks_info_str.substr(comma + 1, end - comma - 1));
        // std::cout << "       block_id: " << block_id << std::endl;
        comma = blocks_info_str.find(')', end+1);
        
        unsigned long long time_stamp = std::stoull(blocks_info_str.substr(end + 1, comma - end - 1), 0, 16);
        // std::cout << "       time_stamp: " << std::hex << time_stamp << std::dec << std::endl;
        end = comma + 1;

        block_info_t info = block_info_t(kernel_id, block_id, time_stamp, sm_id);
        // std::cout << info.kernel_id << " " << info.block_id << " " << std::hex << info.time_stamp << std::dec << " " << info.sm_id << std::endl;
        result.push_back(info);
    }

    return result;
}

void issue_config::init(std::string config_path, bool PRINT_LOG) {
    // named issue config options (unordered)
    // option_parser_t issue_config_opp = option_parser_create();

    std::stringstream ss;

    /****************************************************************/
    // read the "-app_kernels_id" config in the app config file
    // and register the kernel config options
    std::ifstream inputFile;
    inputFile.open(config_path);
    if (!inputFile.good()) {
      fprintf(stderr, "\n\nOptionParser ** ERROR: Cannot open config file '%s'\n",
              config_path.c_str());
      exit(1);
    }

    std::string target1 = "-trace_issued_sms_num";
    std::string target2 = "-trace_issued_sms_vector";
    std::string line;
    size_t commentStart;
    size_t found1, found2;
    std::string result1, result2;
    while (inputFile.good()) {
      getline(inputFile, line);
      commentStart = line.find_first_of("#");
      if (commentStart != line.npos) continue;
      found1 = line.find(target1);
      found2 = line.find(target2);
      if (found1 != std::string::npos) {
        result1 = line.substr(found1 + target1.length() + 1);
        // std::cout << ">>>" << std::stoi(result) << "<<<" << std::endl;
        trace_issued_sms_num = std::stoi(result1);
      }
      if (found2 != std::string::npos) {
        result2 = line.substr(found2 + target2.length() + 1);
        // std::cout << ">>>" << result << "<<<" << std::endl;
        std::istringstream iss(result2);
        std::string token;
        while (std::getline(iss, token, ',')) {
          trace_issued_sms_vector.push_back(std::stoi(token));
          // std::cout << "trace_issued_sms_vector: " << std::stoi(token) << std::endl;
        }
      }
    }
    inputFile.close();
    /****************************************************************/
    trace_issued_sm_id_blocks_str.resize(trace_issued_sms_num);

    // int tmp1;
    // std::string tmp2;
    // option_parser_register(issue_config_opp, "-trace_issued_sms_num", OPT_INT32,
    //                        &tmp1, "number of SMs that have been issued blocks (default=1)", "1");
    // option_parser_register(issue_config_opp, "-trace_issued_sms_vector", OPT_CSTR,
    //                        &tmp2, "vector of SMs id that have been issued blocks (default=None)", "");
    
    inputFile.open(config_path);
    for (int j = 0; j < trace_issued_sms_num; ++j) {
      int sm_num = trace_issued_sms_vector[j];

      ss.str("");
      ss << "-trace_issued_sm_id_" << std::to_string(sm_num);
      // option_parser_register(issue_config_opp, ss.str().c_str(), OPT_CSTR,
      //                        &trace_issued_sm_id_blocks_str[j],
      //                        "issued blocks list on i-th SM (default=None)", "None");

      while (inputFile.good()) {
        getline(inputFile, line);
        commentStart = line.find_first_of("#");
        if (commentStart != line.npos) continue;

        size_t found = line.find(ss.str());
        if (found != std::string::npos) {
          trace_issued_sm_id_blocks_str[j] = line.substr(found + ss.str().length() + 1);
        }
      }

      inputFile.clear();
      inputFile.seekg(0, std::ios::beg);
    }

    // option_parser_cfgfile(issue_config_opp, config_path.c_str());
    
    if (PRINT_LOG) fprintf(stdout, ">>> ISSUE config Options <<<:\n");
    // if (PRINT_LOG) option_parser_print(issue_config_opp, stdout); //BUG: del this will cause fault
    // option_parser_destroy(issue_config_opp);

    /****************************************************************/
    /* parse the issued blocks list on each SM */
    std::string blocks_info_str;
    trace_issued_sm_id_blocks.resize(trace_issued_sms_num);
    for (int j = 0; j < trace_issued_sms_num; ++j) {
      // std::cout << "trace_issued_sm_id_blocks_str[" << j << "]: " 
      //           << trace_issued_sm_id_blocks_str[j].c_str() << std::endl;
      blocks_info_str = trace_issued_sm_id_blocks_str[j].c_str();
      std::vector<block_info_t> result = parse_blocks_info(blocks_info_str);
      trace_issued_sm_id_blocks[j] = result;
    }
    
    // for (int j = 0; j < trace_issued_sms_num; ++j) {
    //   std::cout << "@@@ trace_issued_sm_id_blocks[" << j << "]: " << std::endl;
    //   for (unsigned k = 0; k < trace_issued_sm_id_blocks[j].size(); ++k) {
    //     std::cout << "  kernel_id: " << trace_issued_sm_id_blocks[j][k].kernel_id << " | ";
    //     std::cout << "  block_id: " << trace_issued_sm_id_blocks[j][k].block_id << std::endl;
    //   }
    // }
    /****************************************************************/
    m_valid = true;
}

kernel_trace_t* trace_parser::parse_kernel_info(int kernel_id, bool PRINT_LOG) {
  kernel_trace_t *kernel_info = new kernel_trace_t;
  
  kernel_info->kernel_name = get_appcfg()->get_kernel_name(kernel_id).c_str();

  if (PRINT_LOG) std::cout << "    Creating kernel info for kernel: " << kernel_info->kernel_name
                           << " (kernel_id: " + std::to_string(kernel_id) + ")" << std::endl;

  kernel_info->kernel_id = (unsigned)get_appcfg()->get_app_kernel_id(kernel_id);
  kernel_info->grid_dim_x = (unsigned)get_appcfg()->get_kernel_grid_dim_x(kernel_id);
  kernel_info->grid_dim_y = (unsigned)get_appcfg()->get_kernel_grid_dim_y(kernel_id);
  kernel_info->grid_dim_z = (unsigned)get_appcfg()->get_kernel_grid_dim_z(kernel_id);
  kernel_info->tb_dim_x = (unsigned)get_appcfg()->get_kernel_tb_dim_x(kernel_id);
  kernel_info->tb_dim_y = (unsigned)get_appcfg()->get_kernel_tb_dim_y(kernel_id);
  kernel_info->tb_dim_z = (unsigned)get_appcfg()->get_kernel_tb_dim_z(kernel_id);
  kernel_info->shmem = (unsigned)get_appcfg()->get_kernel_shmem_base_addr(kernel_id);
  kernel_info->nregs = (unsigned)get_appcfg()->get_kernel_num_registers(kernel_id);
  kernel_info->cuda_stream_id = (unsigned)get_appcfg()->get_kernel_cuda_stream_id(kernel_id);
  /* default disabled */
  kernel_info->binary_verion = VOLTA_BINART_VERSION;
  /* default disabled */
  kernel_info->enable_lineinfo = 0;
  /* default disabled */
  kernel_info->nvbit_verion = "1.5.0";
  kernel_info->trace_verion = 0;
  kernel_info->shmem_base_addr = get_appcfg()->get_kernel_shmem_base_addr(kernel_id);
  kernel_info->local_base_addr = get_appcfg()->get_kernel_local_base_addr(kernel_id);

  // do not close the file ifs, the kernel_finalizer will close it
  return kernel_info;
}

kernel_trace_t* trace_parser::parse_kernel_info(const std::string &kerneltraces_filepath) {
  kernel_trace_t *kernel_info = new kernel_trace_t;
  kernel_info->enable_lineinfo = 0; // default disabled
  kernel_info->ifs = new std::ifstream;
  std::ifstream *ifs = kernel_info->ifs;
  ifs->open(kerneltraces_filepath.c_str());

  if (!ifs->is_open()) {
    std::cout << "Unable to open file: " << kerneltraces_filepath << std::endl;
    exit(1);
  }

  std::cout << "Processing kernel " << kerneltraces_filepath << std::endl;

  std::string line;

  while (!ifs->eof()) {
    getline(*ifs, line);

    if (line.length() == 0) {
      continue;
    } else if (line[0] == '#') {
      // the trace format, ignore this and assume fixed format for now
      break;  // or the begin of the instruction stream
    } else if (line[0] == '-') {
      std::stringstream ss;
      std::string string1, string2;

      ss.str(line);
      ss.ignore();
      ss >> string1 >> string2;

      if (string1 == "kernel" && string2 == "name") {
        const size_t equal_idx = line.find('=');
        kernel_info->kernel_name = line.substr(equal_idx + 2);
      } else if (string1 == "kernel" && string2 == "id") {
        sscanf(line.c_str(), "-kernel id = %d", &kernel_info->kernel_id);
      } else if (string1 == "grid" && string2 == "dim") {
        sscanf(line.c_str(), "-grid dim = (%d,%d,%d)", &kernel_info->grid_dim_x,
               &kernel_info->grid_dim_y, &kernel_info->grid_dim_z);
      } else if (string1 == "block" && string2 == "dim") {
        sscanf(line.c_str(), "-block dim = (%d,%d,%d)", &kernel_info->tb_dim_x,
               &kernel_info->tb_dim_y, &kernel_info->tb_dim_z);
      } else if (string1 == "shmem" && string2 == "=") {
        sscanf(line.c_str(), "-shmem = %d", &kernel_info->shmem);
      } else if (string1 == "nregs") {
        sscanf(line.c_str(), "-nregs = %d", &kernel_info->nregs);
      } else if (string1 == "cuda" && string2 == "stream") {
        sscanf(line.c_str(), "-cuda stream id = %lu",
               &kernel_info->cuda_stream_id);
      } else if (string1 == "binary" && string2 == "version") {
        sscanf(line.c_str(), "-binary version = %d",
               &kernel_info->binary_verion);
      } else if (string1 == "enable" && string2 == "lineinfo") {
        sscanf(line.c_str(), "-enable lineinfo = %d",
               &kernel_info->enable_lineinfo);
      } else if (string1 == "nvbit" && string2 == "version") {
        const size_t equal_idx = line.find('=');
        kernel_info->nvbit_verion = line.substr(equal_idx + 1);

      } else if (string1 == "accelsim" && string2 == "tracer") {
        sscanf(line.c_str(), "-accelsim tracer version = %d",
               &kernel_info->trace_verion);

      } else if (string1 == "shmem" && string2 == "base_addr") {
        const size_t equal_idx = line.find('=');
        ss.str(line.substr(equal_idx + 1));
        ss >> std::hex >> kernel_info->shmem_base_addr;

      } else if (string1 == "local" && string2 == "mem") {
        const size_t equal_idx = line.find('=');
        ss.str(line.substr(equal_idx + 1));
        ss >> std::hex >> kernel_info->local_base_addr;
      }
      std::cout << "    Info: " << line << std::endl;
      continue;
    }
  }

  // do not close the file ifs, the kernel_finalizer will close it
  return kernel_info;
}

void trace_parser::kernel_finalizer(kernel_trace_t *trace_info) {
  assert(trace_info);
  assert(trace_info->ifs);
  if (trace_info->ifs->is_open()) trace_info->ifs->close();
  delete trace_info->ifs;
  delete trace_info;
}

trace_parser::trace_parser(const char *input_configs_filepath) {
  configs_filepath = input_configs_filepath;
}

#include <unistd.h>
#include <limits.h>

void trace_parser::process_configs_file(std::string config_path, int config_type, bool PRINT_LOG) {
  std::ifstream fs;
  
  char cwd[PATH_MAX];
  assert(getcwd(cwd, sizeof(cwd)) != nullptr);
  std::string current_directory(cwd);

  std::string abs_config_path = current_directory + "/" + config_path;
  fs.open(abs_config_path);

  if (PRINT_LOG) std::cout << std::endl;
  std::cout << "Processing configs file : " << abs_config_path << std::endl;
  if (PRINT_LOG) std::cout << std::endl;
  
  if (!fs.is_open()) {
    std::cout << "Unable to open file: " << abs_config_path << std::endl;
    exit(1);
  }
  fs.close();

  if (config_type == APP_CONFIG) {
    appcfg = app_config();
    appcfg.init(abs_config_path, PRINT_LOG);
  } else if (config_type == INSTN_CONFIG) {
    instncfg = instn_config();
    instncfg.init(abs_config_path, PRINT_LOG);
  } else if (config_type == ISSUE_CONFIG) {
    issuecfg = issue_config();
    issuecfg.init(abs_config_path, PRINT_LOG);
  }
}

void trace_parser::parse_configs_file(bool PRINT_LOG) {
  
  if (configs_filepath.back() == '/') {
    app_config_path = configs_filepath + "app.config";
    instn_config_path = configs_filepath + "instn.config";
    issue_config_path = configs_filepath + "issue.config";
    
  } else {
    app_config_path = configs_filepath + "/" + "app.config";
    instn_config_path = configs_filepath + "/" + "instn.config";
    issue_config_path = configs_filepath + "/" + "issue.config";
  }

  /* process app.config */
  process_configs_file(app_config_path, APP_CONFIG, PRINT_LOG);
  
  /* process instn.config */
  process_configs_file(instn_config_path, INSTN_CONFIG, PRINT_LOG);

  /* process issue.config */
  process_configs_file(issue_config_path, ISSUE_CONFIG, PRINT_LOG);
}

#include <dirent.h>
#include <regex>

void trace_parser::process_mem_instns(std::string mem_instns_dir, bool PRINT_LOG) {
  mem_instns.resize(appcfg.get_kernels_num());
  for (int kid = 0; kid < appcfg.get_kernels_num(); ++kid)
    mem_instns[kid].resize(appcfg.get_kernel_grid_size(kid));
  
  // read all the mem instns from memory_traces dir
  DIR *dir;
  struct dirent *entry;
  
  if ((dir = opendir(mem_instns_dir.c_str())) == nullptr)
    std::cerr << "Not exist directory " << mem_instns_dir << ", please check." << std::endl;

  static const std::regex pattern(R"(kernel_(\d+)_block_(\d+)\.mem)");
  std::smatch match;
  
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_REG && 
        entry->d_name[strlen(entry->d_name) - 4] == '.' && 
        entry->d_name[strlen(entry->d_name) - 3] == 'm' && 
        entry->d_name[strlen(entry->d_name) - 2] == 'e' && 
        entry->d_name[strlen(entry->d_name) - 1] == 'm') {
      auto search = std::string(entry->d_name);
      std::string mem_instns_filepath = mem_instns_dir + "/" + entry->d_name;
      if (PRINT_LOG) std::cout << "Reading mem instns file : " 
                               << mem_instns_filepath << "..." << std::endl;
      
      if (std::regex_search(search, match, pattern)) {
        int kernel_id = std::stoi(match[1]);
        int block_id = std::stoi(match[2]);
        // std::cout << "x: " << kernel_id << " y: " << block_id << std::endl;
        std::ifstream fs;
        fs.open(mem_instns_filepath);
        if (!fs.is_open()) {
          std::cout << "Unable to open file: " << mem_instns_filepath << std::endl;
          exit(1);
        }
        std::string line;
        while (!fs.eof()) {
          getline(fs, line);
          if (line.empty())
            continue;
          else {
            // line: b0 969b38d6 7f60a5750000
            int addr_groups;
            unsigned _pc, _time_stamp;
            unsigned long long _addr_start1, _addr_start2;
            // get above parameters from line
            std::stringstream ss;
            ss.str(line);
            ss >> std::hex >> _pc >> _time_stamp;
            ss >> std::hex >> _addr_start1;
            size_t pos1 = line.find(' ');
            size_t pos2 = line.find(' ', pos1 + 1);
            size_t pos3 = line.find(' ', pos2 + 1);
            size_t pos4 = line.find(' ', pos3 + 1);

            if (pos4 != std::string::npos) {
              addr_groups = 2;
              ss >> std::hex >> _addr_start2;
            } else addr_groups = 1;

            // std::cout << "  " << line << " ";
            // std::cout << "  pc: " << std::hex << _pc << " " << _time_stamp << " " 
            //           << addr_groups << " " << _addr_start1 << std::endl;
            mem_instns[kernel_id-1][block_id].push_back(mem_instn(_pc, _addr_start1, _time_stamp, addr_groups, _addr_start2));
            // auto a = mem_instn(_pc, _addr_start1, _time_stamp, addr_groups, _addr_start2);
            // mem_instns[kernel_id-1][block_id].push_back(a);
          }
        }
        fs.close();
      } else {
        std::cerr << "Wrong name format of memory trace file: "<< entry->d_name << std::endl;
      }
    }
  }

  closedir(dir);
}

void trace_parser::read_mem_instns(bool PRINT_LOG) {
  if (configs_filepath.back() == '/') {
    mem_instns_dir = configs_filepath + "../memory_traces";
  } else {
    mem_instns_dir = configs_filepath + "/" + "../memory_traces";
  }

  if (PRINT_LOG) std::cout << std::endl;
  std::cout << "Memory traces dir: " << mem_instns_dir << std::endl;
  if (PRINT_LOG) std::cout << std::endl;

  mem_instns.resize(appcfg.get_kernels_num());
  for (int kid = 0; kid < appcfg.get_kernels_num(); ++kid)
    mem_instns[kid].resize(appcfg.get_kernel_grid_size(kid));

  // DIR *dir;
  // if ((dir = opendir(mem_instns_dir.c_str())) == nullptr) {
  //   std::cerr << "Not exist directory " << mem_instns_dir << ", please check." << std::endl;
  //   exit(1);
  // }

  // process mem_instns
  process_mem_instns(mem_instns_dir, PRINT_LOG);
}

std::pair<std::vector<trace_command>, int> trace_parser::parse_commandlist_file() {
  std::ifstream fs;
  fs.open(configs_filepath);

  std::cout << "Processing kernel list file : " << configs_filepath
            << std::endl;

  if (!fs.is_open()) {
    std::cout << "Unable to open file: " << configs_filepath << std::endl;
    exit(1);
  }

  std::string directory(configs_filepath);
  const size_t last_slash_idx = directory.rfind('/');
  if (std::string::npos != last_slash_idx) {
    directory = directory.substr(0, last_slash_idx);
  }

  std::string line, filepath;
  std::vector<trace_command> commandlist;
  int concurrent_kernel_kernel_nums = 0;

  while (!fs.eof()) {
    getline(fs, line);
    if (line.empty())
      continue;
    else if (line.substr(0, 10) == "MemcpyHtoD") {
      trace_command command;
      command.command_string = line;
      command.m_type = command_type::cpu_gpu_mem_copy;
      commandlist.push_back(command);
    } else if (line.substr(0, 6) == "kernel") {
      trace_command command;
      command.m_type = command_type::kernel_launch;
      filepath = directory + "/" + line;
      command.command_string = filepath;
      commandlist.push_back(command);
      concurrent_kernel_kernel_nums++;
    }
    // ignore gpu_to_cpu_memory_cpy
  }

  fs.close();
  return std::make_pair(commandlist, concurrent_kernel_kernel_nums);
}

void split(const std::string &str, std::vector<std::string> &cont,
           char delimi = ' ') {
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delimi)) {
    cont.push_back(token);
  }
}

void trace_parser::parse_memcpy_info(const std::string &memcpy_command,
                                     size_t &address, size_t &count) {
  std::vector<std::string> params;
  split(memcpy_command, params, ',');
  assert(params.size() == 3);
  std::stringstream ss;
  ss.str(params[1]);
  ss >> std::hex >> address;
  ss.clear();
  ss.str(params[2]);
  ss >> std::dec >> count;
}

std::vector<std::vector<inst_trace_t> *> trace_parser::get_next_threadblock_traces(
    unsigned trace_version, unsigned enable_lineinfo, std::ifstream *ifs,
    std::string kernel_name,
    unsigned kernel_id,
    unsigned num_warps_per_thread_block) {
  std::vector<std::vector<inst_trace_t> *> threadblock_traces;
  threadblock_traces.resize(num_warps_per_thread_block);
  unsigned block_id_x = 0, block_id_y = 0, block_id_z = 0;
  bool start_of_tb_stream_found = false;

  unsigned warp_id = 0;
  unsigned insts_num = 0;
  unsigned inst_count = 0;

  while (!ifs->eof()) {
    std::string line;
    std::stringstream ss;
    std::string string1, string2;

    getline(*ifs, line);

    if (line.length() == 0) {
      continue;
    } else {
      ss.str(line);
      ss >> string1 >> string2;
      if (string1 == "#BEGIN_TB") {
        if (!start_of_tb_stream_found) {
          start_of_tb_stream_found = true;
        } else
          assert(0 &&
                 "Parsing error: thread block start before the previous one "
                 "finishes");
      } else if (string1 == "#END_TB") {
        assert(start_of_tb_stream_found);
        break;  // end of TB stream
      } else if (string1 == "thread" && string2 == "block") {
        assert(start_of_tb_stream_found);
        sscanf(line.c_str(), "thread block = %d,%d,%d", &block_id_x,
               &block_id_y, &block_id_z);
        std::cout << "Parsing trace of " << line << "..." << std::endl;
      } else if (string1 == "warp") {
        // the start of new warp stream
        assert(start_of_tb_stream_found);
        sscanf(line.c_str(), "warp = %d", &warp_id);
      } else if (string1 == "insts") {
        assert(start_of_tb_stream_found);
        sscanf(line.c_str(), "insts = %d", &insts_num);
        threadblock_traces[warp_id] = new std::vector<inst_trace_t>();
        threadblock_traces[warp_id]->resize(insts_num);  // allocate all the space at once
        inst_count = 0;
      } else {
        assert(start_of_tb_stream_found);
        threadblock_traces[warp_id]
            ->at(inst_count)
            .parse_from_string(line, trace_version, enable_lineinfo,
                               kernel_name, kernel_id);
        inst_count++;
      }
    }
  }
  return threadblock_traces;
}
