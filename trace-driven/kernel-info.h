
#include <cstdio>
#include <vector>
#include <string>

#include "mem-access.h"
#include "../common/vector_types.h"
#include "../trace-parser/trace-parser.h"

#ifndef KERNEL_INFO_H
#define KERNEL_INFO_H

/*
内核函数的信息。kernel_info_t 对象包含GPU网格和块维度、与内核入口点关联的 function_info 对象以
及为内核参数分配的内存。
*/
class kernel_info_t {
 public:
  kernel_info_t(dim3 gridDim, dim3 blockDim);
  ~kernel_info_t(){};

  //返回CUDA代码中的Grid中的所有线程块的总数。
  size_t num_blocks() const {
    return m_grid_dim.x * m_grid_dim.y * m_grid_dim.z;
  }
  //返回每个线程块中的线程数量，threads_per_cta=m_block_dim.x * m_block_dim.y * m_block_dim.z
  size_t threads_per_cta() const {
    return m_block_dim.x * m_block_dim.y * m_block_dim.z;
  }
  //返回CUDA代码中的Grid的三个维度，一个dim3数据类型。
  dim3 get_grid_dim() const { return m_grid_dim; }
  //返回CTA的三个维度，一个dim3数据类型。
  dim3 get_cta_dim() const { return m_block_dim; }

  //返回当前 kernel_info_t 对象的唯一标识号。
  unsigned get_uid() const { return m_uid; }

  //kernel_info_t对象的唯一标识符。
  unsigned m_uid;
  //Grid和Block的维度。
  dim3 m_grid_dim;
  dim3 m_block_dim;
};

class trace_kernel_info_t : public kernel_info_t{
 public:
  trace_kernel_info_t(dim3 gridDim, dim3 blockDim,
                      trace_parser *parser, 
                    //   class trace_config *config,
                      kernel_trace_t *kernel_trace_info);
  ~trace_kernel_info_t(){ delete m_kernel_trace_info; };
  std::vector<std::vector<inst_trace_t> *> get_next_threadblock_traces(std::string kernel_name,
                                                                       unsigned kernel_id,
                                                                       unsigned num_warps_per_thread_block);
  std::vector<mem_instn>& get_one_kernel_one_threadblock_traces(unsigned kernel_id, unsigned block_id);

  unsigned long get_cuda_stream_id() {
    return m_kernel_trace_info->cuda_stream_id;
  }

  kernel_trace_t *get_trace_info() { return m_kernel_trace_info; }

 private:
  const std::unordered_map<std::string, OpcodeChar> *OpcodeMap;
  trace_parser *m_parser;
  kernel_trace_t *m_kernel_trace_info;
};

#endif