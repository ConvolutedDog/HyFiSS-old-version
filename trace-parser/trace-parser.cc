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

_inst_trace_t::_inst_trace_t() { memadd_info = NULL; }

inst_trace_t::~inst_trace_t() {
  if (memadd_info != NULL) delete memadd_info;
}

_inst_trace_t::~_inst_trace_t() {
  if (memadd_info != NULL) delete memadd_info;
}

inst_trace_t::inst_trace_t(const inst_trace_t &b) {
  if (memadd_info != NULL) {
    memadd_info = new inst_memadd_info_t();
    memadd_info = b.memadd_info;
  }
}

_inst_trace_t::_inst_trace_t(const _inst_trace_t &b) {
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

bool _inst_trace_t::check_opcode_contain(const std::vector<std::string> &opcode,
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

/* Here, we split the opcode into individual tokens based on the '.' delimiter. */
std::vector<std::string> _inst_trace_t::get_opcode_tokens() const {
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

unsigned _inst_trace_t::get_datawidth_from_opcode(
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
    sscanf(temp.c_str(), "R%u", &reg_dest[i]);
  }

  ss >> opcode;

  ss >> reg_srcs_num;
  assert(reg_srcs_num <= MAX_SRC);
  for (unsigned i = 0; i < reg_srcs_num; ++i) {
    ss >> temp;
    sscanf(temp.c_str(), "R%u", &reg_src[i]);
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

bool _inst_trace_t::parse_from_string(std::string trace,
                                      unsigned kernel_id) {
  std::stringstream ss;
  ss.str(trace);

  // trace: Pred instn_str
  // trace: @!P7 SHFL.IDX 1 P7 4 R255 R255 R255 R255 
  // instn_str: OPCODE   dsts_num dsts   srcs_num srcs
  // instn_str: SHFL.IDX 1        P7     4        R255 R255 R255 R255 

  std::string temp;

  // Start Parsing
  if (trace.find("@") == 0) {
    ss >> pred_str;
  }

  ss >> opcode;

  ss >> std::dec >> reg_dsts_num;
  assert(reg_dsts_num <= MAX_DST);
  for (unsigned i = 0; i < reg_dsts_num; ++i) {
    ss >> temp;
    if (temp.find("P") == 0)
      sscanf(temp.c_str(), "P%u", &reg_dest[i]);
    else if (temp.find("R") == 0)
      sscanf(temp.c_str(), "R%u", &reg_dest[i]);
    else if (temp.find("[") == 0)
      // This condition will not occur in the trace file.
      sscanf(temp.c_str(), "[R%u]", &reg_dest[i]);
    else
      assert(0);
  }

  ss >> reg_srcs_num;
  assert(reg_srcs_num <= MAX_SRC);
  for (unsigned i = 0; i < reg_srcs_num; ++i) {
    ss >> temp;
    if (temp.find("P") == 0)
      sscanf(temp.c_str(), "P%u", &reg_src[i]);
    else if (temp.find("R") == 0)
      sscanf(temp.c_str(), "R%u", &reg_src[i]);
    else if (temp.find("[") == 0)
      sscanf(temp.c_str(), "[R%u]", &reg_src[i]);
    else
      assert(0);
  }

  // Here we do not parse memory infos for _inst_trace_t, because
  // the memory traces have been processed in memory model.
  memadd_info = new inst_memadd_info_t();
  memadd_info->empty = true;
  memadd_info->width = -1;
  
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
    
    std::stringstream ss;

    /* Read the "-app_kernels_id" config in the app config file and register the 
     * kernel config options. */
    std::ifstream inputFile;
    inputFile.open(config_path);
    if (!inputFile.good()) {
      fprintf(stderr, "\n\nOptionParser ** ERROR: Cannot open config file '%s'\n",
              config_path.c_str());
      exit(1);
    }

    std::string target_app_kernels_id = "-app_kernels_id";
    std::string target_concurrentKernels = "-device_concurrentKernels";
    
    std::string line;
    size_t commentStart;
    size_t found;
    std::string result;
    int comma_count;
    while (inputFile.good()) {
      getline(inputFile, line);
      commentStart = line.find_first_of("#");
      if (commentStart != line.npos) continue;
      found = line.find(target_app_kernels_id);
      if (found != std::string::npos) {
        result = line.substr(found + target_app_kernels_id.length());
        comma_count = std::count(result.begin(), result.end(), ',');
        kernels_num = comma_count + 1;
        // std::cout << ">>>" << result << "<<<" << std::endl;
        app_kernels_id_string = line.substr(found + target_app_kernels_id.length() + 1);
      }
      found = line.find(target_concurrentKernels);
      if (found != std::string::npos) {
        result = line.substr(found + target_concurrentKernels.length());
        // std::cout << ">>>" << result << "<<<" << std::endl;
        concurrentKernels = std::stoi(result);
      }

    }
    inputFile.clear();
    inputFile.seekg(0, std::ios::beg);
    // inputFile.close();

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
    
    // inputFile.open(config_path);
    for (int j = 0; j < kernels_num; ++j) {
      while (inputFile.good()) {
        getline(inputFile, line);
        commentStart = line.find_first_of("#");
        if (commentStart != line.npos) continue;

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

    inputFile.close();

    char *toks = new char[2048];
    char *tokd = toks;
    strcpy(toks, app_kernels_id_string.c_str());

    app_kernels_id.resize(kernels_num);

    toks = strtok(toks, ",");

    for (int i = 0; i < kernels_num; i++) {
      assert(toks);
      int ntok = sscanf(toks, "%d", &app_kernels_id[i]);
      assert(ntok == 1);
      toks = strtok(NULL, ",");
    }

    delete[] tokd;
    delete[] toks;

    if (PRINT_LOG) fprintf(stdout, ">>> APP config Options <<<:\n");

    m_valid = true;
}

void instn_config::init(std::string config_path, bool PRINT_LOG) {
  std::ifstream inputFile;
  /* Open config file, stream every line into a continuous buffer and 
   * get rid of comments in the process. */
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

      _inst_trace_t* instn_info = new _inst_trace_t(kernel_id - 1, pc, instn_str);
      // instn_info.kernel_id = kernel_id;
      // instn_info.pc = pc;
      // instn_info.instn_str = instn_str;

      // instn_info_vector.push_back(instn_info);
      // kernel_id starts from 1
      instn_info_vector[std::make_pair(kernel_id - 1, pc)] = instn_info;
    }
  }
  inputFile.close();

  if (PRINT_LOG) fprintf(stdout, ">>> INSTN config Options <<<:\n");
  // traversal instn_info_vector
  // if (PRINT_LOG) {
  //   for (auto& pair : instn_info_vector) {
  //     auto _kernel_id = pair.first.first;
  //     auto _pc = pair.first.second;
  //     std::cout << "-instn " << std::setw(5) << std::right 
  //               << std::dec << _kernel_id << std::dec;
  //     std::cout << " " << std::setw(8) << std::left
  //               << std::hex << _pc << std::dec;
  //     std::cout << " " << std::setw(55) << std::left 
  //               << pair.second.instn_str << std::endl;
  //   }
  // }
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
    std::stringstream ss;

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
    inputFile.clear();
    inputFile.seekg(0, std::ios::beg);

    trace_issued_sm_id_blocks_str.resize(trace_issued_sms_num);

    std::vector<int> has_found_j;
    while (inputFile.good()) {
      getline(inputFile, line);
      commentStart = line.find_first_of("#");
      if (commentStart != line.npos) continue;

      for (int j = 0; j < trace_issued_sms_num; ++j) {
        if (std::find(has_found_j.begin(), has_found_j.end(), j) == has_found_j.end()) {
          int sm_num = trace_issued_sms_vector[j];
          ss.str("");
          ss << "-trace_issued_sm_id_" << std::to_string(sm_num);
          size_t found = line.find(ss.str());
          if (found != std::string::npos) {
            has_found_j.push_back(j);
            trace_issued_sm_id_blocks_str[j] = line.substr(found + ss.str().length() + 1);
            break;
          }
        }
      }
    }
    inputFile.close();

    if (PRINT_LOG) fprintf(stdout, ">>> ISSUE config Options <<<:\n");

    
    /* parse the issued blocks list on each SM */
    std::string blocks_info_str;
    trace_issued_sm_id_blocks.resize(trace_issued_sms_num);
    for (int j = 0; j < trace_issued_sms_num; ++j) {
      // std::cout << "trace_issued_sm_id_blocks_str[" << j << "]: " 
      //           << trace_issued_sm_id_blocks_str[j].c_str() << std::endl;
      blocks_info_str = trace_issued_sm_id_blocks_str[j].c_str();
      trace_issued_sm_id_blocks[j] = parse_blocks_info(blocks_info_str);
    }
    
    // for (int j = 0; j < trace_issued_sms_num; ++j) {
    //   std::cout << "@@@ trace_issued_sm_id_blocks[" << j << "]: " << std::endl;
    //   for (unsigned k = 0; k < trace_issued_sm_id_blocks[j].size(); ++k) {
    //     std::cout << "  kernel_id: " << trace_issued_sm_id_blocks[j][k].kernel_id << " | ";
    //     std::cout << "  block_id: " << trace_issued_sm_id_blocks[j][k].block_id << std::endl;
    //   }
    // }
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
        sscanf(line.c_str(), "-kernel id = %u", &kernel_info->kernel_id);
      } else if (string1 == "grid" && string2 == "dim") {
        sscanf(line.c_str(), "-grid dim = (%u,%u,%u)", &kernel_info->grid_dim_x,
               &kernel_info->grid_dim_y, &kernel_info->grid_dim_z);
      } else if (string1 == "block" && string2 == "dim") {
        sscanf(line.c_str(), "-block dim = (%u,%u,%u)", &kernel_info->tb_dim_x,
               &kernel_info->tb_dim_y, &kernel_info->tb_dim_z);
      } else if (string1 == "shmem" && string2 == "=") {
        sscanf(line.c_str(), "-shmem = %u", &kernel_info->shmem);
      } else if (string1 == "nregs") {
        sscanf(line.c_str(), "-nregs = %u", &kernel_info->nregs);
      } else if (string1 == "cuda" && string2 == "stream") {
        sscanf(line.c_str(), "-cuda stream id = %lu",
               &kernel_info->cuda_stream_id);
      } else if (string1 == "binary" && string2 == "version") {
        sscanf(line.c_str(), "-binary version = %u",
               &kernel_info->binary_verion);
      } else if (string1 == "enable" && string2 == "lineinfo") {
        sscanf(line.c_str(), "-enable lineinfo = %u",
               &kernel_info->enable_lineinfo);
      } else if (string1 == "nvbit" && string2 == "version") {
        const size_t equal_idx = line.find('=');
        kernel_info->nvbit_verion = line.substr(equal_idx + 1);

      } else if (string1 == "accelsim" && string2 == "tracer") {
        sscanf(line.c_str(), "-accelsim tracer version = %u",
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

void trace_parser::process_configs_file(const std::string config_path, int config_type, bool PRINT_LOG) {
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

void trace_parser::judge_concurrent_issue() {
  /* Judge if kernels can be issued concurrently, the condition for concurrency is that 
   * it must be satisfied.
   * 1. the GPU device must support concurrent execution, we can check this through CUDA 
   *    device query, we have report '-device_concurrentKernels x' in the tracing tool.
   * 2. two kernels have different stream id, we also report the stream id of each kernel
   *    in the tracing tool.
   * 3. even if the device supports concurrent execution, if one of the kernels use too 
   *    many resources (such as registers, shared memory, etc.), they may not be able to 
   *    execute concurrently, we also report the resource usage of all the kernels in the 
   *    tracing tool.
   * 4. if one kernel depends on the result of another kernel, then these two kernels can
   *    not execute concurrently, the check of data dependency is hard, because CUDA does 
   *    not inherently offer a direct mechanism to ascertain the existence of such depen-
   *    dencies, thereby necessitating developers to manually manage and track these during 
   *    code development, what we should do is to judge the stream id. */
  // if (get_appcfg()->get_concurrentKernels() == 1) {
  //   /* The GPU device supports concurrent execution. */
  //   // concurrent_kernels
  //   /* Now we must know how many stream id can not be concurrently issued. */
  //   std::vector<int> remaining_kernels;
  //   for (int i = 0; i < get_appcfg()->get_kernels_num(); ++i) {
  //     remaining_kernels.push_back(i);
  //   }

  //   while (!remaining_kernels.empty()) {
  //     int kernel_id = remaining_kernels[0];
  //     remaining_kernels.erase(remaining_kernels.begin());
  //     std::vector<int> tmp = {kernel_id};
  //     concurrent_kernels.push_back(tmp);
  //     int stream_id = get_appcfg()->get_kernel_cuda_stream_id(kernel_id);
  //     for (int i = 0; i < remaining_kernels.size(); ++i) {
  //       if (get_appcfg()->get_kernel_cuda_stream_id(remaining_kernels[i]) != stream_id) {
  //         concurrent_kernels.back().push_back(remaining_kernels[i]);
  //         remaining_kernels.erase(remaining_kernels.begin() + i);
  //         i--;
  //       }
  //     }
  //   }
    
  //   // int last_add_to_stream_idx;
  //   // int last_stream_id;

  //   // int i = 0;
  //   // while(i < get_appcfg()->get_kernels_num()) {
  //   //     if (i == 0) {
  //   //       // just add kid-0 to the first stream id
  //   //       last_add_to_stream_idx = 0;
  //   //       std::vector<int> tmp = {0};
  //   //       concurrent_kernels.push_back(tmp);
  //   //       last_stream_id = get_appcfg()->get_kernel_cuda_stream_id(0);
  //   //       i++;
  //   //     } else {
  //   //       // check if the current kernel can be added to the last stream id
  //   //       std::cout << "last_stream_id: " << last_stream_id << std::endl;
  //   //       std::cout << "get_appcfg()->get_kernel_cuda_stream_id(i): " << get_appcfg()->get_kernel_cuda_stream_id(i) << std::endl;
  //   //       if (get_appcfg()->get_kernel_cuda_stream_id(i) != last_stream_id) {
  //   //         // can add to the last stream id
  //   //         concurrent_kernels[last_add_to_stream_idx].push_back(i);
  //   //         i++;
  //   //       } else {
  //   //         // cannot add to current stream id, just add to the next stream id, and
  //   //         // the remaining kernels should also add to the next stream id.
            
  //   //         last_add_to_stream_idx += 1;
  //   //         std::cout << "last_add_to_stream_idx: " << last_add_to_stream_idx << std::endl;
  //   //         std::vector<int> tmp = {i};
  //   //         concurrent_kernels.push_back(tmp);
  //   //         last_stream_id = get_appcfg()->get_kernel_cuda_stream_id(i);
  //   //         i++;
  //   //       }
  //   //     }
  //   // }

  //   // int idx = 0;
  //   // for (auto x : concurrent_kernels) {
  //   //   std::cout << "@@@ x-" << idx++ << "  ";
  //   //   for (auto y : x) {
  //   //     std::cout << y << " ";
  //   //   }
  //   //   std::cout << std::endl;
  //   // }
  // } else {
  //   /* The GPU device does not support concurrent execution. */
  //   for (int i = 0; i < get_appcfg()->get_kernels_num(); ++i) {
  //     concurrent_kernels[i].push_back(i);
  //   }
  // }
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

  /* process whether kernels can be issue concurrently */
  judge_concurrent_issue();
}

#include <dirent.h>
#include <regex>
#include <algorithm>


bool judge_format_mem(char* d_name, std::vector<std::pair<int, int>>* x) {
  std::string name = d_name;
  std::regex pattern("kernel_(\\d+)_block_(\\d+).mem");
  std::smatch match;

  if (std::regex_match(name, match, pattern)) {
    int xx = std::stoi(match[1]);
    int yy = std::stoi(match[2]);

    if (std::find((*x).begin(), (*x).end(), std::make_pair(xx, yy)) != (*x).end()) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool judge_format_compute_kernel_id(char* d_name, std::vector<std::pair<int, int>>* x) {
  std::string name = d_name;
  std::regex pattern("kernel_(\\d+).sass");
  std::smatch match;

  if (std::regex_match(name, match, pattern)) {
    int xx = std::stoi(match[1]);

    std::vector<int> need_processed_kernel_ids;
    for (auto i : (*x)) {
      need_processed_kernel_ids.push_back(i.first);
    }

    // check if xx is in need_processed_kernel_ids
    if (std::find(need_processed_kernel_ids.begin(), need_processed_kernel_ids.end(), 
        xx) != need_processed_kernel_ids.end()) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool judge_format_compute_kernel_id_new(int kernel_id, int block_id, std::vector<std::pair<int, int>>* x) {
  // check if xx is in need_processed_kernel_ids
  if (std::find((*x).begin(), (*x).end(), std::make_pair(kernel_id, block_id)) != (*x).end()) {
    return true;
  } else {
    return false;
  }
}

bool judge_format_compute_block_id(int kernel_id, int block_id, std::vector<std::pair<int, int>>* x) {
  std::pair<int, int> target = std::make_pair(kernel_id, block_id);
  if (std::find(x->begin(), x->end(), target) != x->end())
    return true;
  else
    return false;
}


void trace_parser::process_mem_instns(const std::string mem_instns_dir, bool PRINT_LOG, std::vector<std::pair<int, int>>* x) {
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
        judge_format_mem(entry->d_name, x) &&
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
            // origin format: pos      1             2        3        4            5
            //                line  pc  opcode        mask     tstamp   addr_start1  addr_start2
            //                line: da0 LDG.E.U16.SYS 11111111 e4da28c9 7fbc157fe660 ____________
            // new format: pos      1                      2        3        4              5   6     7   8     9   10    11  12 ...
            //             line  pc  opcode                 mask     tstamp   x addr_start1    y stride:num                        z addr_start2  stride:num
            //             line: a00 LDG.E.U16.CONSTANT.GPU ffffffff dbf2ab7a 1 0x7ff3c3721600 7 2:7 3f2:1 2:7 3f2:1 2:7 3f2:1 2:7 _ ____________ ____________
            //                   x : if has addr_start2, x=2, else x=1. x == _addr_groups
            //                   y : num of addr_start1's stride-num pairs, y == stride_num_pairs_1
            //                   z : num of addr_start2's stride-num pairs, z == stride_num_pairs_2

            int _addr_groups;
            unsigned _pc, _time_stamp;
            std::string _opcode;
            unsigned _mask;
            unsigned long long _addr_start1, _addr_start2;
            unsigned stride_num_pairs_1, stride_num_pairs_2;
            // get above parameters from line
            std::stringstream ss;
            ss.str(line);
            // std::cout << "line: " << line << std::endl;
            ss >> std::hex >> _pc;
            ss >> _opcode;
            ss >> std::hex >> _mask;
            ss >> std::hex >> _time_stamp;
            ss >> std::hex >> _addr_groups;
            ss >> std::hex >> _addr_start1;
            ss >> std::hex >> stride_num_pairs_1;

            std::vector<long long> _stride_num;
            std::string tmp_stride_num;
            // std::cout << "stride_num_pairs_1: " << stride_num_pairs_1 << std::endl;
            for (unsigned _i = 0; _i < stride_num_pairs_1; _i++) {
              ss >> tmp_stride_num;
              size_t pos = tmp_stride_num.find(':');
              // std::cout << "tmp_stride_num: " << tmp_stride_num << std::endl;
              long long stride = std::stoll(tmp_stride_num.substr(0, pos), nullptr, 10);
              int num = std::stoi(tmp_stride_num.substr(pos + 1), nullptr, 10);
              // std::cout << "stride: " << stride << " num: " << num 
              //           << " tmp_stride_num.length(): " << tmp_stride_num.length() << std::endl;
              for (int _j = 0; _j < num; _j++) {
                _stride_num.push_back(stride);
              }
            }

            if (_addr_groups == 2) {
              ss >> std::hex >> _addr_start2;
              ss >> std::hex >> stride_num_pairs_2;
              for (unsigned _i = 0; _i < stride_num_pairs_2; _i++) {
                ss >> tmp_stride_num;
                size_t pos = tmp_stride_num.find(':');
                long long stride = std::stoll(tmp_stride_num.substr(0, pos), nullptr, 10);
                int num = std::stoi(tmp_stride_num.substr(pos + 1), nullptr, 10);
                for (int _j = 0; _j < num; _j++) {
                  _stride_num.push_back(stride);
                }
              }
            }

            /*
            std::cout << mem_instns_filepath << std::endl;
            std::cout << "  " << line << " " << std::endl;
            std::cout << "  pc: " << std::hex << _pc << " " << _opcode << " " << _mask << " " << _time_stamp << " " << std::endl; 
            std::cout << _addr_groups << " " << _addr_start1 << std::endl; 
            for (auto x: _stride_num) {
              std::cout << "  |" << std::hex << x << "| ";
            }
            */
            // if (_opcode.find("LD") != std::string::npos)
            mem_instns[kernel_id-1][block_id].push_back(mem_instn(_pc, 
                                                                  _addr_start1, 
                                                                  _time_stamp, 
                                                                  _addr_groups, 
                                                                  _addr_start2,
                                                                  _mask,
                                                                  _opcode,
                                                                  &_stride_num));
            
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

void trace_parser::read_mem_instns(bool PRINT_LOG, std::vector<std::pair<int, int>>* x) {
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
  process_mem_instns(mem_instns_dir, PRINT_LOG, x);
}

void trace_parser::process_compute_instns(std::string compute_instns_dir, bool PRINT_LOG, std::vector<std::pair<int, int>>* x) {
  // read all the mem instns from memory_traces dir
  DIR *dir;
  struct dirent *entry;
  
  if ((dir = opendir(compute_instns_dir.c_str())) == nullptr)
    std::cerr << "Not exist directory " << compute_instns_dir << ", please check." << std::endl;

  static const std::regex pattern(R"(kernel_(\d+)\.sass)");
  std::smatch match;
  
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_REG && 
        judge_format_compute_kernel_id(entry->d_name, x) /* &&
        entry->d_name[strlen(entry->d_name) - 5] == '.' && 
        entry->d_name[strlen(entry->d_name) - 4] == 's' && 
        entry->d_name[strlen(entry->d_name) - 3] == 'a' && 
        entry->d_name[strlen(entry->d_name) - 2] == 's' &&
        entry->d_name[strlen(entry->d_name) - 1] == 's' */) {
      auto search = std::string(entry->d_name);
      std::string compute_instns_filepath = compute_instns_dir + "/" + entry->d_name;
      if (PRINT_LOG) std::cout << "Reading mem instns file : " 
                               << compute_instns_filepath << "..." << std::endl;
      
      if (std::regex_search(search, match, pattern)) {
        int kernel_id = std::stoi(match[1]);
        int num_warps_per_block = get_appcfg()->get_num_warp_per_block(kernel_id - 1);
START_TIMER(6);
        // int block_id = std::stoi(match[2]);
        // std::cout << "x: " << kernel_id << " y: " << block_id << std::endl;
        std::ifstream fs;
        fs.open(compute_instns_filepath);
        if (!fs.is_open()) {
          std::cout << "Unable to open file: " << compute_instns_filepath << std::endl;
          exit(1);
        }
        std::string line;
        while (!fs.eof()) {
          getline(fs, line);
          if (line.empty())
            continue;
          else {
            std::string _pc_str;
            unsigned _pc;

            std::string _mask_str;
            unsigned _mask;
            
            std::string _gwarp_id_str;
            unsigned _gwarp_id;
            // get above parameters from line
            std::stringstream ss;
            ss.str(line);
            // std::cout << "line: " << line << std::endl;
            // ss >> std::hex >> _pc;
            // ss >> std::hex >> _mask;
            // ss >> std::hex >> _gwarp_id;
            while (ss >> _pc_str >> _mask_str >> _gwarp_id_str) {
              _gwarp_id = std::stoi(_gwarp_id_str, nullptr, 16); // judge if _gwarp_id belongs to the x
              
              // if (0) {
              //   std::cout << "_gwarp_id: " << _gwarp_id << " ";
              //   std::cout << "num_warps_per_block: " << num_warps_per_block << " ";
              //   std::cout << "kernel_id: " << kernel_id << " ";
              //   // for (auto i : *x) {
              //   //   std::cout << i.first << " " << i.second << std::endl;
              //   // }
              //   std::cout << judge_format_compute_block_id(kernel_id, (int)(_gwarp_id / num_warps_per_block), x) << std::endl;
              // }

              // bool valid_process = judge_format_compute_block_id(kernel_id, (int)(_gwarp_id / num_warps_per_block), x);

              if (1) {
                _pc = std::stoi(_pc_str, nullptr, 16);
                if (_pc_str == " " || _pc_str == "" || _pc_str == "\n") break;
                if (_mask_str == "!") _mask = 0xffffffff;
                else {
                  _mask = std::stoi(_mask_str, nullptr, 16);
                }
                // kernel_id, pc
                _inst_trace_t* _inst_trace = (*get_instncfg()->get_instn_info_vector())[std::make_pair(kernel_id-1, _pc)]; // ?????
                // inst_trace's kernel_id starts from 0
                // compute_instn's kernel_id starts from 0
                conpute_instns[kernel_id-1][_gwarp_id].push_back(compute_instn(kernel_id - 1, _pc, _mask, _gwarp_id, _inst_trace));
              }
            }
          }
        }
        fs.close();
STOP_AND_REPORT_TIMER_pass(-1, 6);
      } else {
        std::cerr << "Wrong name format of memory trace file: "<< entry->d_name << std::endl;
      }
    }
  }

  closedir(dir);
}


void trace_parser::process_compute_instns_new(std::string compute_instns_dir, bool PRINT_LOG, std::vector<std::pair<int, int>>* x) {
  // read all the mem instns from memory_traces dir
  DIR *dir;
  struct dirent *entry;
  
  if ((dir = opendir(compute_instns_dir.c_str())) == nullptr)
    std::cerr << "Not exist directory " << compute_instns_dir << ", please check." << std::endl;

  static const std::regex pattern(R"(kernel_(\d+)_gwarp_id_(\d+)\.split\.sass)");
  std::smatch match;
  
  while ((entry = readdir(dir)) != nullptr) {
    
    if (entry->d_type == DT_REG && 
        entry->d_name[strlen(entry->d_name) - 6] == 't' /*&&
        entry->d_name[strlen(entry->d_name) - 5] == '.' && 
        entry->d_name[strlen(entry->d_name) - 4] == 's' && 
        entry->d_name[strlen(entry->d_name) - 3] == 'a' && 
        entry->d_name[strlen(entry->d_name) - 2] == 's' &&
        entry->d_name[strlen(entry->d_name) - 1] == 's'*/) {
      
      auto search = std::string(entry->d_name);
      
      if (std::regex_search(search, match, pattern)) {
        int kernel_id = std::stoi(match[1]);
        int gwarp_id = std::stoi(match[2]);
        int num_warps_per_block = get_appcfg()->get_num_warp_per_block(kernel_id - 1);
        
        int block_id = (int)(gwarp_id / num_warps_per_block);
        // std::cout << "kernel_id: " << kernel_id << " " << "gwarp_id: " << gwarp_id << "num_warps_per_block: " << num_warps_per_block << std::endl;
        if (!judge_format_compute_kernel_id_new(kernel_id, block_id, x)) continue;
        
        
        std::string compute_instns_filepath = compute_instns_dir + "/" + entry->d_name;
        if (PRINT_LOG) std::cout << "Reading mem instns file : " 
                                << compute_instns_filepath << "..." << std::endl;

// START_TIMER(6);
        // int block_id = std::stoi(match[2]);
        // std::cout << "x: " << kernel_id << " y: " << block_id << std::endl;
        std::ifstream fs;
        fs.open(compute_instns_filepath);
        if (!fs.is_open()) {
          std::cout << "Unable to open file: " << compute_instns_filepath << std::endl;
          exit(1);
        }
        std::string line;
        while (!fs.eof()) {
          getline(fs, line);
          if (line.empty())
            continue;
          else {
            unsigned _pc;

            std::string _mask_str;
            unsigned _mask;
            
            // get above parameters from line
            std::stringstream ss;
            ss.str(line);
            // std::cout << "line: " << line << std::endl;
            ss >> std::hex >> _pc;
            ss >> std::hex >> _mask_str;
            
                if (_mask_str == "!") _mask = 0xffffffff;
                else {
                  _mask = std::stoi(_mask_str, nullptr, 16);
                }
                // kernel_id, pc
                _inst_trace_t* _inst_trace = (*get_instncfg()->get_instn_info_vector())[std::make_pair(kernel_id-1, _pc)]; // ?????
                // inst_trace's kernel_id starts from 0
                // compute_instn's kernel_id starts from 0
                conpute_instns[kernel_id-1][gwarp_id].push_back(compute_instn(kernel_id - 1, _pc, _mask, gwarp_id, _inst_trace));
          }
        }
        fs.close();
// STOP_AND_REPORT_TIMER_pass(-1, 6);
      } else {
        std::cerr << "Wrong name format of memory trace file: "<< entry->d_name << std::endl;
      }
    }
  }

  // traverse conpute_instns
  std::cout << conpute_instns.size() << " | " << conpute_instns[0].size() << std::endl;

  closedir(dir);
}

void trace_parser::read_compute_instns(bool PRINT_LOG, std::vector<std::pair<int, int>>* x) {
  if (configs_filepath.back() == '/') {
    compute_instns_dir = configs_filepath + "../sass_traces";
  } else {
    compute_instns_dir = configs_filepath + "/" + "../sass_traces";
  }

  if (PRINT_LOG) std::cout << std::endl;
  std::cout << "Compute traces dir: " << compute_instns_dir << std::endl;
  if (PRINT_LOG) std::cout << std::endl;

  /* kernel_id -> warp_id -> compute_instn */
  /* std::vector<std::vector<std::vector<compute_instn>>> conpute_instns; */
  conpute_instns.resize(appcfg.get_kernels_num());
  for (int kid = 0; kid < appcfg.get_kernels_num(); ++kid)
    conpute_instns[kid].resize(appcfg.get_num_global_warps(kid));
  
  // process mem_instns
  // process_compute_instns(compute_instns_dir, PRINT_LOG, x); // the speed is too low
  process_compute_instns_new(compute_instns_dir, PRINT_LOG, x); // the speed is very fast
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
    const std::string kernel_name,
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
        sscanf(line.c_str(), "thread block = %u,%u,%u", &block_id_x,
               &block_id_y, &block_id_z);
        std::cout << "Parsing trace of " << line << "..." << std::endl;
      } else if (string1 == "warp") {
        // the start of new warp stream
        assert(start_of_tb_stream_found);
        sscanf(line.c_str(), "warp = %u", &warp_id);
      } else if (string1 == "insts") {
        assert(start_of_tb_stream_found);
        sscanf(line.c_str(), "insts = %u", &insts_num);
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
