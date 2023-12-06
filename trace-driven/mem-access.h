
#include "../common/common_def.h"

#ifndef MEM_ACCESS_H
#define MEM_ACCESS_H

class mem_access_t {
 public:
  //构造函数。mem_access_t类有一个私有的 gpgpu_context *gpgpu_ctx 对象，初始化该对象。
  mem_access_t() {}
  //构造函数。
  //new_addr_type定义：typedef unsigned long long new_addr_type;
  mem_access_t(mem_access_type type, new_addr_type address, 
               unsigned size, bool wr);
  //构造函数。
  //active_mask_t 活跃掩码定义：
  //    typedef std::bitset<MAX_WARP_SIZE> active_mask_t; 
  //用于在处理一个warp内的线程分支，标记每个线程是否执行某个分支。
  //mem_access_byte_mask_t 访存数据字节掩码定义：
  //    typedef std::bitset<MAX_MEMORY_ACCESS_SIZE> mem_access_byte_mask_t;
  //用于标记一次访存操作中的数据字节掩码，MAX_MEMORY_ACCESS_SIZE设置为128，即每次访存最大数据128字节。
  //mem_access_sector_mask_t 扇区掩码定义：
  //    typedef std::bitset<SECTOR_CHUNCK_SIZE> mem_access_sector_mask_t;
  //用于标记一次访存操作中的扇区掩码，4个扇区，每个扇区32个字节数据。
  mem_access_t(mem_access_type type, new_addr_type address, unsigned size,
               bool wr, const active_mask_t &active_mask,
               const mem_access_byte_mask_t &byte_mask,
               const mem_access_sector_mask_t &sector_mask);
  //返回访存地址。
  new_addr_type get_addr() const { return m_addr; }
  //设置访存地址。
  void set_addr(new_addr_type addr) { m_addr = addr; }
  //返回访存数据大小，以字节为单位。
  unsigned get_size() const { return m_req_size; }
  //返回访存的线程活跃掩码。
  const active_mask_t &get_warp_mask() const { return m_warp_mask; }
  //返回该访存是写/读，1-写，0-读。
  bool is_write() const { return m_write; }
  //返回对存储器进行的访存类型，见构造函数注释。
  enum mem_access_type get_type() const { return m_type; }
  //返回访存的数据字节掩码。
  mem_access_byte_mask_t get_byte_mask() const { return m_byte_mask; }
  //返回访存的扇区掩码。
  mem_access_sector_mask_t get_sector_mask() const { return m_sector_mask; }

  //将访存的 地址、store或load、数据大小、访存类型打印到文件。
  void print(FILE *fp) const;

 private:
  //该次访存操作的唯一ID。
  unsigned m_uid;
  //访存地址。
  new_addr_type m_addr;  // request address
  //该访存是写/读，1-写，0-读。
  bool m_write;
  //访存数据大小，以字节为单位。
  unsigned m_req_size;  // bytes
  //对不同类型的存储器进行的访存类型，见构造函数注释。
  mem_access_type m_type;
  //访存的线程活跃掩码。
  active_mask_t m_warp_mask;
  //访存的数据字节掩码。
  mem_access_byte_mask_t m_byte_mask;
  //访存的扇区掩码。
  mem_access_sector_mask_t m_sector_mask;
};

#endif