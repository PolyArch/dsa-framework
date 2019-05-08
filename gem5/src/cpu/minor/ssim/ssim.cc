#include <unordered_set>
#include <utility>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <assert.h>

#include "ssim.hh"
#include "cpu/minor/cpu.hh"

extern "C" void libsbsim_is_present() {}

using namespace std;

// Vector-Stream Commands (all of these are context-dependent)

ssim_t::ssim_t(Minor::LSQ* lsq) : _lsq(lsq) {
  const char *req_core_id_str = std::getenv("DBG_CORE_ID");
  if (req_core_id_str != nullptr) {
    _req_core_id = atoi(req_core_id_str);
  }
  // cout << "DEBUG PRED FOR CORE " << get_core_id() << ": " << debug_pred() << endl;
  SS_DEBUG::check_env(debug_pred());

  for(int i = 0; i < NUM_ACCEL_TOTAL; ++i) {
    accel_arr[i] = new accel_t(lsq, i, this);
  }
  //accel_arr[SHARED_SP] = new accel_t(lsq, SHARED_SP, this);
  //TODO: inform accel_arr

  // set indirect ports to be 1-byte by default
  // for(int i=22; i<32; ++i) {
  for(int i=START_IND_PORTS; i<STOP_IND_PORTS; ++i) {
    port_data_t& cur_out_port = accel_arr[0]->_port_interf.out_port(i);
    cur_out_port.set_port_width(8);
    
    port_data_t& cur_in_port = accel_arr[0]->_port_interf.in_port(i);
    cur_in_port.set_port_width(8);
  }
}
void ssim_t::req_config(addr_t addr, int size) {
  if(addr==0 && size==0) {
    if(SS_DEBUG::COMMAND_I) {
      std::cout << "Complete reset request issued\n";
    }
    //This is the reset_data case!
    accel_arr[0]->request_reset_data();
    /*for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
      if(_context_bitmask & b) {
        accel_arr[i]->request_reset_data();
      }
    }*/
    return;
  }

  // reset stream case here... (only accel[0])
  if(addr==0 && size==1) {
    // accel_arr[0]->request_reset_streams();
    if(SS_DEBUG::COMMAND_I) {
      std::cout << "RESET stream request triggered\n";
    }
    accel_arr[0]->switch_stream_cleanup_mode_on();
    return;
  }

  // cout << "Release cofiguration request\n";

  set_in_use();  //lets get going then..


  //Send req_config to master accel -- TODO: is this the right decision?
  shared_acc()->req_config(addr,size,_context_bitmask);

  //We're also going to want to let these cores know they're in_config
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_context_bitmask & b) {
      accel_arr[i]->set_in_config();
    }
  }
}

// receive network message at the given input port id
// void ssim_t::push_in_accel_port(int accel_id, int64_t val, int in_port) {
void ssim_t::push_in_accel_port(int accel_id, uint8_t* val, int num_bytes, int in_port) {
  assert(accel_id<NUM_ACCEL_TOTAL);
  accel_arr[accel_id]->receive_message(val, num_bytes, in_port);
}

// SPU has only 1 DGRA in a core
void ssim_t::push_atomic_update_req(int scr_addr, int opcode, int val_bytes, int out_bytes, uint64_t inc) {
  accel_arr[0]->push_atomic_update_req(scr_addr, opcode, val_bytes, out_bytes, inc);
}

bool ssim_t::can_add_stream() {
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i,b<<=1) {
    if(_context_bitmask & b) {
      if(!accel_arr[i]->can_add_stream()) {
        return false;
      }
    }
  }
  return true;
}

bool ssim_t::done(bool show, int mask) {
  bool is_done=true;

  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_context_bitmask & b) {
      is_done &= accel_arr[i]->done(show,mask);
    }
  }
  return is_done;
}

bool ssim_t::is_in_config() {
  bool in_config=true;

  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_context_bitmask & b) {
      in_config &= accel_arr[i]->is_in_config();
    }
  }
  return in_config;
}

void ssim_t::cycle_shared_busses() {
  //bring data int o
  //     auto& read_buf = accel_arr[i]->_buf_shs_read;
  //int max_reqs=2;
  //int reqs_made=0;

  //for(int i = 0; (i < NUM_ACCEL) && (reqs_made < max_reqs); ++i) {
  //  rolling_inc(_which_shr,NUM_ACCEL,0);
  //  auto& read_buf = accel_arr[i]->_scr_r_c._buf_shs_read;
  //  bool didit = shared_acc()->_scr_w_c.accept_buffer(read_buf);
  //  if(didit) {
  //    reqs_made+=1;
  //  }
  //}
}

void ssim_t::step() {
  if(!_in_use) {
    return;
  }
  cycle_shared_busses();
  for(uint64_t i=0,b=1; i < NUM_ACCEL; ++i, b<<=1) {
    if(_ever_used_bitmask & b) {
      accel_arr[i]->tick();
    }
  }
  shared_acc()->tick();
}

void ssim_t::print_stats() {
   auto& out = cout;
   out.precision(4);
   out << dec;

   out << "\n*** ROI STATISTICS for CORE ID: " << _lsq->getCpuId() << " ***\n";
   out << "Simulator Time: " << ((double)elpased_time_in_roi())
                                    /1000000000 << " sec\n";

   out << "Cycles: " << roi_cycles() << "\n";
   out << "Control Core Insts Issued: " << control_core_insts() << "\n";
   out << "Control Core Discarded Insts Issued: " << control_core_discarded_insts() << "\n";
   out << "Control Core Discarded Inst Stalls: " << ((double)control_core_discarded_insts()/(double)control_core_insts()) << "\n";
   // out << "Control Core Bubble Insts Issued: " << control_core_bubble_insts() << "\n";
   out << "Control Core Config Stalls: "
       << ((double)config_waits()/roi_cycles()) << "\n";
   out << "Control Core Wait Stalls:   ";
   for(auto& i : _wait_map) {
     out << ((double)i.second/roi_cycles()) << " (";
     uint64_t mask = i.first;
     if(mask == 0) {
       out << "ALL";
     }
     if(mask & WAIT_SCR_RD) {
       out << "SCR_RD";
     }
     if(mask & WAIT_SCR_WR) {
       out << "SCR_WR";
     }
     if(mask & GLOBAL_WAIT) {
       out << "GLOBAL_WAIT";
     }
     if(mask & STREAM_WAIT) {
       out << "STREAM_WAIT";
     }
     out << ")  ";
   }
   out << "\n";

  int cores_used=0;
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_ever_used_bitmask & b) {
      accel_arr[i]->print_stats();
      if(i!=NUM_ACCEL_TOTAL) {cores_used++;}
    }
  }

  cout << "\nCores used: " << cores_used << "bitmask: "
       << std::hex << _ever_used_bitmask << std::dec << "\n";

  uint64_t total_mem_accesses=0;
  uint64_t max_comp_instances=0;
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_ever_used_bitmask & b) {
      total_mem_accesses+=
        accel_arr[i]->_bw_map[make_pair(LOC::DMA,LOC::TOTAL)].first;
      max_comp_instances=std::max(max_comp_instances,
         accel_arr[i]->_stat_comp_instances);
    }
  }

  cout << "Total Memory Activity: " << (double) total_mem_accesses
                                     / (double) roi_cycles() << "\n";

  cout << "\n -- Parallelization Estimates --\n";

  out << "Multi-Pipeline Cycles (cores:cycles): ";
  for(int i = 1; i * cores_used <= 18; ++i) { // estimate performance
    cout << i*cores_used << ":";
    uint64_t multicore_cyc = roi_cycles() / i;
    uint64_t mem_cyc = total_mem_accesses;
    out << std::max(multicore_cyc,mem_cyc) << " ";
  }
  out << "\n";

  out << "Multi-Pipeline Activity: (cores:cycles)";
  for(int i = 1; i * cores_used <= 18; ++i) { // estimate performance
    cout << i*cores_used << ":";
    uint64_t multicore_cyc = roi_cycles() / i;
    uint64_t mem_cyc = total_mem_accesses;
    uint64_t cycles = std::max(multicore_cyc,mem_cyc);
    double active_ratio = ((double)(max_comp_instances)/i)/((double)cycles);
    out << active_ratio << " ";
  }
  out << "\n\n";


}

uint64_t ssim_t::forward_progress_cycle() {
  uint64_t r = _global_progress_cycle;
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_ever_used_bitmask & b) {
      r = std::max(accel_arr[i]->forward_progress_cycle(),r);
    }
  }
  return r;
}

void ssim_t::add_bitmask_stream(base_stream_t* s, uint64_t ctx) {
  //patch with implicit stuff
  s->set_fill_mode(_fill_mode);
  s->set_id();
  s->set_context_offset(_context_offset);
  for(int in_port : extra_in_ports) {
    assert(s->in_ports().size()); //there should be some already...
    s->in_ports().push_back(in_port);
  }

  //Check if not active!
  if(debug && (SS_DEBUG::COMMAND)  ) {
    timestamp_context();
    cout << "id:" << s->id() << " ";
    s->print_status();
  }

  if(!s->stream_active()) {
    if(debug && (SS_DEBUG::COMMAND)  ) {
      timestamp_context();
      cout << " ---    and this stream is being deleted for being inactive!\n";
    }
    delete s;
    return;
  }

  //if(debug && (SS_DEBUG::COMMAND)  ) {
  //  cout << "Sending to Cores: ";
  //}

  shared_ptr<base_stream_t> s_shr(s);

  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(ctx & b) {
      //if(debug && (SS_DEBUG::COMMAND)  ) {
      //  cout << i;
      //}
      if(s->in_ports().size()!=0) {
        auto& in_vp = accel_arr[i]->port_interf().in_port(s->first_in_port());
        s->set_data_width(in_vp.get_port_width()); // added for dgra
      } else if (s->out_port() != -1) {
        auto& out_vp = accel_arr[i]->port_interf().out_port(s->out_port());
        s->set_data_width(out_vp.get_port_width()); // added for dgra
      } else {
        s->set_data_width(8);
      }

      accel_arr[i]->add_stream(s_shr);
    }
  }

  //if(debug && (SS_DEBUG::COMMAND)  ) {
  //  cout << "\n";
  //}

  extra_in_ports.clear(); //reset this
}

void ssim_t::add_bitmask_stream(base_stream_t* s) {
  add_bitmask_stream(s,_context_bitmask);
}


// ------------------------Time Stuffs-------------------------------------
void ssim_t::timestamp() {
  cout << std::dec << now() - _orig_stat_start_cycle << "\t";
}

void ssim_t::timestamp_index(int i) {
  timestamp();
  cout << "context " << i << " ";
}

void ssim_t::timestamp_context() {
  timestamp();
  int count = __builtin_popcount(_context_bitmask);

  cout << "context ";

  if(count < 8) {
    for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
      if(_context_bitmask & b) {
        cout << i <<" ";
      }
    }
    cout << "\t";

  } else {
    cout << "context:0x" << std::hex << _context_bitmask << "\t" << std::dec;
  }
}


// ----------------------- STREAM COMMANDS ------------------------------------
void ssim_t::set_context(uint64_t context, uint64_t offset) {
  if(debug && (SS_DEBUG::CONTEXT)  ) {
    cout << "Set Context: " << std::hex << context << std::dec << "\n";
  }

  _context_offset = offset;
  _context_bitmask = context;
  _ever_used_bitmask |= context;
}

void ssim_t::set_fill_mode(uint64_t mode) {
  if(debug && (SS_DEBUG::CONTEXT)  ) {
    cout << "Set FILL MODE: " << std::hex << mode << std::dec << "\n";
  }

  _fill_mode = mode;
}

//void ssim_t::load_dma_to_scratch(addr_t mem_addr, uint64_t stride,
//    uint64_t access_size, int stretch, uint64_t num_strides,
//    addr_t scratch_addr, uint64_t flags) {
//
//
//  if(flags) {
//    auto* S = new scr_scr_stream_t(mem_addr,stride, access_size, stretch,
//               num_strides, scratch_addr,true);
//    auto* R = new scr_scr_stream_t(mem_addr,stride, access_size, stretch,
//               num_strides, scratch_addr, false);
//    S->set_remote(R, _context_bitmask); // tie together <3
//    R->set_remote(S, SHARED_MASK);
//
//    add_bitmask_stream(S, SHARED_MASK); // send scratch load to shared accel
//    add_bitmask_stream(R);
//
//  } else {
//    auto* s = new dma_scr_stream_t(mem_addr,stride, access_size, stretch,
//               num_strides, scratch_addr);
//    add_bitmask_stream(s);  //load dma to shared proc
//  }
//}

//void ssim_t::write_dma_from_scratch(addr_t scratch_addr, uint64_t stride,
//      uint64_t access_size, uint64_t num_strides, addr_t mem_addr, uint64_t flags) {
//
//  if(flags) {
//    auto* S = new scr_scr_stream_t(scratch_addr, stride, access_size, 0,
//               num_strides, mem_addr,true);
//    auto* R = new scr_scr_stream_t(scratch_addr, stride, access_size, 0,
//               num_strides, mem_addr, false);
//    S->set_remote(R, SHARED_MASK); // tie together <3
//    R->set_remote(S, _context_bitmask);
//
//    add_bitmask_stream(S); // send scratch load to shared accel
//    add_bitmask_stream(R, SHARED_MASK);
//  } else {scr_dma_stream_t* s = new scr_dma_stream_t(scratch_addr,stride,access_size,
//      num_strides,mem_addr);
//    add_bitmask_stream(s);
//  }
//}

void ssim_t::add_port(int in_port) {
  extra_in_ports.push_back(in_port);
}

void ssim_t::load_dma_to_port(int repeat, int repeat_str) {
  int new_repeat_str = repeat_str >> 1;
  bool repeat_flag = repeat_str & 1;

  affine_read_stream_t* s = new affine_read_stream_t(LOC::DMA, stream_stack,
                                                     {(int) stream_stack.back()},
                                                     repeat, new_repeat_str);
  s->_repeat_flag = repeat_flag;
  // set the cur_repeat_lim to -1
  if(repeat_flag) {
    auto &prt = accel_arr[0]->port_interf().out_port(repeat);
    prt.set_cur_repeat_lim(-1);
  }

  add_bitmask_stream(s);
  stream_stack.clear();
}

void ssim_t::write_dma() {

  affine_write_stream_t* s = new affine_write_stream_t(LOC::DMA, stream_stack);

  add_bitmask_stream(s);
  stream_stack.clear();
}


void ssim_t::load_scratch_to_port(int repeat, int repeat_str) {//, bool repeat_flag) {
  int new_repeat_str = repeat_str >> 1;
  bool repeat_flag = repeat_str & 1;

  affine_read_stream_t* s = new affine_read_stream_t(LOC::SCR, stream_stack,
                                                     {(int) stream_stack.back()},
                                                     repeat, new_repeat_str);
  s->_repeat_flag = repeat_flag;
  if(repeat_flag) {
    auto &prt = accel_arr[0]->port_interf().out_port(repeat);
    prt.set_cur_repeat_lim(-1);
  }
      
  add_bitmask_stream(s);
  stream_stack.clear();

}

void ssim_t::write_scratchpad() {

  affine_write_stream_t* s = new affine_write_stream_t(LOC::SCR, stream_stack);

  add_bitmask_stream(s);
  stream_stack.clear();
}

// it is not a stream; just a linear write (just push data into buf?)
void ssim_t::write_remote_banked_scratchpad(uint8_t* val, int num_bytes, uint16_t scr_addr) {
  // TODO: add a check for full buffer to apply backpressure to network
  accel_arr[0]->push_scratch_remote_buf(val, num_bytes, scr_addr); // hopefully, we use single accel per CC
}

// command decode for atomic stream update
void ssim_t::atomic_update_scratchpad(uint64_t offset, uint64_t iters, int addr_port, int inc_port, int value_type, int output_type, int addr_type, int opcode) {
    atomic_scr_stream_t* s = new atomic_scr_stream_t();
    s->_mem_addr = offset;
    s->_num_strides = iters;
    s->_out_port = addr_port;
    s->_val_port = inc_port;
    s->_op_code = opcode;
    s->_value_type=value_type;
    s->_output_type=output_type;
    s->_addr_type=addr_type;
    s->_unit=LOC::SCR;
    s->set_orig();

    add_bitmask_stream(s);
}

// TODO: make it neater
void ssim_t::multicast_remote_port(uint64_t num_elem, uint64_t mask, int out_port, int rem_port, bool dest_flag, bool spad_type, int64_t stride, int64_t access_size) {
    // remote_port_multicast_stream_t* s = NULL;
    // 0 means port->port stream
    if (dest_flag==0) {
      remote_port_multicast_stream_t* s = new remote_port_multicast_stream_t();
      s->_core_mask = mask;
      s->_num_elements = num_elem;
      s->_out_port = out_port;
      s->_remote_port = rem_port;
      // s->_unit=LOC::SCR; (probably add a flag for the destination to save a new opcode)
      s->set_orig();
        if(SS_DEBUG::NET_REQ){
          printf("Remote stream initialized");
          s->print_status();
        }
       add_bitmask_stream(s);
      } else {
        if(rem_port!=0) { // I hope it can never be 0
          remote_scr_stream_t* s = new remote_scr_stream_t();
          s->_num_elements = num_elem;
          s->_core_mask = -1;
          s->_out_port = out_port;
          s->_remote_port = -1;
          s->_addr_port = rem_port;
          s->_remote_scr_base_addr = mask; // this would now be scratch_base_addr
          s->_scr_type = spad_type;
          s->set_orig();
          add_bitmask_stream(s);
        } else { // inherited by the affine_base_stream
          direct_remote_scr_stream_t* s = new direct_remote_scr_stream_t(mask, access_size, stride);
          s->_scr_type = 0; // spad_type; // this should not be required now?
          s->_num_elements = num_elem; // this is num_strides
          s->_out_port = out_port;
          s->set_orig();
          add_bitmask_stream(s);
        }
    }
}

void ssim_t::write_constant_scratchpad(addr_t scratch_addr, uint64_t value, int num_elem, int const_width) {
    const_scr_stream_t* s = new const_scr_stream_t();
    s->_num_elements = num_elem;
    s->_constant = value;
    s->_const_width = 2; // FIXME: IMP: not reading correctly:const_width;
    s->_scratch_addr = scratch_addr-2; // just for logic later
    s->set_orig();

    add_bitmask_stream(s);
}

//The reroute function handles either local recurrence, or remote data transfer
void ssim_t::reroute(int out_port, int in_port, uint64_t num_elem,
                     int repeat, int repeat_str, uint64_t flags,
                     uint64_t padding_iter) {
  base_stream_t* s=NULL, *r=NULL;
  
  // specific to recurrence stream, FIXME: the accel_arr[0] thing
  auto& out_vp = accel_arr[0]->port_interf().out_port(out_port);
  int src_data_width = out_vp.get_port_width();
  int core_d = ((flags & 3) == 1) ? -1 : 1;
  padding_iter = (flags >> 2) ? padding_iter : NO_PADDING;

  int new_repeat_str = repeat_str >> 1;
  bool repeat_flag = repeat_str & 1;

  if((flags & 3) == 0) {

    s = new port_port_stream_t(out_port,in_port,num_elem,repeat,new_repeat_str,
                               src_data_width, padding_iter, repeat_flag);
    // set the cur_repeat_lim of output port to -1 (this is opp to others)
    if(repeat_flag) {
      auto &prt = accel_arr[0]->port_interf().out_port(repeat);
      prt.set_cur_repeat_lim(-1);
    }
  } else {
    auto S = new remote_port_stream_t(out_port,in_port,num_elem,
        repeat,new_repeat_str,core_d,true, padding_iter, repeat_flag);
    auto R = new remote_port_stream_t(out_port,in_port,num_elem,
        repeat,new_repeat_str,core_d,false, padding_iter, repeat_flag);
    S->_remote_stream=R; // tie together <3
    R->_remote_stream=S;
    s=S;
    r=R;
  }

  add_bitmask_stream(s);

  if(r) {
    uint64_t new_ctx=0;
    if(core_d==1) {
      new_ctx = ((_context_bitmask<<1) & ACCEL_MASK) |
                (_context_bitmask>>(NUM_ACCEL-1) & 1);
    } else if(core_d==-1) {
      new_ctx = ((_context_bitmask>>1) & ACCEL_MASK) |
                ((_context_bitmask&1)<<(NUM_ACCEL-1));
    } else {
      assert(0 && "how do i do this?");
    }
    add_bitmask_stream(r, new_ctx);
  }


}

//Configure an indirect stream with params
void ssim_t::indirect(int ind_port, int ind_type, int in_port, addr_t index_addr,
    uint64_t num_elem, int repeat, int repeat_str,uint64_t offset_list,
    int dtype, uint64_t ind_mult, bool scratch, bool stream, int sstride, int sacc_size, int sn_port) {
  indirect_stream_t* s = new indirect_stream_t();
  s->_ind_port=ind_port;
  s->_ind_type=ind_type;
  s->add_in_port(in_port);;
  s->_index_addr=index_addr;
  s->_num_elements=num_elem;
  s->_repeat_in=repeat;
  s->_repeat_str=repeat_str;
  s->_offset_list=offset_list;
  s->_dtype=dtype;
  s->_ind_mult=ind_mult;
  s->_is_2d_stream=stream;
  if(stream) { // set sub-stream parameters here (read from ss_stride)
    s->_sstride = sstride;
    s->_sacc_size = sacc_size;
    s->_sn_port = sn_port;
  }
  if(scratch) s->_unit=LOC::SCR;
  else s->_unit=LOC::DMA;
  s->set_orig();

  add_bitmask_stream(s);
}

//Configure an indirect stream with params
void ssim_t::indirect_write(int ind_port, int ind_type, int out_port,
    addr_t index_addr, uint64_t num_elem, uint64_t offset_list,
    int dtype, uint64_t ind_mult, bool scratch) {
  indirect_wr_stream_t* s = new indirect_wr_stream_t();
  s->_ind_port=ind_port;
  s->_ind_type=ind_type;
  s->_out_port=out_port; //comes from sb_dma_addr
  s->_index_addr=index_addr;
  s->_num_elements=num_elem;
  s->_dtype=dtype;
  s->_ind_mult=ind_mult;
  if(scratch) s->_unit=LOC::SCR;
  else s->_unit=LOC::DMA;
  s->set_orig();

  add_bitmask_stream(s);
}

// -------------------------------------------------------------------------------

//These two functions just return the first core from 0
bool ssim_t::can_receive(int out_port) {
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_context_bitmask & b) {
      return accel_arr[i]->can_receive(out_port);
    }
  }
  return false;
  assert(0 && "has to be an accel active");
}

uint64_t ssim_t::receive(int out_port) {
  for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    if(_context_bitmask & b) {
      return accel_arr[i]->receive(out_port);
    }
  }
  assert(0 && "has to be an accel active");
  return 0;
}


void ssim_t::insert_barrier(uint64_t mask) {
   stream_barrier_t* s = new stream_barrier_t();
   s->_mask = mask;
   s->set_orig();

  add_bitmask_stream(s);
}

// TODO:FIXME: see if we need new mask
void ssim_t::insert_df_barrier(int64_t num_scr_wr, bool spad_type) {
  stream_barrier_t* s = new stream_barrier_t();
  s->_mask = 64;
  // s->_num_remote_writes = num_scr_wr;
  s->_scr_type = spad_type; // TODO: Where to use this?
  s->set_orig();
  // printf("df count read from register is: %ld\n",num_scr_wr);
  accel_arr[0]->_scr_w_c.set_df_count(num_scr_wr);
  add_bitmask_stream(s);
}

void ssim_t::write_constant(int num_strides, int in_port,
                    SBDT constant, uint64_t num_elem,
                    SBDT constant2, uint64_t num_elem2,
                    uint64_t flags, int const_width) {

  const_port_stream_t* s = new const_port_stream_t();
  s->add_in_port(in_port);

  if((flags&1)==0) {
    s->_constant=0;
    s->_num_elements=0;
    s->_num_iters=1;
  } else {
    s->_constant=constant2;
    s->_num_elements=num_elem2;
    s->_num_iters=num_strides;
  }

  s->_constant2=constant;
  s->_num_elements2=num_elem;

  // data-width according to port should be set earlier?
  s->set_orig();
  add_bitmask_stream(s);


  // std::cout << "Const width: " << const_width << std::endl;

  if(const_width<4 && const_width >0) { // doesn't for T64 right now -- todo
    switch(const_width) {
      case 0: s->_const_width=8;
              break;
      case 1: s->_const_width=4;
              break;
      case 2: s->_const_width=2;
              break;
      case 3: s->_const_width=1;
              break;
    }
  } else { // assume the width of the corresponding input port
    port_data_t& cur_in_port = accel_arr[0]->_port_interf.out_port(in_port);
    s->_const_width = cur_in_port.get_port_width();
  }
  // std::cout << "Final const width: " << s->_const_width << std::endl;

}

uint64_t ssim_t::now() {
  return _lsq->get_cpu().curCycle();
}

void ssim_t::update_stat_cycle() {
  assert(in_roi());
  if (_stat_start_cycle == ~0ull) {
    _stat_start_cycle = now();
  } else {
    _stat_stop_cycle = now();
  }
}

void ssim_t::setup_stat_cycle() {
  _stat_start_cycle = ~0ull;
  _roi_enter_cycle = now();
}

void ssim_t::cleanup_stat_cycle() {
  if (_stat_start_cycle == ~0ull) {
    _stat_start_cycle = _roi_enter_cycle;
    _stat_stop_cycle = now();
  }
}

// ------------------------- TIMING ---------------------------------
void ssim_t::roi_entry(bool enter) {
  if(enter) {
    if(debug && (SS_DEBUG::COMMAND || SS_DEBUG::ROI)  ) {
      timestamp();
      cout << "Entering ROI ------------\n";
    }

    if(_orig_stat_start_cycle==0) {
      _orig_stat_start_cycle=now();
    }
    setup_stat_cycle();
    clock_gettime(CLOCK_REALTIME,&_start_ts);
    _in_roi=true;
    _times_roi_entered+=1;
  } else {
    clock_gettime(CLOCK_REALTIME,&_stop_ts);
    _elapsed_time_in_roi += 1000000000 * (_stop_ts.tv_sec - _start_ts.tv_sec) +
                                          _stop_ts.tv_nsec - _start_ts.tv_nsec;

    if(debug && (SS_DEBUG::COMMAND || SS_DEBUG::ROI)  ) {
      timestamp();
      cout << "Exiting ROI ------------";
      cout << "(" << _stat_start_cycle << "to" << _stat_stop_cycle << ")\n";
    }

    _roi_cycles += _stat_stop_cycle - _stat_start_cycle;
    _in_roi=false;
  }
}
