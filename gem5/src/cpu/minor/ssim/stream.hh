#ifndef __SS_STREAM_H__
#define __SS_STREAM_H__

#include <cstdint>
#include "consts.hh"
#include <iostream>
#include "loc.hh"

#include "cpu/minor/dyn_inst.hh" //don't like this, workaround later (TODO)

class soft_config_t;

//This is a hierarchical classification of access types
enum class STR_PAT {PURE_CONTIG, SIMPLE_REPEATED,
                        SIMPLE_STRIDE, OVERLAP, CONST, REC, IND,
                        NONE, OTHER, LEN};

//1.DMA -> Port    or    2.Port -> DMA
struct base_stream_t {
  static int ID_SOURCE;

  virtual bool stream_active() = 0;
  bool empty() {return _empty;} //This must be set by controller itself

  void reset() {
    _empty=true;
  }

  void set_empty(bool b);

  virtual LOC src() {return LOC::NONE;}
  virtual LOC dest() {return LOC::NONE;}

  static void sep(std::string &s) {
    if(s.length()!=0) s+="|";
  }

  static std::string loc_name(LOC loc) {
    std::string a;
    if(loc==LOC::NONE)                    return "???";
    if((loc&LOC::DMA)!=LOC::NONE)         {sep(a); a+="dma";}
    if((loc&LOC::SCR)!=LOC::NONE)         {sep(a); a+="scr";}
    if((loc&LOC::PORT)!=LOC::NONE)        {sep(a); a+="port";}
    if((loc&LOC::CONST)!=LOC::NONE)       {sep(a); a+="const";}
    if((loc&LOC::REMOTE_PORT)!=LOC::NONE) {sep(a); a+="rem_port";}
    if((loc&LOC::REMOTE_SCR)!=LOC::NONE)  {sep(a); a+="rem_scr";}
    if((loc&LOC::REC_BUS)!=LOC::NONE)     {sep(a); a+="rec_bus";}
    return a;
  }

  static std::string loc_short_name(LOC loc) {
    std::string a;
    if(loc==LOC::NONE)
    if((loc&LOC::DMA)!=LOC::NONE)         {sep(a); return a+="D";}
    if((loc&LOC::SCR)!=LOC::NONE)         {sep(a); return a+="S";}
    if((loc&LOC::PORT)!=LOC::NONE)        {sep(a); return a+="P";}
    if((loc&LOC::CONST)!=LOC::NONE)       {sep(a); return a+="C";}
    if((loc&LOC::REMOTE_PORT)!=LOC::NONE) {sep(a); return a+="R";}
    if((loc&LOC::REMOTE_SCR)!=LOC::NONE)  {sep(a); return a+="Q";}
    if((loc&LOC::REC_BUS)!=LOC::NONE)     {sep(a); return a+="B";}
    return "?";
  }



  virtual std::string short_name() {
    return loc_name(src()) + "->" + loc_name(dest());
  }

  bool check_set_empty() {
    inc_requests(); // check if correct
    if(!stream_active()) {
      set_empty(true);
    }
    return _empty;
  }

  virtual ~base_stream_t() { }
  void print_empty() {
    if(stream_active())  std::cout << "               ACTIVE";
    else                 std::cout << "             inactive";

    if(empty())          std::cout << "EMPTY!\n";
    else                 std::cout << "\n";
  }
  virtual void print_status() {
    print_empty();
  }

  void set_id() {_id=++ID_SOURCE;}
  int id() {return _id;}

  void inc_requests() {_reqs++;}
  uint64_t requests()     {return _reqs;}

  virtual uint64_t mem_addr()    {return 0;}
  virtual uint64_t ctx_offset()  {return _ctx_offset;}
  virtual int64_t  access_size() {return 0;}
  virtual int64_t  stride()      {return 0;}
  virtual uint64_t scratch_addr(){return 0;}
  virtual uint64_t num_strides() {return 0;}
  virtual int64_t  stretch()     {return 0;}
  virtual uint64_t num_bytes()   {return 0;}
  virtual uint64_t constant()    {return 0;}
  virtual int64_t out_port()     {return -1;}
  virtual int64_t val_port()     {return -1;}
  virtual uint64_t wait_mask()   {return 0;}
  virtual uint64_t shift_bytes() {return 0;}
  virtual uint64_t offset_list() {return 0;}
  virtual uint64_t ind_mult()    {return 1;}
  uint64_t data_width()    {return _data_width;}
  uint64_t straddle_bytes()    {return _straddle_bytes;}
  uint64_t wait_cycles()    {return _wait_cycles;} // waiting for whole cache line

  bool timeout() {
    if(_wait_cycles > 30) {
      _wait_cycles = 0;
      return true;
    }
    return false;
  }

  std::vector<int>& in_ports()      {return _in_ports;}
  int first_in_port()      {return _in_ports[0];}

  virtual int repeat_in()   {return 1;}
  virtual int repeat_str()   {return 0;}
  virtual bool repeat_flag()   {return false;}
  virtual uint32_t fill_mode() {return _fill_mode;}
  virtual bool stride_fill() {return _fill_mode == STRIDE_DISCARD_FILL ||
                                     _fill_mode == STRIDE_ZERO_FILL;}

  void add_in_port(int port_idx) {_in_ports.push_back(port_idx);}

  virtual uint64_t data_volume() {return 0;}
  virtual STR_PAT stream_pattern() {return STR_PAT::OTHER;}

  virtual void set_orig() {}

  void set_minst(Minor::MinorDynInstPtr m) {_minst=m;}
  Minor::MinorDynInstPtr minst() {return _minst;}

  void set_fill_mode(uint32_t mode) {_fill_mode = mode;}
  void set_context_offset(uint64_t offset) {_ctx_offset = offset;}

  void print_in_ports();
  void set_soft_config(soft_config_t* s) {_soft_config=s;}
  LOC unit() {return _unit;}
  LOC _unit = LOC::PORT;

  virtual void set_data_width(int d) {_data_width=d;}
  virtual void set_straddle_bytes(int d) {_straddle_bytes=d;}
  virtual void inc_wait_cycles() {_wait_cycles++;}

protected:
  int      _id=0;
  uint32_t _fill_mode=0; //0: none, 1 post-zero fill, 2 pre-zero fill (not implemented)
  int _data_width=DATA_WIDTH; 
  int _straddle_bytes=0; // bytes straddling over cache lines (used only in mem streams as of now!)
  // TODO: add this in all streams
  int _wait_cycles=0; // cycles to wait to get whole cache line at ports for write streams 
  bool _empty=false; //presumably, when we create this, it won't be empty
  Minor::MinorDynInstPtr _minst;
  uint64_t _reqs=0;
  uint64_t _ctx_offset=0;
  std::vector<int> _in_ports;
  soft_config_t* _soft_config;
};


//return -1 if x == 0
static inline int ilog2(const uint64_t x) {
  if(x==0) return -1;
  uint64_t y;
  asm ("\tbsr %1, %0\n" : "=r"(y) : "r" (x));
  return y;
}

//This class represents a network stream at the remote core
struct remote_core_net_stream_t : public base_stream_t {
  int _addr_port=NET_ADDR_PORT;
  int64_t _val_port=NET_VAL_PORT;
  int64_t _num_elements;
  // can be used later if we want separate ports for linear and banked scratchpad
  remote_core_net_stream_t(){
    _num_elements=1000; // TODO: see some other constants
  }

  int addr_port() {return _addr_port;}
  int64_t val_port() {return _val_port;}

   virtual bool stream_active() {
     return true;
  }

  virtual void print_status() {
    std::cout << "remote core net stream _num_elem" << _num_elements << "\taddr_port= " << _addr_port << "\tval_port" << _val_port << "\n";
  }
};

//This class represents a barrier (does not stall core)
struct stream_barrier_t : public base_stream_t {
  uint64_t _mask=0;
  // int64_t _num_remote_writes=-1;
  bool _scr_type=0; // 0 means banked scratchpad

  bool bar_scr_wr_df() {return _mask & WAIT_SCR_WR_DF;}
  bool bar_scr_rd() {return _mask & WAIT_SCR_RD;}
  bool bar_scr_wr() {return _mask & WAIT_SCR_WR;}

  virtual bool stream_active() {
    return true;
    // return (_num_remote_writes!=0); // returns true for streams which do not use this!
  }

  virtual void print_status() {
    if(bar_scr_rd()) {
      std::cout << " read_barrier";
    }
    if(bar_scr_wr()) {
      std::cout << " write_barrier";
    }
    if(bar_scr_wr_df()) {
      std::cout << " remote_write_barrier";
    }
    std::cout << std::hex << " mask=0x" << _mask << std::dec << "\n";
  }

};

struct affine_base_stream_t : public base_stream_t {
  uint64_t _context_bitmask=0; //
  std::vector<uint64_t> origin;
  std::vector<uint64_t> dims;
  std::vector<int> idx;
  std::vector<uint64_t> address;
  bool stride_hit_{false};

  affine_base_stream_t() {}

  int64_t access_size() override {
    assert(dims.size() >= 2);
    return dims[dims.size() - 2];
  }

  int64_t start_addr() {
    return dims[dims.size() - 3];
  }

  uint64_t dim_stride(int x) {
    assert(x >= 0);
    if (x < idx.size() - 1)
      return dims[x * 3];
    assert(x == idx.size() - 1);
    return 0;
  }

  uint64_t &dim_trip_count(int x) {
    return dims[x * 3 + 1];
  }

  int64_t dim_stretch(int x) {
    assert(x >= 0 && x < idx.size() - 1);
    return dims[x * 3 + 2];
  }

  affine_base_stream_t(LOC unit, const std::vector<uint64_t> &dims) : origin(dims), dims(dims) {
    _unit = unit;
    assert(dims.size() % 3 == 0);
    idx.resize(dims.size() / 3, 0);
    address.resize(idx.size(), start_addr());
  }

  virtual STR_PAT stream_pattern() override {
    //FIXME: general dimension requires more complicated analysis
    return STR_PAT::NONE;
  }

  virtual uint64_t data_volume() override { //TODO/FIXME: THIS IS NOT VALID FOR STRETCH!=0
    uint64_t res = access_size();
    for (int i = ((int) origin.size() - 1 - 3); i >= 0; i -= 3) {
      res *= origin[i - 2];
    }
    return res;
  }

  addr_t cur_addr() {
    return address.back();
  }

  bool stride_hit() {
    return stride_hit_;
  }

  // bytes: How many bytes of address to pop
  uint64_t pop_addr(int bytes = -1) {
    if (bytes == -1)
      bytes = data_width();
    auto &current_address = address.back();
    bool continuous = true;
    int total_bytes = 0;
    while (total_bytes < bytes && stream_active()) {
      uint64_t delta = std::min((uint64_t) bytes, (uint64_t)(access_size() - idx.back()));
      idx.back() += delta;
      total_bytes += delta;
      current_address += delta;
      if (!continuous) {
        std::cout << "bytes: " << delta << "\n";
        print_status();
      }
      assert(continuous);
      if (idx.back() == dim_trip_count(idx.size() - 1)) {
        stride_hit_ = true;
      } else {
        stride_hit_ = false;
      }
      for (int i = (int) idx.size() - 1; i >= 0; --i) {
        if (dim_stride(i)) {
          continuous = false;
        }
        current_address = (address[i] += dim_stride(i));
        if (idx[i] == dim_trip_count(i)) {
          if (!stream_active())
            break;
          idx[i] = 0;
          if (i - 1 >= 0) {
            ++idx[i - 1];
            dim_trip_count(i) += dim_stretch(i - 1);
          }
        } else {
          for (int j = i + 1; j < idx.size() - 1; ++j)
            address[j] = address[i];
          break;
        }
      }
    }
    return current_address;
  }

  bool stream_active() override {
    return idx[0] != dim_trip_count(0) && dims[dims.size() - 2] != 0;
  }

  virtual void print_status() override {
    std::cout << short_name() << "\n";
    for (size_t i = 0; i < idx.size() - 1; ++i) {
      std::cout << "dim[" << i << "]: stride=" << dim_stride(i) << ", "
                << "trip_cnt=" << idx[i] << "/" << dim_trip_count(i) << ", "
                << "stretch=" << dim_stretch(i) << "\n";
    }
    std::cout << "innermost: current=" << cur_addr()
              << ", acc_size=" << idx.back() << "/" << access_size()
              << ", port=" << dims.back() << "\n";
  }

};

//.........STREAM DEFINITION.........
struct affine_read_stream_t : public affine_base_stream_t {
  int _repeat_in=1, _repeat_str=0;
  bool _repeat_flag=false; // assume by-default not a port

  affine_read_stream_t(LOC unit, const std::vector<uint64_t> &dims,
    const std::vector<int> &in_ports, int repeat_in, int repeat_str) :
    affine_base_stream_t(unit, dims) {
    _in_ports=in_ports;
    _repeat_in=repeat_in;
    _repeat_str=repeat_str;
  }


  virtual int repeat_in() {return _repeat_in;}
  virtual int repeat_str() {return _repeat_str;}
  virtual bool repeat_flag() {return _repeat_flag;}

  virtual LOC src() {return _unit;}
  virtual LOC dest() {return LOC::PORT;}

  virtual void print_status() {
    affine_base_stream_t::print_status();
    std::cout << " in_port=";
    print_in_ports();
    std::cout << " repeat_in=" << _repeat_in << " stretch=" << _repeat_str;
    print_empty();
  }

};

struct affine_write_stream_t : public affine_base_stream_t {
  int _out_port;           //source or destination port

  affine_write_stream_t(LOC unit, const std::vector<uint64_t> &dims) :
    affine_base_stream_t(unit, dims) {
    _out_port = dims.back();
  }

  int64_t out_port()    {return _out_port;}
  uint64_t garbage()     {
    const auto &dims = affine_base_stream_t::dims;
    return dims[0] == 0 && dims.size() == 3u;
  }

  virtual LOC src() {return LOC::PORT;}
  virtual LOC dest() {return _unit;}

  virtual void print_status() {
    affine_base_stream_t::print_status();
    std::cout << " out_port=" << _out_port << " garbage=" << garbage();
    print_empty();
  }

};

//3. Scratch -> Scratch
//struct scr_scr_stream_t;
//struct scr_scr_stream_t : public mem_stream_base_t {
//  uint64_t _scratch_addr; //CURRENT scratch addr
//  bool _is_source;
//  bool _is_ready;
//
//  scr_scr_stream_t* _remote_stream;
//  uint64_t          _remote_bitmask;
//
//  scr_scr_stream_t() {}
//
//  scr_scr_stream_t(addr_t mem_addr, uint64_t stride, uint64_t access_size,
//      int stretch, uint64_t num_strides, addr_t scratch_addr, bool is_src) :
//         mem_stream_base_t(mem_addr,stride,access_size,stretch,
//        num_strides) {
//    _scratch_addr=scratch_addr;
//
//    _is_source = is_src;
//    _is_ready=false;
//
//    set_orig();
//  }
//
//  uint64_t mem_addr()    {return _mem_addr;}
//  int64_t  access_size() {return _access_size;}
//  int64_t  stride()      {return _stride;}
//  uint64_t num_strides() {return _num_strides;}
//  uint64_t shift_bytes() {return _shift_bytes;}
//  uint64_t scratch_addr(){return _scratch_addr;}
//
//  void set_remote(scr_scr_stream_t* r, uint64_t r_bitmask) {
//    _remote_bitmask=r_bitmask;
//    _remote_stream=r;
//  }
//  virtual LOC src() {
//    if(_is_source) return LOC::SCR;
//    else          return LOC::REMOTE_SCR;
//  }
//  virtual LOC dest() {
//    if(_is_source) return LOC::REMOTE_SCR;
//    else          return LOC::SCR;
//  }
//
//  virtual void print_status() {
//    if(_is_source) {
//      std::cout << "scr->remote_scr";
//    } else {
//      std::cout << "remote_scr->scr";
//    }
//
//    std::cout << "\tscr_addr=" << _scratch_addr
//              << "\tacc_size=" << _access_size
//              << " stride=" << _stride << " bytes_comp=" << _bytes_in_access
//              << " mem_addr=" << std::hex << _mem_addr << std::dec
//              << " strides_left=" << _num_strides;
//
//    base_stream_t::print_status();
//  }
//
//};


//Constant -> Port
struct const_port_stream_t : public base_stream_t {
  addr_t _constant;
  addr_t _num_elements=0;
  addr_t _constant2;
  addr_t _num_elements2=0;
  addr_t _iters_left=0;
  int _const_width=8;

  //running counters
  addr_t _elements_left;
  addr_t _elements_left2;
  addr_t _num_iters;

  addr_t _orig_elements;

  void check_for_iter() {
    if(!_elements_left && !_elements_left2 && _iters_left) {
      _iters_left--;
      _elements_left=_num_elements;
      _elements_left2=_num_elements2;
    }
  }

  virtual void set_orig() {
    _iters_left=_num_iters;
    _elements_left=0;
    _elements_left2=0;
    check_for_iter();
  }

  uint64_t constant()    {return _constant;}
  uint64_t num_strides() {return _num_elements;}

  virtual LOC src() {return LOC::CONST;}
  virtual LOC dest() {return LOC::PORT;}

  virtual uint64_t data_volume() {
    // return (_num_elements + _num_elements2) * _num_iters * sizeof(SBDT);
    return (_num_elements + _num_elements2) * _num_iters * _data_width;
  }
  virtual STR_PAT stream_pattern() {
    return STR_PAT::CONST;
  }

  uint64_t pop_item() {
    check_for_iter();
    if(_elements_left > 0) {
      _elements_left--;
      return _constant;
    } else if(_elements_left2) {
      _elements_left2--;
      return _constant2;
    }
    assert(0&&"should not have popped");
    return 0;
  }

  virtual bool stream_active() {
    return _iters_left!=0 || _elements_left!=0 || _elements_left2!=0;
  }

  virtual void print_status() {
     std::cout << "const->port" << "\tport=";
     print_in_ports();
     std::cout << " const_width: "  << _const_width;
     if(_num_elements) {
       std::cout << "\tconst:" << _constant << " left=" << _elements_left
                 << "/" << _num_elements;
     }
     if(_num_elements2) {
       std::cout << "\tconst2:" << _constant2  << " left=" << _elements_left2
                 << "/"  << _num_elements2;
     }
     std::cout << "\titers=" << _iters_left << "/" << _num_iters << "";

    base_stream_t::print_status();
  }

};

// const->scr
struct const_scr_stream_t : public base_stream_t {

  int _const_width=DATA_WIDTH;
  uint64_t _constant;
  int _num_elements;
  int _iters_left=0; //needs zeroing here, otherwise default can be active
  int _scratch_addr;

  virtual void set_orig() {
    _iters_left=_num_elements;
  }

  uint64_t constant()    {return _constant;}
  uint64_t num_strides() {return _num_elements;}

  int cur_scratch_addr() {
    // std::cout << " Initial scratc addr: " << _scratch_addr << " and const_width: " << _const_width << "\n";
    _scratch_addr += _const_width;
    // std::cout << " Final scratc addr: " << _scratch_addr << "\n";
    _iters_left--;
    return _scratch_addr;
  }

  virtual LOC src() {return LOC::CONST;}
  virtual LOC dest() {return LOC::SCR;}

  virtual bool stream_active() {
    return _iters_left!=0;
  }

  virtual void print_status() {
     if(_num_elements) {
       std::cout << "\tconst:" << _constant << " left=" << _iters_left
                 << "/" << _num_elements;
     }
     std::cout << "\titers=" << _iters_left << "/" << _num_elements;
     std::cout << "\tcur_scratch_addr=" << _scratch_addr << "";

    base_stream_t::print_status();
  }

};

//Port -> Port
struct port_port_stream_t : public base_stream_t {
  port_port_stream_t(){
    _num_elements=0;
  }

  port_port_stream_t(int out_port, int in_port,
                     uint64_t num_elem, int repeat, int repeat_str,
                     int src_data_width, uint64_t padding_size, bool repeat_flag) {
    _out_port=out_port;
    _in_ports.push_back(in_port);
    _num_elements=num_elem;
    _repeat_in=repeat;
    _repeat_str=repeat_str;
    _src_data_width=src_data_width;
    _padding_size = padding_size;
    _repeat_flag=repeat_flag;

    set_orig();
  }

  int _repeat_in=1, _repeat_str=0;
  bool _repeat_flag;

  virtual int repeat_in() {return _repeat_in;}
  virtual int repeat_str() {return _repeat_str;}
  bool repeat_flag() {return _repeat_flag;}

  int src_data_width() {
    return _src_data_width;
  }

  int _out_port;
  addr_t _num_elements=0;
  addr_t _orig_elements;
  // TODO: make it general (Read from out_vp)
  int _src_data_width=8; // 8-byte by default

  addr_t _padding_size;
  addr_t _padding_cnt;

  virtual void set_orig() {
    _orig_elements = _num_elements;
    _padding_cnt = 0;
  }

  // virtual uint64_t data_volume() {return _num_elements * sizeof(SBDT);}
  virtual uint64_t data_volume() {return _num_elements * _data_width;}
  virtual STR_PAT stream_pattern() {return STR_PAT::REC;}

  int64_t out_port()    {return _out_port;}
  uint64_t num_strides() {return _num_elements;}

  virtual LOC src() {return LOC::PORT;}
  virtual LOC dest() {return LOC::PORT;}

  virtual bool stream_active() {
    return _num_elements!=0;
  }

  virtual void cycle_status() {
  }

  virtual void print_status() {
    std::cout << "port->port" << "\tout_port=" << _out_port
              << " in_port:";
    print_in_ports();
    std::cout << " data_width: " << _data_width;
    std::cout  << " repeat:" << _repeat_in
               << " elem_left=" << _num_elements;
    std::cout << " padding_size=" << _padding_size
              << " current_pad=" << _padding_cnt;
    base_stream_t::print_status();
  }
};

struct remote_port_stream_t;
struct remote_port_stream_t : public port_port_stream_t {
  remote_port_stream_t() {
    _num_elements=0;
  }

  //core describes relative core position (-1 or left, +1 for right)
  // TODO: sending fix 8-byte for port->remote port, make it configurable
  remote_port_stream_t(int out_port, int in_port, uint64_t num_elem,
       int repeat, int repeat_str, int core, bool is_source, int access_size, bool repeat_flag) :
                       port_port_stream_t(out_port,in_port,num_elem,
                           repeat,repeat_str, 8, access_size, repeat_flag) { // send default p.w. to be 1, also default repeat 

    _in_ports.clear();

    if(is_source) {
      //shrug?
    } else {
      _in_ports.push_back(in_port);
    }
    _is_source = is_source;
    _which_core = core;
    _is_ready=false;
  }

  //this is the only pointer to source stream after source issues
  remote_port_stream_t* _remote_stream;
  int _which_core = 0;
  bool _is_source = false;
  bool _is_ready = false;

  virtual STR_PAT stream_pattern() {return STR_PAT::REC;}

  int64_t out_port()    {
    if(_is_source) return _out_port;
    else          return -1;
  }

  virtual LOC src() {
    if(_is_source) return LOC::PORT;
    else          return LOC::REMOTE_PORT;
  }
  virtual LOC dest() {
    if(_is_source) return LOC::REMOTE_PORT;
    else          return LOC::PORT;
  }

  virtual void print_status() {
    if(_is_source) {
      std::cout << "port->remote";
    } else {
      std::cout << "remote->port";
      if(_is_ready) { std::cout << " (source is ready)";}
      else { std::cout << " (source NOT ready)";}
    }
    std::cout << "\tout_port=" << _out_port;
    print_in_ports();
    std::cout << "\tdir:" << _which_core << "\trepeat:" << _repeat_in
              << "\telem_left=" << _num_elements;

    base_stream_t::print_status();
  }
};


//Indirect Read Port -> Port
struct indirect_base_stream_t : public base_stream_t {
  int _ind_port;
  int _ind_type, _dtype; //index and data types
  addr_t _num_elements;
  addr_t _index_addr;
  uint64_t _offset_list;
  uint64_t _ind_mult;
  std::vector<char> _offsets;
  int _sstream_size=1; // size of sub-stream: should be extracted from the num_elem port
  int _ssind=0;
  int _sstride=-1, _sacc_size=-1, _sn_port=1; // to prevent NULL (check in assert)
  bool _is_2d_stream=false;
  bool _first_ss_access=true;

  addr_t _orig_elements;
  //These get set based on _type
  unsigned _index_bytes, _data_bytes; // , _indices_in_word;
  uint64_t _index_mask, _data_mask;

  //Note: since ports hold 1-bit, this is the adapter
  //for indirect read so that it works regardless
  // uint64_t _cur_ind_val=0;
  int _ind_bytes_complete=0;

  virtual void set_orig() { //like constructor but lazier
    _orig_elements = _num_elements;
    // _index_in_word=0;
    _index_in_offsets=0;

    switch(_ind_type) {
      case T64: _index_bytes= 8; _index_mask = 0xFFFFFFFFFFFFFFFF;  break;
      case T32: _index_bytes= 4; _index_mask = 0xFFFFFFFF;          break;
      case T16: _index_bytes= 2; _index_mask = 0xFFFF;              break;
      case T08: _index_bytes= 1; _index_mask = 0xFF;                break;
      default: assert(0);
    }
    switch(_dtype) {
      case T64:  _data_bytes= 8; _data_mask = 0xFFFFFFFFFFFFFFFF;  break;
      case T32:  _data_bytes= 4; _data_mask = 0xFFFFFFFF;          break;
      case T16:  _data_bytes= 2; _data_mask = 0xFFFF;              break;
      case T08:  _data_bytes= 1; _data_mask = 0xFF;                break;
      default: assert(0);
    }

    // _indices_in_word = DATA_WIDTH / _index_bytes;
    
    // set up offset list
    _offsets.push_back(0);
    // FIXME: check this!
    // for(int i = 0; i < DATA_WIDTH; i++) {
    for(int i = 0; i < _data_width; i++) {
      char offset = (_offset_list >> i*8) & 0xFF;
      if(offset != 0) {
        _offsets.push_back(offset);
      }
    }
  }

  virtual uint64_t ind_port()     {return _ind_port;}
  virtual uint64_t ind_type()     {return _ind_type;}
  virtual uint64_t num_strides()  {return _num_elements;}
  virtual uint64_t index_addr()   {return _index_addr;}
  virtual uint64_t offset_list()  {return _offset_list;}
  virtual uint64_t ind_mult()     {return _ind_mult;}

  bool scratch()     {return _unit==LOC::SCR;}

  // virtual uint64_t data_volume() {return _num_elements * sizeof(SBDT);} //TODO: config
  virtual uint64_t data_volume() { return _num_elements; }
  virtual STR_PAT stream_pattern() {return STR_PAT::IND;}

  //if index < 64 bit, the index into the word from the port
  // unsigned _index_in_word=0;
  unsigned _index_in_offsets=0;

  addr_t cur_addr(SBDT val) {
    // uint64_t index =  (val >> (_index_in_word * _index_bytes * 8)) & _index_mask;
    uint64_t index =  val & _index_mask;
    if(SS_DEBUG::MEM_REQ) {
      std::cout << "index: " << index << " mult: " << _ind_mult << " ss_ind: " << _ssind << " offset: " << unsigned(_offsets[_index_in_offsets]) << " sstream size: " << _sstream_size << "\n";
      addr_t x = _index_addr + index * _ind_mult + _offsets[_index_in_offsets]*_data_bytes + _ssind*_sstride;
      std::cout << "The computed address is: " << x << "\n";
    }
    // return   _index_addr + index * _ind_mult + _offsets[_index_in_offsets]*_data_bytes;
    return   _index_addr + index * _ind_mult + _offsets[_index_in_offsets]*_data_bytes + _ssind*_sstride;
  }

  virtual LOC src() {return LOC::PORT;}
  virtual LOC dest() {return LOC::PORT;}

  virtual bool stream_active() {
    return _num_elements!=0;
  }

  //return value: should pop vector port
  bool pop_elem() {
    _index_in_offsets++;
    // std::cout << "index in offsets: " << _index_in_offsets << " and offset size: " << _offsets.size() << std::endl;

    if(_index_in_offsets >= _offsets.size()) {
      _index_in_offsets=0;
      _ssind++;
      if(_sstream_size!=_ssind) return false;
      _ssind=0;
      _num_elements--;
      //std::cout << _num_elements << " ";
      return true;
    }
    //std::cout << "\n";

    return false;
  }

  virtual void cycle_status() {
  }
};

//Indirect Read Port -> Port
struct indirect_stream_t : public indirect_base_stream_t {
  int _repeat_in=1, _repeat_str=0;
  addr_t cur_base_addr = 0;
  
  virtual int repeat_in() {return _repeat_in;}
  virtual int repeat_str() {return _repeat_str;}

  virtual void print_status() {
    std::cout << "mem[ind_port]->in_port" << "\tscratch? " << scratch()
      << "\tout_port width" << _data_width << "\tind_port=" << _ind_port
              << "\tind_type:" << _ind_type  << "\tind_addr:" << _index_addr
              << "\tnum_elem:" << _num_elements << "\tin_port:" 
              << "\tmult: " << _ind_mult
              << "\t2d_stream? " << _is_2d_stream
              << "\tnum_elem_port: " << _sn_port
              << "\tstride: " << _sstride
              << "\taccess_size: " << _sacc_size
              << "\tcur_ss_ind: " << _ssind
              << "\tsstream_size: " << _sstream_size << std::endl;
    print_in_ports();
    std::cout << "\toffsets:" << _offset_list;
    base_stream_t::print_status();
  }
  virtual bool stream_active() {
    return indirect_base_stream_t::stream_active();
  }

  // FIXME:CHECKME: this consumes both port->dma_ctrl and dma->port b/w
  virtual LOC src() {
    if(!scratch()) {
      // FIXME:CHECKME
      // return LOC::PORT|LOC::DMA;
      return LOC::DMA;
    } else {
      return LOC::PORT|LOC::SCR;
    }
  }
  virtual LOC dest() {return LOC::PORT;}

};

//Indirect Read Port -> Port
struct indirect_wr_stream_t : public indirect_base_stream_t {
  int _out_port;

  int64_t out_port()     {return _out_port;}

  uint64_t cur_value(uint64_t val) {
    return (val >> (_ind_bytes_complete * 8)) & _data_mask;
  }

  virtual void print_status() {
    std::cout << "out_port->mem[ind_port]" << "\tout_port width" << _data_width << "\tind_port=" << _ind_port // check this!
              << "\tind_type:" << _ind_type  << "\tind_addr:" << std::hex <<_index_addr
        << std::dec << "\tnum_elem:" << _num_elements << "\tout_port" << _out_port;
    base_stream_t::print_status();
  }

  virtual LOC src() {return LOC::PORT;}
  virtual LOC dest() {
    if(!scratch()) {
      return LOC::PORT|LOC::DMA;
    } else {
      return LOC::PORT|LOC::SCR;
    }
  }
};

//Port -> Remote Port
//TODO: TO reuse some info, can convert to port_port_stream later
struct remote_port_multicast_stream_t;
struct remote_port_multicast_stream_t : public base_stream_t {
  remote_port_multicast_stream_t() {
    _num_elements=0;
  }

remote_port_multicast_stream_t(int out_port, int remote_in_port, uint64_t num_elem,
                       int64_t mask) { //:
                       // base_stream_t(out_port,remote_in_port,num_elem) {
    _remote_port = remote_in_port;
    _core_mask = mask;
    // _is_ready=false;
  }

  LOC src() {return LOC::PORT;}
  LOC dest() {return LOC::NETWORK;}

    // _core_mask = mask;
  // this was core_id instead of the mask
  // bool _is_ready = false;
  // int _which_core = 0;

  int64_t _core_mask = 0;
  int64_t _remote_port = -1;
  int64_t _out_port;
  addr_t _num_elements=0; // TODO: this is num_bytes
  addr_t _orig_elements;

  virtual void set_orig() {
    _orig_elements = _num_elements;
  }

  int64_t out_port()    {return _out_port;}
  uint64_t num_strides() {return _num_elements;}

  virtual STR_PAT stream_pattern() {return STR_PAT::REC;}

  virtual bool stream_active() {
    return _num_elements!=0;
  }

  virtual void cycle_status() {
  }

  // port
  virtual void print_status() {
    std::cout << "port->remote port";
    std::cout << "\tout_port=" << _out_port;
    // print_in_ports();
    std::cout << "\tremote_in_port=" << _remote_port;
    std::cout << "\tmask:" << _core_mask << "\telem_left=" << _num_elements;

    base_stream_t::print_status();
  }

  // virtual LOC src() {return LOC::PORT;}
  // virtual LOC dest() {return LOC::REMOTE_PORT;}
};

struct remote_scr_stream_t;
struct remote_scr_stream_t : public remote_port_multicast_stream_t {
    remote_scr_stream_t() {
    _num_elements=0;
  }

  remote_scr_stream_t(int out_port, int addr_port, addr_t scratch_base_addr, uint64_t num_elem, bool spad_type) :
                       remote_port_multicast_stream_t(out_port,-1,num_elem,-1) {

    // _which_core = core;
    // _is_ready=false;
    _remote_scr_base_addr = scratch_base_addr;
    _scr_type = spad_type;
    _addr_port = addr_port;
  }

  // LOC src() {return LOC::PORT;}
  // LOC dest() {return LOC::NETWORK;}


  addr_t _remote_scr_base_addr = -1;
  bool _scr_type = 0; // 0 means banked scr
  int _addr_port = -1;
  // out_port is val port
  // int64_t _out_port;
  // addr_t _num_elements=0;
  // addr_t _orig_elements;

  virtual void set_orig() {
    _orig_elements = _num_elements;
  }

  int64_t val_port()    {return _out_port;}
  int addr_port()    {return _addr_port;}
  addr_t scratch_base_addr()    {return _remote_scr_base_addr;}
  uint64_t num_strides() {return _num_elements;}

  virtual STR_PAT stream_pattern() {return STR_PAT::REC;}

  virtual bool stream_active() {
    return _num_elements!=0;
  }

  virtual void cycle_status() {
  }

  // port
  virtual void print_status() {
    std::cout << "indirect port->remote scratch";
    std::cout << "\tval_port=" << _out_port;
    std::cout << "\taddr_port=" << _addr_port;
    std::cout << "\tremote_scratch_base_addr:" << _remote_scr_base_addr << "\telem_left=" << _num_elements;

    // base_stream_t::print_status(); // configuration may not have been done yet!
  }

  virtual LOC src() {return LOC::PORT;}
  virtual LOC dest() {return LOC::REMOTE_SCR;}
};

struct direct_remote_scr_stream_t : public remote_scr_stream_t {
  direct_remote_scr_stream_t() {
    _num_elements=0;
  }
  virtual void set_data_width(int data_width) override {
    _data_width = data_width;
    assert(_access_size >= _data_width);
    assert(_access_size%_data_width==0);
    _max_count = _access_size/_data_width; // assuming we can send max data width at a time
  }

  direct_remote_scr_stream_t(addr_t base_addr, int64_t acc_size, int64_t stride) {
    _num_elements=0;
    _remote_scr_base_addr = base_addr;
    _cur_addr = _remote_scr_base_addr;
    _stride = stride;
    _access_size = acc_size;
  }

  /*
  remote_scr_stream_t(int out_port, int addr_port, addr_t scratch_base_addr, uint64_t num_elem, bool spad_type) :
                       remote_port_multicast_stream_t(out_port,-1,num_elem,-1) {

    // _which_core = core;
    // _is_ready=false;
    _remote_scr_base_addr = scratch_base_addr;
    _scr_type = spad_type;
    _addr_port = addr_port;
  }
  */

  // addr_t _remote_scr_base_addr = -1;
  bool _scr_type = 0; // 0 means banked scr
  int64_t _stride = -1;
  int64_t _access_size = -1;
  addr_t _cur_addr = -1;
  int _count = 0;
  int _max_count = 0;
  // int _addr_port = -1;
  // out_port is val port
  // int64_t _out_port;
  // addr_t _num_elements=0;
  // addr_t _orig_elements;

  virtual void set_orig() override {
    _orig_elements = _num_elements;
  }

  int64_t val_port() override {return _out_port;}
  addr_t scratch_base_addr() {return _remote_scr_base_addr;}
  uint64_t num_strides() override {return _num_elements;}

  virtual STR_PAT stream_pattern() override {return STR_PAT::REC;}

  virtual bool stream_active() override {
    return _num_elements!=0;
  }

  // Oh, 2 dimensions?
  // TODO: use the affine_base_stream for this
  virtual addr_t cur_addr() {
    if(_count == 0) { // the first base addr
      _count++;
    } else if(_count < _max_count) { // the next ones in acc_size dimension
      _cur_addr += _data_width;
      _count++;
    } else {
      _cur_addr = _cur_addr - _access_size + _stride + _data_width;
      _count = 0;
      _count++;
      // _num_strides--;
      _num_elements--;
    }
    return _cur_addr;
  }

  virtual void cycle_status() override {
  }

  // port
  virtual void print_status() override {
    std::cout << "direct port->remote scratch";
    std::cout << "\tval_port=" << _out_port;
    // std::cout << "\tremote_scratch_base_addr:" << _mem_addr << "\telem_left=" << _num_elements;
    std::cout << "\telem_left=" << _num_elements;
    std::cout << "\tdata_width=" << _data_width;

    base_stream_t::print_status(); // configuration may not have been done yet!
  }

  virtual LOC src() override {return LOC::PORT;}
  virtual LOC dest() override {return LOC::REMOTE_SCR;}
};



//Indirect Read Port -> SCR
struct atomic_scr_stream_t;
struct atomic_scr_stream_t : public base_stream_t {
  int _val_port;
  int _out_port;
  int _op_code;
  int _value_type;
  int _output_type;
  int _addr_type;
  uint64_t _num_strides;
  uint64_t _mem_addr;
  uint8_t _value_bytes, _addr_bytes, _output_bytes;
  uint64_t _value_mask, _addr_mask, _output_mask;
  uint64_t _values_in_word;
  uint64_t _addr_in_word;
  uint64_t _cur_val_index;
  uint64_t _cur_addr_index;

  atomic_scr_stream_t() {}
  virtual void set_orig() { //like constructor but lazier
    switch(_value_type) {
      case T64: _value_bytes= 8; _value_mask = 0xFFFFFFFFFFFFFFFF;  break;
      case T32: _value_bytes= 4; _value_mask = 0xFFFFFFFF;          break;
      case T16: _value_bytes= 2; _value_mask = 0xFFFF;              break;
      case T08: _value_bytes= 1; _value_mask = 0xFF;                break;
      default: assert(0);
    }
    switch(_output_type) {
      case T64: _output_bytes= 8; _output_mask = 0xFFFFFFFFFFFFFFFF;  break;
      case T32: _output_bytes= 4; _output_mask = 0xFFFFFFFF;          break;
      case T16: _output_bytes= 2; _output_mask = 0xFFFF;              break;
      case T08: _output_bytes= 1; _output_mask = 0xFF;                break;
      default: assert(0);
    }
    switch(_addr_type) {
      case T64:  _addr_bytes= 8; _addr_mask = 0xFFFFFFFFFFFFFFFF; break;
      case T32:  _addr_bytes= 4; _addr_mask = 0xFFFFFFFF;         break;
      case T16:  _addr_bytes= 2; _addr_mask = 0xFFFF;             break;
      case T08:  _addr_bytes= 1; _addr_mask = 0xFF;               break;
      default: assert(0);
    }
    _cur_val_index=0;
    _cur_addr_index=0;
    _values_in_word = DATA_WIDTH / _value_bytes;
    _addr_in_word = DATA_WIDTH / _addr_bytes;
  }

  int64_t out_port()     {return _out_port;}
  int64_t val_port()     {return _val_port;} // this is the inc
  int op_code()     {return _op_code;} // opcode

  uint64_t num_strides() {return _num_strides;} // iters
  uint64_t mem_addr()    {return _mem_addr;}

  // FIXME: this should from most significant (Although doesn't matter much
  // because our operations our idempotent)
  uint64_t cur_offset(){
    // extracting from right (least significant bits)
    // return (mem_addr() >> (_cur_addr_index*_addr_bytes*8)) & _addr_mask;
    return (mem_addr() >> ((_addr_in_word-_cur_addr_index-1)*_addr_bytes*8)) & _addr_mask;
  }
  uint64_t cur_addr(uint64_t loc){
    // extracting from right (least significant bits)
    // return (loc >> (_cur_addr_index*_addr_bytes*8)) & _addr_mask;
    return (loc >> ((_addr_in_word-_cur_addr_index-1)*_addr_bytes*8)) & _addr_mask;
  }
  uint64_t cur_val(uint64_t val){
    // return (val >> (_cur_val_index*_value_bytes*8)) & _value_mask;
    return (val >> ((_values_in_word-_cur_val_index-1)*_value_bytes*8)) & _value_mask;
  }
  void inc_val_index(){
    _cur_val_index = (_cur_val_index+1)%(_values_in_word+1);
  }
  void inc_addr_index(){
    _cur_addr_index = (_cur_addr_index+1)%(_addr_in_word+1);
  }

  bool can_pop_val(){
    // std::cout << "_cur_val_index: " << _cur_val_index << " values in word: " << _values_in_word << "\n";
    bool can_pop = (_cur_val_index==_values_in_word);
    return can_pop;
  }
  bool can_pop_addr(){
    // std::cout << "_cur_addr_index: " << _cur_addr_index << " values in word: " << _addr_in_word << "\n";
    bool can_pop = (_cur_addr_index==_addr_in_word);
    return can_pop;

  }

  virtual bool stream_active() {
    return _num_strides!=0;
  }
  virtual LOC src() {return LOC::PORT;}
  virtual LOC dest() {return LOC::SCR;}

  virtual void print_status() {
    std::cout << "atomic_scr " << "\tval_port=" << _val_port
              << "\taddr_port:" << _out_port  << "\top_code:" << _op_code << "\titers left: " << _num_strides
         << std::dec << "\tinput_type:" << _value_type << "\toutput_type:" << _output_type << "\taddr_type:" << _addr_type << "\n";
       };

  // base_stream_t::print_status();
};


#endif
