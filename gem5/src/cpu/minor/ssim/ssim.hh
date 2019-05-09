#ifndef __SOFTSIM_H__
#define __SOFTSIM_H__

#include <time.h>
#include <cstdint>
#include <iostream>
#include "accel.hh"

//Some utilities:
template<typename T, typename S, typename R>
static void rolling_inc(T& which, S max, R reset) {
  which = ((which+1)==max) ? reset : which + 1; //rolling increment
}


class ssim_t
{
  friend class ticker_t;
  friend class scratch_read_controller_t;
  friend class scratch_write_controller_t;
  friend class network_controller_t;
  friend class dma_controller_t;
  friend class port_port_controller_t;

public:
  //Simulator Interface
  ssim_t(Minor::LSQ* lsq);

  uint64_t roi_enter_cycle() { return _roi_enter_cycle; }

  // Interface from instructions to streams
  // IF SB_TIMING, these just send the commands to the respective controllers
  // ELSE, they carry out all operations that are possible at that point
  void set_context(uint64_t context, uint64_t offset);
  void set_fill_mode(uint64_t mode);
  void req_config(addr_t addr, int size);
  //void load_dma_to_scratch(addr_t mem_addr, uint64_t stride, uint64_t acc_size,
  //    int stretch, uint64_t num_strides, addr_t scratch_addr, uint64_t flags);
  //void write_dma_from_scratch(addr_t scratch_addr, uint64_t stride,
  //    uint64_t access_size, uint64_t num_strides, addr_t mem_addr, uint64_t flags);
  void load_dma_to_port(int repeat_in, int repeat_str);
  void add_port(int in_port);
  void load_scratch_to_port(int repeat_in, int repeat_str);
  void write_scratchpad();
  void write_dma();
  void reroute(int out_port, int in_port, uint64_t num_elem,
               int repeat, int repeat_str,  uint64_t flags, uint64_t access_size);
  void indirect(int ind_port, int ind_type, int in_port, addr_t index_addr,
    uint64_t num_elem, int repeat, int repeat_str, uint64_t offset_list,
    int dtype, uint64_t ind_mult, bool scratch, bool stream, int sstride, int sacc_size, int sn_port);
  void indirect_write(int ind_port, int ind_type, int out_port,
    addr_t index_addr, uint64_t num_elem, uint64_t offset_list,
    int dtype, uint64_t ind_mult, bool scratch);
  bool can_receive(int out_port);
  uint64_t receive(int out_port);
  void write_constant(int num_strides, int in_port,
                      SBDT constant, uint64_t num_elem,
                      SBDT constant2, uint64_t num_elem2,
                      uint64_t flags, int const_width);
  void atomic_update_scratchpad(uint64_t offset, uint64_t iters, int addr_port, int inc_port, int value_type, int output_type, int addr_type, int opcode);
  void multicast_remote_port(uint64_t num_elem, uint64_t mask, int out_port, int rem_port, bool dest_flag, bool spad_type, int64_t stride, int64_t access_size);
  void write_constant_scratchpad(addr_t scratch_addr, uint64_t value, int num_elem, int const_width);

  void push_in_accel_port(int accel_id, uint8_t* val, int num_bytes, int in_port);
  void push_atomic_update_req(int scr_addr, int opcode, int val_bytes, int out_bytes, uint64_t inc);
  void write_remote_banked_scratchpad(uint8_t* val, int num_bytes, uint16_t scr_addr);

  void insert_barrier(uint64_t mask);
  
  void print_stats();
  uint64_t forward_progress_cycle();
  void forward_progress(uint64_t c) {_global_progress_cycle=c;}
  bool can_add_stream();
  void add_bitmask_stream(base_stream_t* s);
  void add_bitmask_stream(base_stream_t* s, uint64_t context);
  bool done(bool show, int mask);
  bool is_in_config();

  // do not want to stall the control core
  void insert_df_barrier(int64_t num_scr_wr, bool spad_type);

  void set_in_use() {
    if(!_in_use) {
      if(SS_DEBUG::COMMAND || SS_DEBUG::ROI) {
        timestamp();
        std::cout << "SSIM in use\n";
      }
    }
    _in_use=true;
  }
  void set_not_in_use() {
    if(SS_DEBUG::COMMAND || SS_DEBUG::ROI) {
      timestamp();
      std::cout << "SSIM *NOT* in use\n";
    }
    _in_use=false;
  }
  bool in_use() {return _in_use;}

  uint64_t now();

  uint64_t elpased_time_in_roi() {return _elapsed_time_in_roi;}

  bool in_roi() {return _in_roi;}
  void roi_entry(bool enter);

  /* To get prepared to enter ROI. */
  void setup_stat_cycle();

  /* After entering the ROI, updates the status of starting and ending cycle so that
   * we can ignore those "white bubbles". */
  void update_stat_cycle();

  /* If it does not use any SB code, it will do normal ROI statistics.
   * O.W it ignores those "white bubbles". */
  void cleanup_stat_cycle();

  void timestamp(); //print timestamp
  void timestamp_index(int i); //print timestamp
  void timestamp_context(); //print timestamp

  void step();
  void cycle_shared_busses();


  Minor::MinorDynInstPtr cur_minst() {return _cur_minst;}

  void set_cur_minst(Minor::MinorDynInstPtr m) {
    assert(m);
    _cur_minst=m;
  }

  void issued_inst() {
    if(in_roi()) {
      _control_core_insts++;
    }
  }

  // due to stream seq mismatch?
  void issued_discarded_inst() {
    if(in_roi()) {
      _control_core_discarded_insts++;
    }
  }
  /*
  void issued_bubble_inst() {
    if(in_roi()) {
      _control_core_bubble_insts++;
    }
  }
  */

  void wait_inst(uint64_t mask) {
    if(in_roi()) {
      _wait_map[mask]++;
    }
  }
  void wait_config() {
    if(in_roi()) {
      _config_waits++;
    }
  }
  
  // for global barrier case
  void set_num_active_threads(int num_threads) {
    _num_active_threads = num_threads;
  }

  static bool stall_core(uint64_t mask) {
    // std::cout << "Came in stall core with mask: " << mask << std::endl;
    return (mask==0) || (mask&WAIT_CMP) ||
      (mask&WAIT_MEM_WR) || (mask&WAIT_SCR_ATOMIC) || (mask&GLOBAL_WAIT) || (mask&STREAM_WAIT);
  }

  uint64_t roi_cycles() {return _roi_cycles;}
  uint64_t control_core_insts() {return _control_core_insts;}
  /*uint64_t control_core_bubble_insts() {return _control_core_bubble_insts;}*/
  uint64_t control_core_discarded_insts() {return _control_core_discarded_insts;}
  uint64_t config_waits() {return _config_waits;}

  accel_t* get_acc(int i) {
    assert(i>=0 && i<NUM_ACCEL_TOTAL);
    return accel_arr[i];
  }

  accel_t* shared_acc() { return accel_arr[SHARED_SP]; }

  //bool can_push_shs_buf(int size, uint64_t addr, uint64_t bitmask) {
  //  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
  //    if(bitmask & b) {
  //      if(!accel_arr[i]->scr_w_c()->_buf_shs_write.can_push_addr(size,addr)) {
  //        return false;
  //      }
  //    }
  //  }
  //  return true;
  //}
  
  int get_core_id() {
    return _lsq->getCpuId();
  }

  bool debug_pred() {
    if(_req_core_id==-1) return true;
    else return (_req_core_id==get_core_id());
  }

  int num_active_threads() {
    return _num_active_threads;
  }
   
  bool printed_this_before() { return _printed_this_before; }
  void set_printed_this_before(bool f) { _printed_this_before=f; }

  void pushStreamDimension(uint64_t a, uint64_t b, uint64_t c) {
    stream_stack.push_back(a);
    stream_stack.push_back(b);
    stream_stack.push_back(c);
  }

private:

  int _req_core_id=-1;
  bool _printed_this_before=false;
  unsigned _which_shr=0;

  Minor::MinorDynInstPtr _cur_minst;
  uint64_t _context_offset=0; //no offset between addresses
  uint64_t _context_bitmask=1; //core 1 active
  uint64_t _ever_used_bitmask=1; //bitmask if core ever used

  uint64_t _fill_mode=0; //fill mode (default 0, no fill)

  Minor::LSQ* _lsq;
  // Minor::Execute* _execute; (not sure if we need it)

  accel_t* accel_arr[NUM_ACCEL+1]; //LAST ONE IS SHARED SCRATCH

  int _num_active_threads=4; // for global barrier

  bool _prev_done = true;

  bool _in_use=false;

  bool debug=true;

  //Statistics Members
  bool _in_roi = false;
  timespec _start_ts, _stop_ts;
  uint64_t _elapsed_time_in_roi=0;
  uint64_t _times_roi_entered=0;
  uint64_t _orig_stat_start_cycle = 0;
  uint64_t _stat_start_cycle = 0;
  uint64_t _roi_enter_cycle = 0;
  uint64_t _stat_stop_cycle = 0;
  uint64_t _roi_cycles=0;

  uint64_t _control_core_insts=0;
  // uint64_t _control_core_bubble_insts=0;
  uint64_t _control_core_discarded_insts=0;

  uint64_t _config_waits=0;
  std::unordered_map<uint64_t,uint64_t> _wait_map;

  std::vector<uint64_t> stream_stack;
  std::vector<int> extra_in_ports;

  uint64_t _global_progress_cycle=0;
};


#endif
