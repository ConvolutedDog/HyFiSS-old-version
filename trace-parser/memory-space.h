
#include "../common/vector_types.h"
#include "../common/common_def.h"

#ifndef MEMORY_SPACE_H
#define MEMORY_SPACE_H

class memory_space_t {
 public:
  //构造函数。初始时，设置存储空间类型为 未定义的空间类型，设置 Bank 数为0。
  memory_space_t();
  //构造函数。设置存储空间类型为 传入参数的类型，设置 Bank 数为0。
  memory_space_t(const enum _memory_space_t &from);
  
  bool operator==(const memory_space_t &x) const;
  bool operator!=(const memory_space_t &x) const;
  bool operator<(const memory_space_t &x) const;
  enum _memory_space_t get_type() const;
  void set_type(enum _memory_space_t t);
  unsigned get_bank() const;
  void set_bank(unsigned b);
  bool is_const() const;
  bool is_local() const;
  bool is_global() const;

 private:
  enum _memory_space_t m_type;
  unsigned m_bank;  // n in ".const[n]"; note .const == .const[0] (see PTX 2.1
                    // manual, sec. 5.1.3)
};

#endif