
#ifndef ENTRY_H
#define ENTRY_H

struct inst_fetch_buffer_entry {
  inst_fetch_buffer_entry() {
    m_valid = false;
    pc = 0;
    wid = 0;
    kid = 0;
    uid = 0;
    latency = -1;
  };
  inst_fetch_buffer_entry(unsigned _pc, unsigned _wid, unsigned _kid, unsigned _uid) {
    pc = _pc;
    wid = _wid;
    kid = _kid;
    uid = _uid;
    m_valid = true;
    latency = -1;
  };
  void set_latenct(unsigned _latency) {latency = _latency;}
  unsigned pc;
  unsigned wid;
  unsigned kid;
  unsigned uid;
  bool m_valid;
  unsigned latency;
};

struct curr_instn_id_per_warp_entry {
  curr_instn_id_per_warp_entry() {
    kid = 0;
    block_id = 0;
    warp_id = 0;
  };
  curr_instn_id_per_warp_entry(unsigned _kid, unsigned _block_id, unsigned _warp_id) {
    kid = _kid;
    block_id = _block_id;
    warp_id = _warp_id;
  };
  unsigned kid;
  unsigned block_id;
  unsigned warp_id;
};

bool operator<(const curr_instn_id_per_warp_entry& lhs, const curr_instn_id_per_warp_entry& rhs);

#endif