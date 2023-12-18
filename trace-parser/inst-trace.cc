
#include "inst-trace.h"


bool is_number(const std::string &s) {
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}


std::string intToHex(unsigned num) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(4) << std::hex << num;
  return stream.str();
}

inst_trace_t::inst_trace_t() { memadd_info = NULL; }

// _inst_trace_t::_inst_trace_t() { memadd_info = NULL; }

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

// _inst_trace_t::_inst_trace_t(const _inst_trace_t &b) {
//   if (memadd_info != NULL) {
//     memadd_info = new inst_memadd_info_t();
//     memadd_info = b.memadd_info;
//   }
// }

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

unsigned _inst_trace_t::get_latency() const {
  return latency;
}

unsigned _inst_trace_t::get_initiation_interval() const {
  return initiation_interval;
}

enum FUNC_UNITS_NAME _inst_trace_t::get_func_unit() const {
  return func_unit;
}

void _inst_trace_t::parse_opcode_latency_info() {
  std::cout << "@@@@@@ " << opcode_tokens[0] << std::endl;
  auto it = Volta_OpcodeMap.find(opcode_tokens[0]);
  if (it == Volta_OpcodeMap.end()) {
    std::cout << "ERROR: opcode " << opcode << " not found in Volta_OpcodeMap" << std::endl;
    assert(0);
  } else {
    initiation_interval = 1;
    latency = 1;
    func_unit = NON_UNIT;
    
    auto category = it->second.opcode_category;
    std::cout << " SP_OP: " << SP_OP 
              << " DP_OP: " << DP_OP 
              << " SFU_OP: " << SFU_OP 
              << " INTP_OP: " << INTP_OP
              << " SPECIALIZED_UNIT_1_OP: " << SPECIALIZED_UNIT_1_OP
              << " SPECIALIZED_UNIT_2_OP: " << SPECIALIZED_UNIT_2_OP
              << " SPECIALIZED_UNIT_3_OP: " << SPECIALIZED_UNIT_3_OP << std::endl;
    std::cout << " category: " << category << std::endl;
    // std::cout << hw_cfg->get_opcode_latency_initiation_int(0) << std::endl;
    // std::cout << hw_cfg->get_opcode_latency_initiation_int(1) << std::endl;

    switch (category) {
      case SP_OP:
        latency = hw_cfg->get_opcode_latency_initiation_sp(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_sp(1);
        func_unit = SP_UNIT;
        break;
      case DP_OP:
        latency = hw_cfg->get_opcode_latency_initiation_dp(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_dp(1);
        func_unit = DP_UNIT;
        break;
      case SFU_OP:
        latency = hw_cfg->get_opcode_latency_initiation_sfu(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_sfu(1);
        func_unit = SFU_UNIT;
        break;
      case INTP_OP:
        latency = hw_cfg->get_opcode_latency_initiation_int(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_int(1);
        func_unit = INT_UNIT;
        break;
      case SPECIALIZED_UNIT_1_OP:
        latency = hw_cfg->get_opcode_latency_initiation_spec_op_1(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_spec_op_1(1);
        func_unit = SPEC_UNIT_1;
        break;
      case SPECIALIZED_UNIT_2_OP:
        latency = hw_cfg->get_opcode_latency_initiation_spec_op_2(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_spec_op_2(1);
        func_unit = SPEC_UNIT_2;
        break;
      case SPECIALIZED_UNIT_3_OP:
        latency = hw_cfg->get_opcode_latency_initiation_spec_op_3(0);
        initiation_interval = hw_cfg->get_opcode_latency_initiation_spec_op_3(1);
        func_unit = SPEC_UNIT_3;
        break;
      case LOAD_OP: // TODO: need to be modified
      case STORE_OP: // TODO: need to be modified
        latency = 100; // TODO: need to be modified
        initiation_interval = 100; // TODO: need to be modified
        func_unit = LDST_UNIT;
        break;
      default:
        latency = 1;
        initiation_interval = 1;
        /* Here, we suppose that the instns that are not included in the above 
         * cases will be executed in the INT_UNIT, but they still have latency 
         * of 1 and initiation_interval of 1. */
        func_unit = INT_UNIT;
        break;
    }

    std::cout << "  latency: " << latency << " initiation_interval: " << initiation_interval << std::endl;
  }
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
