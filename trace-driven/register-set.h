
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <memory>

#include "entry.h"
#include "../hw-parser/hw-parser.h"

#ifndef REGISTER_SET_H
#define REGISTER_SET_H

class register_set {
 public:
  //构造函数，用于初始化寄存器集合，寄存器集合中有num个寄存器，每个寄存器含有一条指令。
  register_set(){};
  
  register_set(const unsigned num, const std::string name, hw_config* hw_cfg) {
    // for (unsigned i = 0; i < num; i++) {
    //   regs_instance.push_back(inst_fetch_buffer_entry());
    //   regs.push_back(&(regs_instance.back()));
    // }
    regs.reserve(num);
    for (unsigned i = 0; i < num; i++) {
      regs.push_back(new inst_fetch_buffer_entry());
    }
    //m_name是该寄存器集合的名字。
    m_name = name;
    m_hw_cfg = hw_cfg;
    // this->print();
  }
  //获取该寄存器集合的名字。
  const std::string get_name() { return m_name; }
  //遍历寄存器集合中的所有寄存器，判断是否有寄存器为空。
  bool has_free() {
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid == false) {
        return true;
      }
    }
    return false;
  }
  void release_register_set() {
    std::cout << "DELETE " << m_name << std::endl;
    for (auto ptr : regs) {
      // if (ptr != NULL) {
        delete ptr;
        ptr = NULL;
      // }
    }
    regs.clear();

  }
  //给定一个寄存器id，判断该寄存器是否为空。
  bool has_free(bool sub_core_model, unsigned reg_id) {
    // in subcore model, each sched has a one specific reg to use (based on
    // sched id)
    if (!sub_core_model) return has_free();

    assert(reg_id < regs.size());
    return (regs[reg_id]->m_valid == false);
  }
  //获取一个非空寄存器的id。
  bool has_ready() {
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        return true;
      }
    }
    return false;
  }
  //给定一个寄存器id，判断该寄存器是否非空。
  bool has_ready(bool sub_core_model, unsigned reg_id) {
    if (!sub_core_model) return has_ready();
    assert(reg_id < regs.size());
    return (regs[reg_id]->m_valid);
  }
  //获取一个非空寄存器的id。
  unsigned get_ready_reg_id() {
    // for sub core model we need to figure which reg_id has the ready warp
    // this function should only be called if has_ready() was true
    assert(has_ready());
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    unsigned reg_id;
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        if (ready and (*ready)->uid < regs[i]->uid) {
          // ready is oldest
        } else {
          ready = &regs[i];
          reg_id = i;
        }
      }
    }
    return reg_id;
  }
  unsigned get_schd_id(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return (unsigned)(regs[reg_id]->wid % m_hw_cfg->get_num_sched_per_sm());
  }

  void move_warp_newalloc_src(inst_fetch_buffer_entry *&dest, 
                 inst_fetch_buffer_entry *&src) {
    std::cout << "    src: " 
                         << src->kid << ", " 
                         << src->pc << ", " 
                         << src->wid << ", " 
                         << src->uid << std::endl;
    dest->pc = src->pc;
    dest->wid = src->wid;
    dest->kid = src->kid;
    dest->uid = src->uid;
    dest->m_valid = true;
    // src->clear();

    std::cout << "    dest: " 
                          << dest->kid << ", " 
                          << dest->pc << ", " 
                          << dest->wid << ", " 
                          << dest->uid << std::endl;
    
  }

  void move_warp(inst_fetch_buffer_entry *&dest, 
                 inst_fetch_buffer_entry *&src) {
    dest->pc = src->pc;
    dest->wid = src->wid;
    dest->kid = src->kid;
    dest->uid = src->uid;
    dest->latency = src->latency;
    std::cout << "src->latency: " << src->latency << std::endl;
    dest->m_valid = true;
    std::cout << "    dest: " 
                          << dest->kid << ", " 
                          << dest->pc << ", " 
                          << dest->wid << ", " 
                          << dest->uid << std::endl;
    // src->clear();
    src->m_valid = false;
  }

  //获取一个非空寄存器，并将一条指令存入。
  void move_in(inst_fetch_buffer_entry *&src) {
    inst_fetch_buffer_entry **free = get_free();
    inst_fetch_buffer_entry* tmp = *free;
    move_warp(tmp, src);
  }
  //获取一个空寄存器，并将一条指令存入。
  void move_in(bool sub_core_model, unsigned reg_id, inst_fetch_buffer_entry *&src) {
    std::cout << "  move in: " 
                              << src->pc << ", " 
                              << src->wid << ", " 
                              << src->kid << ", " 
                              << src->uid << std::endl; 
    inst_fetch_buffer_entry *free;
    // std::cout << "sub_core_model: | " << sub_core_model << std::endl;
    if (!sub_core_model) {
      free = *(get_free());
    } else {
      assert(reg_id < regs.size());
      free = get_free_addr(sub_core_model, reg_id);
    }

    if (free != NULL) {
      // std::cout << "get_free_addr(sub_core_model, reg_id): " 
      //           << get_free_addr(sub_core_model, reg_id) << std::endl;
      // std::cout << "free: " << free << std::endl;
      inst_fetch_buffer_entry* tmp = free;
      // std::cout << "tmp: " << tmp << std::endl;
      move_warp_newalloc_src(tmp, src);
    }
  }
  //获取一个非空寄存器，并将其指令移出到dest。
  void move_out_to(inst_fetch_buffer_entry *&dest) {
    inst_fetch_buffer_entry **ready = get_ready();
    move_warp(dest, *ready);
    (*ready)->m_valid = false;
  }
  //依据寄存器编号reg_id，获取一个非空寄存器，并将其指令移出到dest。
  void move_out_to(bool sub_core_model, unsigned reg_id, inst_fetch_buffer_entry *&dest) {
    if (!sub_core_model) {
      return move_out_to(dest);
    }
    inst_fetch_buffer_entry **ready = get_ready(sub_core_model, reg_id);
    std::cout << "    ready: " << ready << std::endl;
    std::cout << "    (*ready): kid, " << (*ready)->kid << std::endl
              << "              pc, " << (*ready)->pc << std::endl
              << "              wid, " << (*ready)->wid << std::endl
              << "              uid, " << (*ready)->uid << std::endl
              << "              latency, " << (*ready)->latency << std::endl;
    assert(ready != NULL);
    move_warp(dest, *ready);
  }
  //获取一个非空寄存器，将其指令移出，并返回这条指令。
  inst_fetch_buffer_entry **get_ready() {
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        if (ready and (*ready)->uid < regs[i]->uid) {
          // ready is oldest
        } else {
          ready = &regs[i];
        }
      }
    }
    return ready;
  }
  inst_fetch_buffer_entry **get_ready(std::vector<inst_fetch_buffer_entry>* except_regs) {
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        if (ready and (*ready)->uid < regs[i]->uid) {
          // ready is oldest
        } else {
          ready = &regs[i];
        }

        if (ready && except_regs != NULL) {
          bool is_except = false;
          for (auto except_reg : *except_regs) {
            if (except_reg.uid == regs[i]->uid && 
                except_reg.wid == regs[i]->wid && 
                except_reg.kid == regs[i]->kid) {
              is_except = true;
              break;
            }
          }
          if (is_except) {
            ready = NULL;
          }
        }
      }
    }
    return ready;
  }
  //获取一个非空寄存器，将其指令移出，并返回这条指令。
  inst_fetch_buffer_entry **get_ready(bool sub_core_model, unsigned reg_id) {
    if (!sub_core_model) return get_ready();
    inst_fetch_buffer_entry **ready;
    ready = NULL;
    assert(reg_id < regs.size());
    if (regs[reg_id]->m_valid) ready = &regs[reg_id];
    // std::cout << "get_ready111: " << ready << std::endl;
    return ready;
  }
  //打印寄存器集合中的所有寄存器。
  void print() const {
    std::cout << "    " << m_name << " : @ " << this << std::endl;
    for (unsigned i = 0; i < regs.size(); i++) {
      std::cout << "     ";
      if (regs[i]->m_valid) {
        std::cout << "    valid: ";
        std::cout << "pc: " << regs[i]->pc << ", wid: " << regs[i]->wid 
                  << ", kid: " << regs[i]->kid << ", uid: " << regs[i]->uid;
      } else {
        std::cout << "    novalid      ";
      }
      std::cout << std::endl;
    }
  }
  //遍历所有寄存器，获取一个空寄存器的id。
  inst_fetch_buffer_entry **get_free() {
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid == false) {
        return &regs[i];
      }
    }
    return NULL;
  }
  //遍历所有寄存器，获取一个空寄存器的地址。
  inst_fetch_buffer_entry **get_free(bool sub_core_model, unsigned reg_id) {
    // in subcore model, each sched has a one specific reg to use (based on
    // sched id)
    if (!sub_core_model) return get_free();

    assert(reg_id < regs.size());
    if (regs[reg_id]->m_valid == false) {
      std::cout << "get free: " << regs[reg_id] << std::endl;
      std::cout << "get free: " << &regs[reg_id] << std::endl;
      return &regs[reg_id];
    }
    return NULL;
  }
  inst_fetch_buffer_entry* get_free_addr(bool sub_core_model, unsigned reg_id) {
    // in subcore model, each sched has a one specific reg to use (based on
    // sched id)
    if (!sub_core_model) return *(get_free());
    std::cout << "  @#@#@#: " << regs[reg_id] << std::endl;
    assert(reg_id < regs.size());
    if (regs[reg_id]->m_valid == false) {
      // std::cout << "get free: | " << regs[reg_id] << std::endl;
      return regs[reg_id];
    }
    return NULL;
  }
  //返回寄存器集合的大小。
  unsigned get_size() { return regs.size(); }
  void print_regs(unsigned reg_id) {
    std::cout << "    pipeline_reg[" << m_name << "] : @ " << this << std::endl;
    std::cout << "     ";
    if (regs[reg_id]->m_valid) {
      std::cout << "    valid: ";
      std::cout << "pc: " << regs[reg_id]->pc << ", wid: " << regs[reg_id]->wid 
                << ", kid: " << regs[reg_id]->kid << ", uid: " << regs[reg_id]->uid;
    } else {
      std::cout << "    novalid      ";
    }
    std::cout << std::endl;
  }
  std::vector<unsigned> get_ready_reg_ids() {
    std::vector<unsigned> ready_reg_ids;
    for (unsigned i = 0; i < regs.size(); i++) {
      if (regs[i]->m_valid) {
        ready_reg_ids.push_back(i);
      }
    }
    return ready_reg_ids;
  }
  unsigned get_kid(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return regs[reg_id]->kid;
  }
  unsigned get_wid(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return regs[reg_id]->wid;
  }
  unsigned get_uid(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return regs[reg_id]->uid;
  }
  unsigned get_pc(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return regs[reg_id]->pc;
  }
  unsigned get_latency(unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    return regs[reg_id]->latency;
  }
  void set_latency(unsigned latency, unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    regs[reg_id]->latency = latency;
  }
  void set_initial_interval(unsigned initial_interval, unsigned reg_id) {
    assert(regs[reg_id]->m_valid);
    regs[reg_id]->initial_interval = initial_interval;
  }

 private:
  //将寄存器集合中的所有寄存器用一个向量保存。
  std::vector<inst_fetch_buffer_entry*> regs;
  // std::vector<inst_fetch_buffer_entry> regs_instance;
  //该寄存器集合的名字。
  std::string m_name;
  hw_config* m_hw_cfg;
};

#endif
