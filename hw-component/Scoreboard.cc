

#include "Scoreboard.h"

Scoreboard::Scoreboard(const unsigned smid, const unsigned n_warps)
    : longopregs() {
  m_smid = smid;
  // Initialize size of table
  reg_table.resize(n_warps);
  longopregs.resize(n_warps);
}

void Scoreboard::printContents() const {
  printf("Scoreboard contents (sid=%u): \n", m_smid);
  for (unsigned i = 0; i < reg_table.size(); i++) {
    if (reg_table[i].size() == 0) continue;
    printf("  wid = %2u: ", i);
    std::set<int>::const_iterator it;
    for (it = reg_table[i].begin(); it != reg_table[i].end(); it++)
      printf("R%d ", *it);
    printf("\n");
  }
}

void Scoreboard::reserveRegister(const unsigned wid, const int regnum) {
  if (!(reg_table[wid].find(regnum) == reg_table[wid].end())) {
    printf(
        "Error: trying to reserve an already reserved register (sid=%u, "
        "wid=%u, regnum=%d).",
        m_smid, wid, regnum);
    abort();
  }
  reg_table[wid].insert(regnum);
}

void Scoreboard::releaseRegister(const unsigned wid, const int regnum) {
  if (!(reg_table[wid].find(regnum) != reg_table[wid].end())) return;
  if (regnum != -1)
    reg_table[wid].erase(regnum);
}

const bool Scoreboard::islongop(const unsigned wid, const int regnum) {
  if (regnum == -1) return false;
  else return longopregs[wid].find(regnum) != longopregs[wid].end();
}

/* For pred registers, we reserve [65536 + pred] into reg_table and longopregs. */
void Scoreboard::reserveRegisters(const unsigned wid, std::vector<int> regnums, bool is_load) {
  std::vector<int> prev_regs;
  // if regnums[r] is in prev_regs, we do not add it to scoreboard
  // else we add it to scoreboard, and add it to prev_regs
  for (unsigned r = 0; r < regnums.size(); r++) {
    if (regnums[r] > 0) {
      if (std::find(prev_regs.begin(), prev_regs.end(), regnums[r]) == prev_regs.end()) {
        prev_regs.push_back(regnums[r]);
        std::cout << "  reserve register: " << regnums[r] << std::endl;
        reserveRegister(wid, regnums[r]);
      }
    }
  }

  // Keep track of long operations
  if (is_load) {
    for (unsigned r = 0; r < regnums.size(); r++) {
      if (regnums[r] > 0) {
        longopregs[wid].insert(regnums[r]);
      }
    }
  }
}

void Scoreboard::releaseRegisters(const unsigned wid, std::vector<int> regnums) {
  for (unsigned r = 0; r < regnums.size(); r++) {
    if (regnums[r] > 0) {
      releaseRegister(wid, regnums[r]);
      longopregs[wid].erase(regnums[r]);
    }
  }
}

bool Scoreboard::checkCollision(const unsigned wid, std::vector<int> regnums, 
                                int pred, int ar1, int ar2) const {
  // Get list of all input and output registers
  std::set<int> inst_regs;

  // Add all input/output registers to the regnums vector
  for (unsigned iii = 0; iii < regnums.size(); iii++)
    if (regnums[iii] > 0) inst_regs.insert(regnums[iii]);

  if (pred > 0) inst_regs.insert(pred);
  if (ar1 > 0) inst_regs.insert(ar1);
  if (ar2 > 0) inst_regs.insert(ar2);

  // Check for collision, get the intersection of reserved registers and
  // instruction registers
  std::set<int>::const_iterator it2;
  for (it2 = inst_regs.begin(); it2 != inst_regs.end(); it2++)
    if (reg_table[wid].find(*it2) != reg_table[wid].end()) {
      return true;
    }
  return false;
}

bool Scoreboard::pendingWrites(const unsigned wid) const {
  return !reg_table[wid].empty();
}