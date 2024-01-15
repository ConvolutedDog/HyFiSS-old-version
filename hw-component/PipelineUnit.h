
#include <vector>
#include <bitset>

#include "../trace-driven/register-set.h"
#include "../hw-parser/hw-parser.h"
#include "../trace-parser/trace-parser.h"

#ifndef PIPELINEUNIT_H
#define PIPELINEUNIT_H

#define MAX_ALU_LATENCY 512

/*
SP单元和SFU单元的时序模型主要在 shader.h 中定义的 pipelined_simd_unit 类中实现。模拟单元的具体类（
sp_unit类和sfu类）是从这个类派生出来的，由可重载的 can_issue() 成员函数来指定单元可执行的指令类型。

SP单元通过OC_EX_SP流水线寄存器连接到操作收集器单元；SFU单元通过OC_EX_SFU流水线寄存器连接到操作数收集
器单元。两个单元通过WB_EX流水线寄存器共享一个共同的写回阶段。为了防止两个单元因写回阶段的冲突而停滞，
每条进入任何一个单元的指令都必须在发出到目标单元之前在结果总线（m_result_bus）上分配一个槽（见shader
_core_ctx::execute()）。

手册[ALU流水线软件模型]中的图提供了一个概览，介绍了pipelined_simd_unit如何为不同类型的指令建立吞吐量
和延迟。

在每个pipelined_simd_unit中，issue(warp_inst_t*&)成员函数将给定的流水线寄存器的内容移入m_dispatch_
reg。然后指令在m_dispatch_reg等待initiation_interval个周期。在此期间，没有其他的指令可以发到这个单
元，所以这个等待是指令的吞吐量的模型。等待之后，指令被派发到内部流水线寄存器m_pipeline_reg进行延迟建
模。派发的位置是确定的，所以在m_dispatch_reg中花费的时间也被计入延迟中。每个周期，指令将通过流水线寄
存器前进，最终进入m_result_port，这是共享的流水线寄存器，通向SP和SFU单元的共同写回阶段。

各类指令的吞吐量和延迟在cuda-sim.cc的ptx_instruction::set_opcode_and_latency()中指定。这个函数在预
解码时被调用。
*/
class pipelined_simd_unit {
 public:
  pipelined_simd_unit(register_set *result_port,
                      unsigned max_latency,
                      unsigned issue_reg_id, 
                      hw_config* hw_cfg, trace_parser* tracer);
  ~pipelined_simd_unit();
  // modifiers
  /* virtual void cycle();*/
  //issue(warp_inst_t*&)成员函数将给定的流水线寄存器的内容移入m_dispatch_reg。
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  //lane的意思为一个warp中有32个线程，而在流水线寄存器中可能暂存了很多条指令，这些指令的每对应的线程掩
  //码的每一位都是一个lane。即遍历流水线寄存器中的非空指令，返回所有指令的整体线程掩码（所有指令线程掩
  //码的或值）。
  /* virtual */unsigned get_active_lanes_in_pipeline() {return 0;};

  // virtual void active_lanes_in_pipeline() = 0;
  
  /*
      virtual void issue( register_set& source_reg )
      {
          //move_warp(m_dispatch_reg,source_reg);
          //source_reg.move_out_to(m_dispatch_reg);
          simd_function_unit::issue(source_reg);
      }
  */
  // accessors
  virtual bool stallable() const { return false; }
  //判断一条指令能否发射，即判断m_dispatch_reg是否为空，其在occupied对应的标识位是否为空。
  virtual bool can_issue(unsigned latency) const;
  virtual bool is_issue_partitioned() = 0;
  //获取发射寄存器的ID。
  unsigned get_issue_reg_id() { return m_issue_reg_id; }
  void print() const {
    printf("%s dispatch= ", m_name.c_str());
    // m_dispatch_reg->print(fp);
    std::cout << "m_dispatch_reg: (pc,wid,kid,uid) " << m_dispatch_reg->pc
              << " " << m_dispatch_reg->wid << " " << m_dispatch_reg->kid
              << " " << m_dispatch_reg->uid << std::endl;
    for (int s = m_pipeline_depth - 1; s >= 0; s--) {
      if (m_pipeline_reg[s]->m_valid) {
        printf("      %s[%2d] ", m_name.c_str(), s);
        // m_pipeline_reg[s]->print();
        std::cout << "m_pipeline_reg[" << s << "]: (pc,wid,kid,uid) "
                  << m_pipeline_reg[s]->pc << " " << m_pipeline_reg[s]->wid
                  << " " << m_pipeline_reg[s]->kid << " "
                  << m_pipeline_reg[s]->uid << std::endl;
      }
    }
  }

  void cycle();

  virtual unsigned clock_multiplier() const { return 1; };
  //获取SIMD单元的名称。
  const char *get_name() { return m_name.c_str(); }

 protected:
  //流水线的深度。
  unsigned m_pipeline_depth = MAX_ALU_LATENCY;
  //流水线寄存器。
  std::vector<inst_fetch_buffer_entry*> m_pipeline_reg;
  //结果端口。
  register_set *m_result_port;
  //发射寄存器的ID。
  unsigned m_issue_reg_id;  // if sub_core_model is enabled we can only issue
                            // from a subset of operand collectors

  unsigned active_insts_in_pipeline;

  std::string m_name;
  //SIMD单元的dispatch寄存器。
  inst_fetch_buffer_entry *m_dispatch_reg;

  hw_config* m_hw_cfg;
  trace_parser* m_tracer;
  //流水线寄存器至多512个槽的位图，标识每个槽是否被占用。
  std::bitset<MAX_ALU_LATENCY> occupied;
};

/*
register_set *result_port,
unsigned max_latency,
unsigned issue_reg_id, 
hw_config* hw_cfg,
trace_parser* tracer
*/

class sfu : public pipelined_simd_unit {
 public:
  sfu(register_set *result_port, unsigned issue_reg_id,
      hw_config* hw_cfg, trace_parser* tracer)
      : pipelined_simd_unit(result_port, 
                            hw_cfg->get_opcode_latency_initiation_sfu(0), 
                            issue_reg_id,
                            hw_cfg, tracer) {
    m_name = "SFU";
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return false; }
};


class dp_unit : public pipelined_simd_unit {
 public:
  dp_unit(register_set *result_port, unsigned issue_reg_id,
          hw_config* hw_cfg, trace_parser* tracer)
          : pipelined_simd_unit(result_port, 
                                hw_cfg->get_opcode_latency_initiation_dp(0), 
                                issue_reg_id,
                                hw_cfg, tracer) {
    m_name = "DP";
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return false; }
};

class sp_unit : public pipelined_simd_unit {
 public:
  sp_unit(register_set *result_port, unsigned issue_reg_id,
          hw_config* hw_cfg, trace_parser* tracer)
          : pipelined_simd_unit(result_port, 
                                hw_cfg->get_opcode_latency_initiation_sp(0), 
                                issue_reg_id,
                                hw_cfg, tracer) {
    m_name = "SP";
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return false; }
};

class tensor_core : public pipelined_simd_unit {
 public:
  tensor_core(register_set *result_port, unsigned issue_reg_id,
              hw_config* hw_cfg, trace_parser* tracer)
              : pipelined_simd_unit(result_port, 
                                    hw_cfg->get_opcode_latency_initiation_tensor_core(0), 
                                    issue_reg_id,
                                    hw_cfg, tracer) {
    m_name = "TENSOR_CORE";
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return false; }
};

class int_unit : public pipelined_simd_unit {
 public:
  int_unit(register_set *result_port, unsigned issue_reg_id,
           hw_config* hw_cfg, trace_parser* tracer)
           : pipelined_simd_unit(result_port, 
                                 hw_cfg->get_opcode_latency_initiation_int(0), 
                                 issue_reg_id,
                                 hw_cfg, tracer) {
    m_name = "INT";
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return false; }
};

class specialized_unit : public pipelined_simd_unit {
 public:
  specialized_unit(register_set *result_port, unsigned issue_reg_id,
                   hw_config* hw_cfg, trace_parser* tracer, unsigned index)
                   : pipelined_simd_unit(result_port, 
                                         hw_cfg->get_opcode_latency_initiation_spec_unit(index, 0), 
                                         issue_reg_id,
                                         hw_cfg, tracer) {
    m_index = index;
    m_name = std::string("SPECIALIZED_UNIT") + std::to_string(m_index);
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return false; }

 private:
  unsigned m_index;
};

class mem_unit : public pipelined_simd_unit {
 public:
  mem_unit(register_set *result_port, unsigned issue_reg_id,
           hw_config* hw_cfg, trace_parser* tracer)
           : pipelined_simd_unit(result_port, 
                                 10,  
                                 // TODO: should be latency of L1 miss + L2 miss + DRAM access time
                                 issue_reg_id,
                                 hw_cfg, tracer) {
    m_name = "MEM";
  }

  virtual bool can_issue(const inst_fetch_buffer_entry &inst) const;
  virtual unsigned clock_multiplier() const { return 1; }
  virtual void issue(register_set &source_reg);
  virtual void issue(register_set &source_reg, unsigned reg_id);
  bool is_issue_partitioned() { return true; }
  virtual bool stallable() const { return true; }
};

#endif