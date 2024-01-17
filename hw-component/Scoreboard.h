
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <vector>
#include <iostream>
#include <algorithm>

#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#define _DEBUG_LOG_ 0

class Scoreboard {
 public:
  //构造函数。
  Scoreboard(const unsigned smid, const unsigned n_warps);
  //发射指令时，其目标寄存器将保留在相应硬件warp的记分牌中。
  void reserveRegisters(const unsigned wid, std::vector<int> regnums, bool is_load);
  //当指令完成写回时，其目标寄存器将被释放。
  void releaseRegisters(const unsigned wid, std::vector<int> regnums);
  void releaseRegisters(const unsigned wid, const int regnum);
  //将单个目标寄存器释放。
  void releaseRegister(const unsigned wid, const int regnum);
  //检测冒险，检测某个指令使用的寄存器是否被保留在记分板中，如果有的话就是发生了 WAW 或 RAW 冒险。
  bool checkCollision(const unsigned wid, std::vector<int> regnums, 
                      int pred, int ar1, int ar2) const;
  //返回记分牌的reg_table中是否有挂起的写入。warp id指向的reg_table为空的话，代表没有挂起的写入，返
  //回false。[挂起的写入]是指wid是否有已发射但尚未完成的指令，将目标寄存器保留在记分牌。
  bool pendingWrites(const unsigned wid) const;
  //打印记分牌的内容。
  void printContents() const;
  
  const bool islongop(const unsigned wid, const int regnum);

  const unsigned regs_size(const unsigned wid) const { return reg_table[wid].size(); }

 private:
  //将单个目标寄存器保留在相应硬件warp的记分牌中。
  void reserveRegister(const unsigned wid, const int regnum);
  //返回SM的ID。
  int get_sid() const { return m_smid; }
  //SM的ID。
  unsigned m_smid;
  
  //下面的声明代码中：
  //    reg_table保留已发射指令中尚未写回的所有目标寄存器。
  //    longopregs保留已发射的内存访问指令中尚未写回的所有目标寄存器。
  
  // keeps track of pending writes to registers
  // indexed by warp id, reg_id => pending write count
  //跟踪除访存指令以外所有的目标寄存器。挂起对寄存器的写入，即如果某条计算指令要写入寄存器 r0，在该条
  //指令发射前，就需要将目标寄存器 r0 加入到 reg_table 中。
  //索引: warp id=>reg_id=>挂起的写入计数。这里，每个warp有自己的一个 std::vector reg_table。换句
  //话说，每个warp有一个记分牌。
  std::vector<std::set<int> > reg_table;

  // Register that depend on a long operation (global, local or tex memory)
  //跟踪存储器访问的目的寄存器。挂起对寄存器的写入，即如果某条访存指令要写入寄存器 r1，在该条指令发射
  //前，就需要将目标寄存器 r1 加入到 longopregs 中。
  //索引: warp id=>reg_id=>挂起的写入计数。这里，每个warp有自己的一个 std::vector longopregs。换
  //句话说，每个warp有一个记分牌。
  std::vector<std::set<int> > longopregs;
};


#endif