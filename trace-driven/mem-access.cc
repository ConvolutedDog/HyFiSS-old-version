
#include "mem-access.h"

mem_access_t::mem_access_t(mem_access_type type, new_addr_type address, 
                           unsigned size, bool wr) {
  //mem_access_type定义了在时序模拟器中对不同类型的存储器进行不同的访存类型：
  //    MA_TUP(GLOBAL_ACC_R), 从global memory读
  //    MA_TUP(LOCAL_ACC_R), 从local memory读
  //    MA_TUP(CONST_ACC_R), 从常量缓存读
  //    MA_TUP(TEXTURE_ACC_R), 从纹理缓存读
  //    MA_TUP(GLOBAL_ACC_W), 向global memory写
  //    MA_TUP(LOCAL_ACC_W), 向local memory写
  //在V100中，L1 cache的m_write_policy为WRITE_THROUGH，实际上L1_WRBK_ACC也不会用到：
  //    MA_TUP(L1_WRBK_ACC), L1缓存write back
  //在V100中，当L2 cache写不命中时，采取lazy_fetch_on_read策略，当找到一个cache block
  //逐出时，如果这个cache block是被MODIFIED，则需要将这个cache block写回到下一级存储，
  //因此会产生L2_WRBK_ACC访问，这个访问就是为了写回被逐出的MODIFIED cache block。
  //    MA_TUP(L2_WRBK_ACC), L2缓存write back
  //    MA_TUP(INST_ACC_R), 从指令缓存（I-Cache）读
  //L1_WR_ALLOC_R/L2_WR_ALLOC_R在V100配置中暂时用不到：
  //    MA_TUP(L1_WR_ALLOC_R), L1缓存write-allocate（对cache写不命中，将主存中块调入cache，写入
  //                           该cache块）
  //L1_WR_ALLOC_R/L2_WR_ALLOC_R在V100配置中暂时用不到：
  //    MA_TUP(L2_WR_ALLOC_R), L2缓存write-allocate
  //    MA_TUP(NUM_MEM_ACCESS_TYPE), 存储器访问的类型总数
  m_type = type;
  //访存的地址。
  m_addr = address;
  //访存的数据大小，以字节为单位。
  m_req_size = size;
  //该访存是写/读，1-写，0-读。
  m_write = wr;
}

mem_access_t::mem_access_t(mem_access_type type, new_addr_type address, 
                           unsigned size, bool wr, 
                           const active_mask_t &active_mask,
                           const mem_access_byte_mask_t &byte_mask,
                           const mem_access_sector_mask_t &sector_mask)
    : m_warp_mask(active_mask),
      m_byte_mask(byte_mask),
      m_sector_mask(sector_mask) {
  m_type = type;
  m_addr = address;
  m_req_size = size;
  m_write = wr;
}

void mem_access_t::print(FILE *fp) const {
  fprintf(fp, "addr=0x%llx, %s, size=%u, ", m_addr,
          m_write ? "store" : "load ", m_req_size);
  switch (m_type) {
    case GLOBAL_ACC_R:
      fprintf(fp, "GLOBAL_R");
      break;
    case LOCAL_ACC_R:
      fprintf(fp, "LOCAL_R ");
      break;
    case CONST_ACC_R:
      fprintf(fp, "CONST   ");
      break;
    case TEXTURE_ACC_R:
      fprintf(fp, "TEXTURE ");
      break;
    case GLOBAL_ACC_W:
      fprintf(fp, "GLOBAL_W");
      break;
    case LOCAL_ACC_W:
      fprintf(fp, "LOCAL_W ");
      break;
    case L2_WRBK_ACC:
      fprintf(fp, "L2_WRBK ");
      break;
    case INST_ACC_R:
      fprintf(fp, "INST    ");
      break;
    case L1_WRBK_ACC:
      fprintf(fp, "L1_WRBK ");
      break;
    default:
      fprintf(fp, "unknown ");
      break;
  }
}