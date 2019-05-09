#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <utility>

#include "cpu/minor/cpu.hh"
#include "ssim.hh"

using namespace std;

int accel_t::get_cur_cycle() {
  return _ssim->now()-_ssim->roi_enter_cycle();
}

Minor::MinorDynInstPtr accel_t::cur_minst() { return _ssim->cur_minst(); }

void accel_t::sanity_check_stream(base_stream_t *s) {
  // sanity check -- please don't read/write a stream that's not configured!
  for (int in_port : s->in_ports()) {
    if (in_port != -1 && in_port < START_IND_PORTS) {
      vector<int>::iterator it =
          std::find(_soft_config.in_ports_active.begin(),
                    _soft_config.in_ports_active.end(), in_port);
      if (it == _soft_config.in_ports_active.end()) {
        timestamp();
        std::cout << "In port " << in_port << " is not active for "
                  << s->short_name() << ", maybe a configure error!\n";
        s->print_status();
      }
    }
  }

  if (s->out_port() != -1 && s->out_port() < START_IND_PORTS) {
    vector<int>::iterator it =
        std::find(_soft_config.out_ports_active.begin(),
                  _soft_config.out_ports_active.end(), s->out_port());
    if (it == _soft_config.out_ports_active.end()) {
      timestamp();
      std::cout << "Out port " << s->out_port() << " is not active for "
                << s->short_name() << ", maybe a configure error!\n";
    }
  }
}

void accel_t::req_config(addr_t addr, int size, uint64_t context) {
  assert(_in_config == false);

  if (debug && (SS_DEBUG::COMMAND || SS_DEBUG::SCR_BARRIER)) {
    timestamp();
    cout << "SS_CONFIGURE(request): "
         << "0x" << std::hex << addr << " " << std::dec << size << "\n";
  }
  SSMemReqInfoPtr sdInfo = new SSMemReqInfo(-4, context, CONFIG_STREAM);

  _lsq->pushRequest(cur_minst(), true /*isLoad*/, NULL /*data*/,
                    size * 8 /*cache line*/, addr, 0 /*flags*/, 0 /*res*/,
                    sdInfo);
}

void accel_t::request_reset_data() {
  _cleanup_mode = true;
  reset_data();
  _cmd_queue.clear(); // check this as well
  if(SS_DEBUG::COMMAND_O) {
    cout << "Complete reset request served except deleting requested memory reads/writes: " << _lsq->getCpuId() << "\n";
  }
}

// Let's not consider input ports here
bool accel_t::all_ports_empty() {

  
  // TESTING -- it worked?
  /*
  for (unsigned i = 0; i < _soft_config.in_ports_active_plus.size(); ++i) {
    int cur_port = _soft_config.in_ports_active_plus[i];
    auto &in_vp = _port_interf.in_port(cur_port);
    // if(i==5 || i==31) { continue; }
    if(in_vp.in_use() && in_vp.mem_size()) {
      cout << " Input port not empty: " << cur_port << endl;
      return false;
    }
  }

  cout << "Input ports empty\n";
  */
  
  
  
  // It should check only those output ports which are busy
  for (unsigned i = 0; i < _soft_config.out_ports_active_plus.size(); ++i) {
    int cur_port = _soft_config.out_ports_active_plus[i];
    auto &out_vp = _port_interf.out_port(cur_port);
    // if(out_vp.in_use() && out_vp.mem_size()) {
    if(out_vp.in_use() && out_vp.mem_size()) {
      cout << "Output port not empty: " << cur_port << " at core id: " << _lsq->getCpuId() << endl;
      return false;
    }
  }

  cout << "Output ports empty\n";

  // Let's check atomic bank buffers
  if(_scr_w_c.atomic_scr_issued_requests_active()) {
    cout << "Atomic bank buffers not empty!\n";
    return false;
  }

  // cout << "Atomic bank buffers empty\n";

  return true;
}

void accel_t::request_reset_streams() {

  if(!all_ports_empty()) return;

  if(SS_DEBUG::COMMAND_O) {
    cout << "RESET STREAM REQUEST RELEASED for core: " << _lsq->getCpuId() << "\n";
  }

  print_status();
  _cmd_queue.clear();

  // free ports and streams (not input ports?)
  // reset_data();
  for (unsigned i = 0; i < _soft_config.out_ports_active_plus.size(); ++i) {
    int cur_port = _soft_config.out_ports_active_plus[i];
    auto &out_vp = _port_interf.out_port(cur_port);
    out_vp.reset_data();
    /*if(out_vp.in_use()) {
      out_vp.reset_data();
    }*/
  }

  for (unsigned i = 0; i < _soft_config.in_ports_active_plus.size(); ++i) {
    int cur_port = _soft_config.in_ports_active_plus[i];
    auto &in_vp = _port_interf.in_port(cur_port);
    /*if(cur_port==5 || cur_port==31) {
      continue;
    } else {
      in_vp.reset_data();
    }*/
    if(in_vp.in_use()) {
      // also remove data from it
      // in_vp.set_status(port_data_t::STATUS::FREE);
      in_vp.reset_data();
    }
  }

  _dma_c.reset_data();
  _scr_r_c.reset_data();
  _scr_w_c.reset_data();
  _net_c.reset_data();
  _port_c.reset_data();

  if (_sched) {
    _sched->reset_simulation_state();
  }
  print_status();
  _stream_cleanup_mode=false;
  _cleanup_mode = true; // it should wait for outstanding mem req to be done (wait on o/p ports and then cleanup memory)
}

void accel_t::switch_stream_cleanup_mode_on() {
  _stream_cleanup_mode = true;
}

void accel_t::reset_data() {
  if (SS_DEBUG::COMMAND) {
    timestamp();
    std::cout << "RESET_DATA REQUEST INITIALIZED\n";
  }

  for (unsigned i = 0; i < _soft_config.in_ports_active_plus.size(); ++i) {
    int cur_port = _soft_config.in_ports_active_plus[i];
    auto &in_vp = _port_interf.in_port(cur_port);
    in_vp.reset_data();
  }

  // TODO: FIXME: I think we should not need to reset network ports? (reset is
  // for compute -- cannot reset it before config also)
  /*
  auto &in_addr = _port_interf.in_port(NET_ADDR_PORT);
  in_addr.reset_data();
 
  auto &in_val = _port_interf.in_port(NET_VAL_PORT);
  in_val.reset_data();
  */
  
  for (unsigned i = 0; i < _soft_config.out_ports_active_plus.size(); ++i) {
    int cur_port = _soft_config.out_ports_active_plus[i];
    auto &out_vp = _port_interf.out_port(cur_port);
    out_vp.reset_data();
  }

  _dma_c.reset_data();
  _scr_r_c.reset_data();
  _scr_w_c.reset_data();
  _net_c.reset_data();
  _port_c.reset_data();

  if (_sched) {
    _sched->reset_simulation_state();
  }
}

// --------------------------------- CONFIG
// ---------------------------------------
void soft_config_t::reset() {
  cur_config_addr = 0;
  in_ports_active.clear();
  in_ports_active_group.clear();
  in_ports_active_backcgra.clear();
  in_ports_name.clear();
  out_ports_name.clear();
  group_thr.clear();
  out_ports_active.clear();
  out_ports_active_group.clear();
  in_ports_active_plus.clear();
  out_ports_active_plus.clear();
  in_port_delay.clear();
  out_ports_lat.clear();
  cgra_in_ports_active.clear();
  input_dfg_node.clear();
  output_dfg_node.clear();
  inst_histo.clear();
}

// ------------------------------ VECTOR PORTS
// ------------------------------------- void port_data_t::initialize(SSModel*
// ssconfig, int port, bool isInput, int port_width) {
void port_data_t::initialize(SSModel *ssconfig, int port, bool isInput) {
  _isInput = isInput;
  _port = port;
}

void port_data_t::reset() {
  assert(_outstanding == 0);
  assert(_status == STATUS::FREE);
  // assert(_loc==LOC::NONE); TODO: should this be enforced?
  // removed because for data which enters remotely, cgra might get data from
  // somewhere else assert(_mem_data.size()==0);
  assert(_mem_data.size() == _valid_data.size());

  assert(_num_in_flight == 0);
  assert(_num_ready == 0);
  _dfg_vec=NULL;

  _total_pushed = 0;
}


// total size elements of the vector port
unsigned port_data_t::port_vec_elem() {
  if(_dfg_vec) {
    // return _dfg_vec->length();
    return _dfg_vec->logical_len();
  } else {
    return 1;
  }
}

void port_data_t::set_repeat(int r, int rs, bool rf) {
  _repeat = r;
  _cur_repeat_lim = r;
  _repeat_stretch = rs;
  _repeat_flag = rf;
  if (r != _repeat || rs != _repeat_stretch) {
    // we are safe to reset times_repeated if these have changed, since no
    // stream can be active while these are changing
    _num_times_repeated = 0;
  }
}

bool port_data_t::inc_repeated() {
  auto repeat_lim = (_cur_repeat_lim - 1) / (1 << REPEAT_FXPNT) + 1;
  if (++_num_times_repeated >= repeat_lim) {
    assert(_num_times_repeated == repeat_lim &&
           "Repeat time cannot be more than repeat limit!");
    _num_times_repeated = 0;
    _cur_repeat_lim += _repeat_stretch;
  }
  return _num_times_repeated == 0;
}

void port_data_t::reformat_in() { // rearrange data for CGRA
  // resize the data queue to macth the data elements of port vector
  /*unsigned extra_elem = _mem_data.size() % port_vec_elem();
  if(extra_elem!=0) {
    std::cerr << "Too few elem, zero padding\n";
    _mem_data.resize(_mem_data.size() + port_vec_elem()-extra_elem);
  }*/
  while (_mem_data.size() >= port_vec_elem()) {
    reformat_in_work();
  }
}

void port_data_t::reformat_in_one_vec() { // rearrange data for CGRA
  if (_mem_data.size() >= port_vec_elem() &&
      _num_ready + port_depth() <= CGRA_FIFO_LEN) {
    reformat_in_work();
  }
}

void port_data_t::reformat_in_work() {
  if (_repeat == 0) { // on repeat==0, delete data
    _mem_data.erase(_mem_data.begin(), _mem_data.begin() + port_vec_elem());
    _valid_data.erase(_valid_data.begin(),
                      _valid_data.begin() + port_vec_elem());
    return;
  }

  int cgra_port=0; //just stripe accross the port
  for(unsigned i = 0; i < port_vec_elem(); ++i,    
        cgra_port = cgra_port+1 == _cgra_data.size() ? 0 : cgra_port + 1) {
      std::vector<uint8_t> val = _mem_data.front();
      bool valid = _valid_data.front();
      _cgra_data[cgra_port].push_back(val);
      _cgra_valid[cgra_port].push_back(valid);
      _mem_data.pop_front();
      _valid_data.pop_front();
  }

  _num_ready += port_depth();
}

// pop data from input port
// It should be allowed to pop only _port_width data
SBDT port_data_t::pop_in_data() {
  assert(_mem_data.size());
  assert(_mem_data.size() == _valid_data.size());
  SBDT val = peek_out_data();
  _mem_data.pop_front();
  _valid_data.pop_front();
  return val;
}

template <typename T> 
T port_data_t::pop_in_custom_data() {
  assert(_mem_data.size());
  vector<uint8_t> v = _mem_data.front();
  T val = get_custom_val<T>(v, _port_width);
  _mem_data.pop_front();
  _valid_data.pop_front();
  return val;
}

// pop SBDT data from output port
SBDT port_data_t::pop_out_data() {
  assert(_mem_data.size());
  SBDT val = peek_out_data();
  _mem_data.pop_front();
  _valid_data.pop_front(); // TODO: for now we ignore invalid, hope that's okay?
  return val;
}

// pop T amount of data from output port
template <typename T> T port_data_t::pop_out_custom_data() {
  assert(_mem_data.size());
  vector<uint8_t> v = _mem_data.front();
  T val = get_custom_val<T>(v, _port_width);
  _mem_data.pop_front();
  _valid_data.pop_front();
  return val;
}

// peek at output data (return original data typecasted to SBDT)
SBDT port_data_t::peek_out_data() {
  assert(_mem_data.size());
  vector<uint8_t> v = _mem_data.front();
  SBDT val = get_sbdt_val(v, _port_width);
  return val;
}

// send ith element considering scalar port
SBDT port_data_t::peek_out_data(int i) {
  assert(i>=0 && _mem_data.size()>i);
  vector<uint8_t> v = _mem_data[i];
  SBDT val = get_sbdt_val(v, _port_width);
  return val;
}

// Throw away data in CGRA input ports
void port_data_t::pop(unsigned instances) {
  assert(_num_ready >= instances);
  _num_ready -= instances;
  assert(_mem_data.size() == _valid_data.size());

  for (unsigned cgra_port = 0; cgra_port < _cgra_data.size(); ++cgra_port) {
    assert(_cgra_data[cgra_port].size() == _cgra_valid[cgra_port].size());
    assert(_cgra_data[cgra_port].size() >= instances);
    _cgra_data[cgra_port].erase(_cgra_data[cgra_port].begin(),
                                _cgra_data[cgra_port].begin() + instances);
    _cgra_valid[cgra_port].erase(_cgra_valid[cgra_port].begin(),
                                 _cgra_valid[cgra_port].begin() + instances);
  }
  assert(_mem_data.size() == _valid_data.size());
}

// rearrange data from CGRA
void port_data_t::reformat_out() {
  while (can_output()) {
    reformat_out_work();
  }
}

void port_data_t::reformat_out_one_vec() {
  // if (can_output() && _mem_data.size() + port_vec_elem() <= VP_LEN) {
  if (can_output() && _mem_data.size() + port_vec_elem() <= VP_LEN*8/_port_width) {
    assert(_cgra_data[0].size() >= _num_ready);
    assert(_cgra_data[0].size() >= 1);
    reformat_out_work();
  }
}

void port_data_t::reformat_out_work() {
  unsigned num = 0;
  for (unsigned cgra_port = 0; cgra_port < _cgra_data.size(); ++cgra_port) {
    assert(num == 0 || _cgra_data[cgra_port].size() == num);
    num = _cgra_data[cgra_port].size();
  }

  int cgra_port=0; //just stripe accross the port
  for(unsigned i = 0; i < port_vec_elem(); ++i,
    cgra_port = cgra_port+1 == _cgra_data.size() ? 0 : cgra_port + 1) {
    assert(_cgra_data[cgra_port].size() > 0);
    assert(_cgra_data[cgra_port].size() == _cgra_valid[cgra_port].size());

    vector<uint8_t> v = _cgra_data[cgra_port].front();
    bool valid = _cgra_valid[cgra_port].front();

    if (valid) push_data(v, v.size());

    _cgra_data[cgra_port].pop_front();
    _cgra_valid[cgra_port].pop_front();
  }

  _num_ready -= port_depth();
}

// ----------------------------- Port Interface
void port_interf_t::initialize(SSModel *ssconfig) {
  _in_port_data.resize(NUM_IN_PORTS);
  _out_port_data.resize(NUM_OUT_PORTS);

  for(int i=0; i<NUM_IN_PORTS; ++i) {
    _in_port_data[i].initialize(ssconfig, i, true);
  }

  for(int j=0; j<NUM_OUT_PORTS; ++j) {
    _out_port_data[j].initialize(ssconfig, j, false);
  }

  /*
  for (auto &x : ssconfig->subModel()->io_interf().in_vports) {
    // cout << "Sending for input port: " << x.first << "\n";
    _in_port_data[x.first].initialize(ssconfig, x.first, true);
  }

  for (auto &x : ssconfig->subModel()->io_interf().out_vports) {
    // cout << "Sending for output port: " << x.first << "\n";
    _out_port_data[x.first].initialize(ssconfig, x.first, false);
  }*/
}

// ---------------------------- ACCEL ------------------------------------------
uint64_t accel_t::now() { return _lsq->get_cpu().curCycle(); }

accel_t::accel_t(Minor::LSQ *lsq, int i, ssim_t *ssim)
    : _ssim(ssim), _lsq(lsq), _accel_index(i), _accel_mask(1 << i),
      _dma_c(this, &_scr_r_c, &_scr_w_c, &_net_c), _scr_r_c(this, &_dma_c),
      _scr_w_c(this, &_dma_c), _net_c(this, &_dma_c), _port_c(this) {

  int ugh = system("mkdir -p stats/");
  ugh += system("mkdir -p viz/");

  if (SS_DEBUG::VERIF_MEM || SS_DEBUG::VERIF_PORT || SS_DEBUG::VERIF_CGRA ||
      SS_DEBUG::VERIF_SCR || SS_DEBUG::VERIF_CMD) {
    ugh += system("mkdir -p verif/");
    cout << "DUMPING VERIFICATION OUTPUTS (dir: verif/) ... SIMULATION WILL BE "
            "SLOWER\n";
    cout << "ALSO, MEMORY ACCESS STATISTICS MAY DIFFER\n";
  }
  if (SS_DEBUG::VERIF_PORT) {
    in_port_verif.open(
        ("verif/" + SS_DEBUG::verif_name + "in_port.txt").c_str(),
        ofstream::trunc | ofstream::out);
    assert(in_port_verif.is_open());
    out_port_verif.open(
        ("verif/" + SS_DEBUG::verif_name + "out_port.txt").c_str(),
        ofstream::trunc | ofstream::out);
    assert(out_port_verif.is_open());
  }
  if (SS_DEBUG::VERIF_SCR) {
    scr_rd_verif.open(("verif/" + SS_DEBUG::verif_name + "scr_rd.txt").c_str(),
                      ofstream::trunc | ofstream::out);
    assert(scr_rd_verif.is_open() && scr_rd_verif.good());

    scr_wr_verif.open(("verif/" + SS_DEBUG::verif_name + "scr_wr.txt").c_str(),
                      ofstream::trunc | ofstream::out);
    assert(scr_wr_verif.is_open());
  }
  if (SS_DEBUG::VERIF_CMD) {
    cmd_verif.open(("verif/" + SS_DEBUG::verif_name + "core_trace.txt").c_str(),
                   ofstream::trunc | ofstream::out);
    assert(cmd_verif.is_open());
  }

  _cgra_dbg_stream = &std::cout;

  if (SS_DEBUG::VERIF_CGRA) {
    SS_DEBUG::COMP = 1;
    cgra_multi_verif.open(("verif/" + SS_DEBUG::verif_name + "cgra_comp" +
                           std::to_string(_accel_index) + ".txt")
                              .c_str(),
                          ofstream::trunc | ofstream::out);
    assert(cgra_multi_verif.is_open());
    _cgra_dbg_stream = &cgra_multi_verif;
  }

  const char *fifo_len_str = std::getenv("FU_FIFO_LEN");
  if (fifo_len_str != nullptr) {
    _fu_fifo_len = atoi(fifo_len_str);
  }

  const char *ind_rob_size = std::getenv("IND_ROB_SIZE");
  if (ind_rob_size != nullptr) {
    _ind_rob_size = atoi(ind_rob_size);
  }

  const char *ssconfig_file = std::getenv("SBCONFIG");
 
  // if (!SS_DEBUG::SUPRESS_STATS) {
  if (SS_DEBUG::SUPRESS_STATS==0) {
    // static bool printed_this_before = false;
    if (!_ssim->printed_this_before()) {
      std::cout << "Loading SS Config (env SBCONFIG): \"" << ssconfig_file
                << "\"\n";
      std::cout << "FU_FIFO_LEN:" << _fu_fifo_len << "\n";
      std::cout << "IND_ROB_SIZE:" << _ind_rob_size << "\n";
      _ssim->set_printed_this_before(true);
      // printed_this_before=true;
    }
  }

  const char *back_cgra_str = std::getenv("BACKCGRA");
  if (back_cgra_str != NULL) {
    _back_cgra = true;
  }
 
  const char *linear_spad_str = std::getenv("LINEAR_SCR");
  if (linear_spad_str != NULL) {
    _linear_spad = true;
  }
  _banked_spad_mapping_strategy = std::getenv("MAPPING");

  _ssconfig = new SSModel(ssconfig_file);
  // cout << "Came here to create a new configuration for a new accel\n";
  _ssconfig->setMaxEdgeDelay(_fu_fifo_len);
  _port_interf.initialize(_ssconfig);

  scratchpad.resize(SCRATCH_SIZE);
  if (_linear_spad) {
    scratchpad.resize(SCRATCH_SIZE + LSCRATCH_SIZE);
    // scratchpad_readers.resize(SCRATCH_SIZE+LSCRATCH_SIZE);
    // scratchpad_writers.resize(SCRATCH_SIZE+LSCRATCH_SIZE);
  }

  // optionally used -- this is expensive
  // TODO: make it compat with linear scr case
  scratchpad_readers.resize(SCRATCH_SIZE);
  scratchpad_writers.resize(SCRATCH_SIZE);

  // FIXME: schedule the implicit network stream in the each accel
  if (i == 0) { // required only in 1st accel
    remote_core_net_stream_t *implicit_stream = new remote_core_net_stream_t();

    port_data_t *in_addr = &_port_interf.in_port(NET_ADDR_PORT);
    port_data_t *in_val = &_port_interf.in_port(NET_VAL_PORT);
    // assert(out_vp->can_take(LOC::PORT) && !blocked_ovp[NET_ADDR_PORT]);
    // assert(out_vp2->can_take(LOC::PORT) && !blocked_ovp[NET_VAL_PORT]);
    _scr_w_c.schedule_network_stream(*implicit_stream);
    in_addr->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
    in_val->set_status(port_data_t::STATUS::BUSY, LOC::PORT);

    // push_net_in_cmd_queue(implicit_stream);
  }
}

void accel_t::timestamp() {
  _ssim->timestamp();
  cout << "acc" << _accel_index << " ";
}

bool accel_t::in_use() { return _ssim->in_use(); }
bool accel_t::can_add_stream() {
  _ssim->set_in_use();
  return _cmd_queue.size() < _queue_size;
}

bool accel_t::in_roi() { return _ssim->in_roi(); }

// The job of whos_to_blame(group) is to determine the reason of whether
// a group is able to issue or not.
// It's not possible to determine if
// there is data/stream for one port, but no data/stream for another, why
// that is (is it scatchpad or core?), so that's not answered here.
pipeline_stats_t::PIPE_STATUS accel_t::whos_to_blame(int group) {
  bool any_empty_fifo = false, any_scr = false, any_dma = false,
       any_const = false, any_rec = false;

  auto &active_ports = _soft_config.in_ports_active_group[group];
  auto &active_out_ports = _soft_config.out_ports_active_group[group];

  if (active_ports.size() == 0)
    return pipeline_stats_t::NOT_IN_USE;

  if (_cgra_issued_group[group])
    return pipeline_stats_t::ISSUED;

  // Iterate over inputs
  bool any_input_activity = false;
  bool all_inputs_inactive = true;
  int num_unknown_input = 0;
  for (unsigned i = 0; i < active_ports.size(); ++i) {
    auto &in_vp = _port_interf.in_port(active_ports[i]);
    bool assigned_ivp = in_vp.in_use() || in_vp.completed();
    bool empty_fifo = in_vp.any_data() == 0;
    any_empty_fifo |= empty_fifo;
    if (empty_fifo && assigned_ivp) {
      switch (in_vp.loc()) {
      case LOC::DMA:
        any_dma = true;
        break;
      case LOC::SCR:
        any_scr = true;
        break;
      case LOC::PORT:
        any_rec = true;
        break;
      case LOC::CONST:
        any_const = true;
        break;
      default:
        break;
      }
    }
    num_unknown_input += (empty_fifo && !assigned_ivp);
    any_input_activity |= assigned_ivp || !empty_fifo;
    all_inputs_inactive = all_inputs_inactive && empty_fifo && !assigned_ivp;
  }

  if (num_unknown_input == 0) {
    if (!any_empty_fifo)
      return pipeline_stats_t::CGRA_BACK;
    if (any_const)
      return pipeline_stats_t::CONST_FILL;
    if (any_scr)
      return pipeline_stats_t::SCR_FILL;
    if (any_dma)
      return pipeline_stats_t::DMA_FILL;
    if (any_rec)
      return pipeline_stats_t::REC_WAIT;
  }

  for (unsigned i = 0; i < active_out_ports.size(); ++i) {
    // This is just a guess really, but things really are bad
    // if memory write is the bottleneck...
    auto &out_vp = _port_interf.out_port(active_out_ports[i]);
    if (out_vp.any_data()) {
      if (!_lsq->sd_transfers[MEM_WR_STREAM].canReserve()) {
        return pipeline_stats_t::DMA_WRITE;
      }
    }
  }

  bool bar_scratch_read = false; // These are set by scratch
  bool bar_scratch_write = false;
  bool bar_remote_scratch_write = false;
  std::set<int> scratch_waiters;
  std::set<int> waiters;
  for (auto i = _cmd_queue.begin(); i != _cmd_queue.end(); ++i) {
    base_stream_t *ip = i->get();
    if (auto stream = dynamic_cast<stream_barrier_t *>(ip)) {
      bar_scratch_read |= stream->bar_scr_rd();
      bar_scratch_write |= stream->bar_scr_wr();
      bar_remote_scratch_write |= stream->bar_scr_wr_df(); // it has no waiters
    }

    for (int port : ip->in_ports()) {
      port_data_t &in_vp = _port_interf.in_port(port);
      auto it = std::find(active_ports.begin(), active_ports.end(), port);
      if (it == active_ports.end())
        continue;

      if (!(in_vp.in_use() || in_vp.completed() || in_vp.any_data())) {
        if (bar_scratch_write && ((ip->src() & LOC::SCR) != LOC::NONE)) {
          scratch_waiters.insert(port);
        } else {
          waiters.insert(port);
        }
      }
    }
  }

  // if all of our unknowns are waiting for us, then maybe its scratch or cmd
  // queue
  unsigned total_waiters = scratch_waiters.size() + waiters.size();
  if (total_waiters == num_unknown_input) {
    if (scratch_waiters.size()) {
      return pipeline_stats_t::SCR_BAR_WAIT;
    }
    return pipeline_stats_t::CMD_QUEUE;
  }
  if (scratch_waiters.size() && _cmd_queue.size() >= _queue_size) {
    return pipeline_stats_t::SCR_BAR_WAIT;
  }

  any_input_activity |= (total_waiters); // waiters counts as input activity

  bool any_out_activity = false, any_cgra_activity = false;
  for (unsigned i = 0; i < active_out_ports.size(); ++i) {
    auto &out_vp = _port_interf.out_port(active_out_ports[i]);
    bool assigned_ovp = out_vp.in_use() || out_vp.completed();
    any_cgra_activity |= out_vp.num_in_flight();
    any_out_activity |= assigned_ovp || out_vp.any_data();
  }

  // No activity on this group, so ignore it
  if (!any_input_activity && !any_cgra_activity && !any_out_activity) {
    return pipeline_stats_t::NOT_IN_USE;
  }

  if (!any_input_activity) { // no input, but at least some cgra/out
    return pipeline_stats_t::DRAIN;
  }

  // some input activity

  return pipeline_stats_t::CORE_WAIT;
  // cout << num_unassigned << "/" << total_ivps << "-" <<
  // num_unassigned_queued; done(true,0);
}

// Figure out who is to blame for a given cycle's not issuing
void accel_t::whos_to_blame(
    std::vector<pipeline_stats_t::PIPE_STATUS> &blame_vec,
    std::vector<pipeline_stats_t::PIPE_STATUS> &group_vec) {
  if (_soft_config.in_ports_active.size() == 0) {
    blame_vec.push_back(pipeline_stats_t::NOT_IN_USE);
    return;
  }

  if (_in_config) {
    blame_vec.push_back(pipeline_stats_t::CONFIG);
    return;
  }

  bool draining = false, cgra_back = false;

  // Temp vec are for reasons that we consider to have priority
  // when considering blame over draining and backpressure.
  std::vector<pipeline_stats_t::PIPE_STATUS> temp_vec;

  for (int g = 0; g < NUM_GROUPS; ++g) {
    if (_dfg->group_prop(g).is_temporal)
      continue;

    pipeline_stats_t::PIPE_STATUS blame = whos_to_blame(g);
    group_vec.push_back(blame);
    switch (blame) {
    case pipeline_stats_t::DRAIN:
      draining = true;
      break;
    case pipeline_stats_t::CGRA_BACK:
      cgra_back = true;
      break;
    case pipeline_stats_t::NOT_IN_USE:
      break;
    default:
      temp_vec.push_back(blame);
      break;
    }
  }

  if (_dedicated_cgra_issued > 1) {
    blame_vec.push_back(pipeline_stats_t::ISSUED_MULTI);
    return;
  }
  if (_dedicated_cgra_issued == 1) {
    blame_vec.push_back(pipeline_stats_t::ISSUED);
    return;
  }

  if (_backcgra_issued != 0) {
    blame_vec.push_back(pipeline_stats_t::TEMPORAL_ONLY);
    return;
  }

  if (temp_vec.size() == 0) {
    if (draining) {
      blame_vec.push_back(pipeline_stats_t::DRAIN);
      return;
    }
    if (cgra_back) {
      blame_vec.push_back(pipeline_stats_t::CGRA_BACK);
      return;
    }
    blame_vec.push_back(pipeline_stats_t::CORE_WAIT);
    return;
  }

  for (auto i : temp_vec) {
    blame_vec.push_back(i);
  }

  // bool any_stream_active =
  //   _dma_c.any_stream_active() || _scr_r_c.any_stream_active() ||
  //   _port_c.any_stream_active() || _scr_w_c.any_stream_active();

  // bool any_buf_active = _scr_r_c.buf_active() || _scr_w_c.buf_active();

  // bool mem_rds = _dma_c.mem_reads_outstanding();
  // bool mem_wrs = _dma_c.mem_writes_outstanding();
  // bool scr_req = _dma_c.scr_reqs_outstanding();

  // bool cgra_input = cgra_input_active();
  // bool cgra_compute = cgra_compute_active();
  // bool cgra_output = cgra_output_active();
  // bool cgra_active = cgra_input || cgra_compute || cgra_output;

  // bool queue = _cmd_queue.size();

  // bool busy = any_stream_active || any_buf_active || cgra_active ||
  //               mem_wrs || mem_rds || scr_req || queue;

  // if(!busy) return pipeline_stats_t::NO_ACTIVITY;
}

void accel_t::tick() {
  
  _cgra_issued = 0; // for statistics reasons
  _dedicated_cgra_issued = 0;
  _backcgra_issued = 0;

  for (int i = 0; i < NUM_GROUPS; ++i) {
    _cgra_issued_group[i] = false;
  }

  _dma_c.cycle();

  // HACK: logic such that only atomic or rd/wr occurs in 1 cycle
  bool performed_atomic_scr = false;
  bool performed_read = false;

  if (_scr_ctrl_turn == 0) {
    _scr_w_c.cycle(true, performed_atomic_scr);
    if (!performed_atomic_scr) {
      _scr_r_c.cycle(performed_read);
    }
  }
  if (_scr_ctrl_turn == 1) {
    _scr_r_c.cycle(performed_read);
    _scr_w_c.cycle(!performed_read, performed_atomic_scr);
  }
  _scr_ctrl_turn = (_scr_ctrl_turn + 1) % 2;
  // TODO: remove this after removing the above logic
  _scr_r_c.linear_scratch_cycle();
  _scr_w_c.linear_scratch_write_cycle();

  _port_c.cycle();
  _net_c.cycle();
  cycle_in_interf();
  cycle_cgra();

  uint64_t cur_cycle = now();

  for (int i = 0; i < NUM_GROUPS; ++i) {
    std::vector<bool> &prev_issued_group = _cgra_prev_issued_group[i];
    if (prev_issued_group.size() > 0) {
      int mod_index = cur_cycle % prev_issued_group.size();
      prev_issued_group[mod_index] = _cgra_issued_group[i];
    }
  }

  if (in_roi()) {
    get_ssim()->update_stat_cycle();

    if (SS_DEBUG::CYC_STAT && _accel_index == SS_DEBUG::ACC_INDEX) {
      if (_back_cgra) {
        cycle_status_backcgra();
      } else {
        cycle_status();
      }
    }

    std::vector<pipeline_stats_t::PIPE_STATUS> blame_vec;
    std::vector<pipeline_stats_t::PIPE_STATUS> group_vec;

    whos_to_blame(blame_vec, group_vec);
    if (SS_DEBUG::BLAME && _accel_index == SS_DEBUG::ACC_INDEX &&
        group_vec.size() && (blame_vec.size() != group_vec.size())) {
      for (int g = 0; g < group_vec.size(); ++g) {
        if (_soft_config.in_ports_active_group[g].size()) {
          cout << " " << pipeline_stats_t::name_of(group_vec[g]);
        }
      }
      cout << " >";
    }

    for (auto i : blame_vec) {
      if (SS_DEBUG::BLAME && _accel_index == SS_DEBUG::ACC_INDEX) {
        cout << " " << pipeline_stats_t::name_of(i);
      }

      _pipe_stats.pipe_inc(i, 1.0f / (float)blame_vec.size());
    }

    if ((SS_DEBUG::CYC_STAT || SS_DEBUG::BLAME) &&
        _accel_index == SS_DEBUG::ACC_INDEX) {
      cout << "\n";
    }

    // if(_accel_index==0) {
    //  if(new_now!=_prev_roi_clock+1) {
    //    cout << "\n\n";
    //  }
    //  timestamp();
    //  cout << pipeline_stats_t::name_of(who) << "\n";
    //}

    //_prev_roi_clock=new_now;
  }

  cycle_out_interf();
  cycle_indirect_interf();
  schedule_streams();
  _waiting_cycles++;

  _dma_c.finish_cycle();
  _scr_w_c.finish_cycle();
  _scr_r_c.finish_cycle();
  _net_c.finish_cycle();

  if(_stream_cleanup_mode) {
    request_reset_streams();
  }
}

// forward from indirect inputs to indirect outputs -- this makes the protocol
// simpler than it used to be
void accel_t::cycle_indirect_interf() {
  for (unsigned i = START_IND_PORTS; i < STOP_IND_PORTS; ++i) { // ind read ports
    auto &ind_in_port = _port_interf.in_port(i);
    auto &ind_out_port = _port_interf.out_port(i);
    
    // we only allow 8 words in the output vector port, since there's already
    // buffering in the input vector port, so it would be kind of cheating
    while (ind_in_port.mem_size() && ind_out_port.mem_size() < 64) {
      ind_out_port.push_data(ind_in_port.pop_in_custom_data<uint8_t>());
    }
  }
}

void accel_t::cycle_in_interf() {
  for (unsigned i = 0; i < _soft_config.in_ports_active.size(); ++i) {
    int cur_port = _soft_config.in_ports_active[i];
    _port_interf.in_port(cur_port).reformat_in_one_vec();
  }
}

void accel_t::cycle_out_interf() {
  for (unsigned i = 0; i < _soft_config.out_ports_active.size(); ++i) {
    int cur_port = _soft_config.out_ports_active[i];
    _port_interf.out_port(cur_port).reformat_out_one_vec();
  }
}

bool accel_t::can_receive(int out_port) {
  port_data_t &out_vp = port_interf().out_port(out_port);
  // Make sure a stream is not using the data, and also that there exists data
  return !out_vp.in_use() && out_vp.mem_size() != 0;
}

uint64_t accel_t::receive(int out_port) {
  port_data_t &out_vp = port_interf().out_port(out_port);
  SBDT val = out_vp.pop_out_data();
  // out_vp.reset_data(); // remove later
  if (SS_DEBUG::COMMAND || SS_DEBUG::COMMAND_I || SS_DEBUG::COMMAND_O) {
    timestamp();
    cout << "SS_RECV value: " << val << " on port" << out_vp.port() << " " << out_vp.mem_size()
         << " on core: " << _lsq->getCpuId() << "\n";
  }
  return val;
}

// Simulate the portion of the dataflow graph which requires either
// 1. backpressure, or 2. temporal sharing.
// Ports relevant for this simulation this are called "active_in_ports_bp"

void accel_t::cycle_cgra_backpressure() {
  bool print = false;
  if (SS_DEBUG::COMP) {
    print = true;
    // timestamp();
    // *cgra_dbg_stream  <<"\n";
  }

  int num_computed = 0;
  auto &active_in_ports = _soft_config.in_ports_active_backcgra;

  // for stats, this may make simulation slower--have separate mode for it: TODO
  int count_avail;
  count_avail = 0;
  for (int i = 0; i < active_in_ports.size(); ++i) {
    int port_index = active_in_ports[i];
    auto &cur_in_port = _port_interf.in_port(port_index);
    SSDfgVecInput *vec_in = dynamic_cast<SSDfgVecInput *>(
        _sched->vportOf(make_pair(true /*input*/, port_index)));
    assert(vec_in != NULL && "input port pointer is null\n");
    // flag is true if data is available at this port
    if (cur_in_port.num_ready()) {
      count_avail++;
    }
    if (_dfg->can_push_input(vec_in)) {
      _slot_avail[i]++;
      if (!cur_in_port.num_ready()) {
        _could_not_serve[i]++;
      }
    }
  }
  // when data is not available at any of the input ports => not an issue of
  // control core: should return 1 (when none is available, memory bandwidth
  // bottleneck)
  double ratio = (double)count_avail / (double)active_in_ports.size();
  _stat_ss_data_avail_ratio += std::max(ratio, 1 - ratio);

  for (int i = 0; i < active_in_ports.size(); ++i) {
    int port_index = active_in_ports[i];
    auto &cur_in_port = _port_interf.in_port(port_index);
        
    auto *port = _sched->vportOf(make_pair(true /*input*/, port_index));
    auto *vec_in = dynamic_cast<SSDfgVecInput *>(port); 
    // cout << "REPEAT FLAG: " << cur_in_port.repeat_flag() << " and num_repeated_till_now: " << cur_in_port.num_times_repeated() << endl;
    // TODO: make it compatible for different datawidth of the port and repeat
    // count
    if(cur_in_port.repeat_flag() && cur_in_port.num_times_repeated()==0 && cur_in_port.num_ready() && _dfg->can_push_input(vec_in)) { // need to pop to set it
      auto &repeat_prt = port_interf().out_port(cur_in_port.repeat());
      if(repeat_prt.mem_size()) {
        uint64_t x = repeat_prt.pop_out_data();
        if(SS_DEBUG::MEM_REQ) {
          // cout << "Port details: " << cur_in_port.port_cgra_elem() << " " << vec_in->logical_len() << " ( " << vec_in->name() << " ) "<< endl;
          // cout << "NEW REPEAT COUNT: " << x << endl;
        }
        int y = (x)*(1<<REPEAT_FXPNT);
        cur_in_port.set_cur_repeat_lim(y);
      } else { // sstream_size data is not yet available
        cur_in_port.set_cur_repeat_lim(-1*(1<<REPEAT_FXPNT));
      }
    }

        
    // cout << "Port details: " << cur_in_port.port_cgra_elem() << " " << vec_in->logical_len() << " ( " << vec_in->name() << " ) " << " cur_repeat_lim: " << cur_in_port.cur_repeat_lim() << " and num_ready: " << cur_in_port.num_ready() << " and mem size: " << cur_in_port.mem_size() << endl;
    
    if (cur_in_port.num_ready() && cur_in_port.cur_repeat_lim()>0) {
      // auto *port = _sched->vportOf(make_pair(true /*input*/, port_index));
      // auto *vec_in = dynamic_cast<SSDfgVecInput *>(port);
      assert(vec_in != NULL && "input port pointer is null\n");

      if (_dfg->can_push_input(vec_in)) {
        forward_progress();

        // execute_dfg
        vector<SBDT> data;
        vector<bool> data_valid;
        SBDT val = 0;
        bool valid = false;

        // if(cur_in_port.port_cgra_elem()!=vec_in->length()){
        if(cur_in_port.port_cgra_elem()!=vec_in->logical_len()){
          cout << cur_in_port.port_cgra_elem() << " " << vec_in->logical_len() << " ( " << vec_in->name() << " ) "<< endl;
        }
        assert(cur_in_port.port_cgra_elem()==vec_in->logical_len() && "Vector size not same in scheduler and simulator");
        // cout << "Logical length: " << vec_in->logical_len() << " ( " << vec_in->name() << " ) "<< endl;

        // cout << "Initial size in dgra connector: " << cur_in_port.port_cgra_elem() << endl;
        for (unsigned port_idx = 0; port_idx < cur_in_port.port_cgra_elem();
             ++port_idx) { // port_idx are the scalar cgra nodes
          int cgra_port = cur_in_port.cgra_port_for_index(port_idx);
          if (_soft_config.cgra_in_ports_active[cgra_port] == false) {
            break;
          }

          // get the data of the instance of CGRA FIFO
          val = cur_in_port.value_of(port_idx, 0);
          valid = cur_in_port.valid_of(port_idx, 0);
          data_valid.push_back(valid);
          data.push_back(val);
        }

        // TODO: this is supposed to be the verif flag, fix it later!
        num_computed = _dfg->push_vector(vec_in, data, data_valid, print, true);
        data.clear();       // clear the input data pushed to dfg
        data_valid.clear(); // clear the input data pushed to dfg

        // pop input from CGRA port after it is pushed into the dfg node
        if (!vec_in->backPressureOn()) {
          bool should_pop = cur_in_port.inc_repeated();
          // cout << "Vec name: " << vec_in->name() << " and should pop? " << should_pop << endl;
          if (should_pop) {
            cur_in_port.pop(1);
          }
        }

        if(SS_DEBUG::COMP) {
          cout << "Vec name: " << vec_in->name() << "Allowed to push input: " << data[0] << " " << data[1] << " " << data[2] << " " << data[3] << "\n";
        }

      }
    } else if(cur_in_port.num_ready() && cur_in_port.cur_repeat_lim()==0) { // when the element has to be repeated 0 times (discard)
      // cout << "Came inside repeat=0 for Vec name: " << vec_in->name() << "\n";
      cur_in_port.pop(1);
      }
  }

  // calling with the default parameters for now
  num_computed = _dfg->cycle(print, true);
  if (num_computed) {
    _cgra_issued++;
    _backcgra_issued++;
  }

  if (in_roi()) {
    _stat_ss_insts += num_computed;
    _stat_ss_dfg_util += (double)num_computed / _dfg->ordered_insts().size();
  }

  // pop the ready outputs
  auto &active_out_ports = _soft_config.out_ports_active;
  vector<SBDT> data;
  vector<bool> data_valid;

  for (int i = 0; i < active_out_ports.size(); ++i) {
    int port_index = active_out_ports[i];
    auto &cur_out_port = _port_interf.out_port(port_index);
       
    SSDfgVec *vec_out =
        _sched->vportOf(make_pair(false /*output*/, port_index));
    SSDfgVecOutput *vec_output = dynamic_cast<SSDfgVecOutput *>(vec_out);
    assert(vec_output != NULL && "output port pointer is null\n");

    int data_width = cur_out_port.get_port_width();
 
    // we need to send the real vec_outputs
    int len = ceil(cur_out_port.port_vec_elem()*cur_out_port.get_port_width()/float(8));
    // cout << "Length of output required: " << len << endl;

    if (_dfg->can_pop_output(vec_output, len)) {
      data.clear();       // make sure it is empty here
      data_valid.clear(); // make sure it is empty here
      _dfg->pop_vector_output(vec_output, data, data_valid, len, print, true);

      if(SS_DEBUG::COMP) {
        cout << "Output vector name: " << vec_output->name() << endl;
        cout << "Allowed to pop output: " << data[0] << " " << data[1] << "\n";
      }
      if (in_roi()) {
        // _stat_comp_instances += 1;
        _stat_comp_instances += len; // number of scalar inputs
      }

      int j = 0, n_times=0;
      for (j = 0; j < len; ++j) {
        // push the data to the CGRA output port only if discard is not 0
        if (data_valid[j]) {

          n_times = 8/data_width;
          if(vec_output->logical_len()-j*n_times>0) {
            n_times = std::min(n_times, (int)vec_output->logical_len()-j*n_times);
          }
          // push elements from dfg output port
          for(int k=0; k<n_times; ++k) {
            vector<uint8_t> v = cur_out_port.get_byte_vector(
              data[j] >> (k*data_width*8), data_width);
            cur_out_port.push_data(v);
          }
          // cur_out_port.inc_ready(1); // FIXME:CHECKME: I hope this is correct!
        } 
      }
    }
  }

  // Statistics to measure port imbalance
  if(_ssim->in_roi() && _ssim->in_use()) { // would measure only when something was issued
    _stat_port_imbalance += _dfg->count_starving_nodes();
  }
}

// FIXME: this doesn't work with dgra
void accel_t::cycle_cgra_fixedtiming() {
  uint64_t cur_cycle = now();
  for (int group = 0; group < NUM_GROUPS; ++group) {
    if (_dfg->group_prop(group).is_temporal)
      continue; // break out
    std::vector<bool> &prev_issued_group = _cgra_prev_issued_group[group];
    // int mod_index = cur_cycle%prev_issued_group.size();

    // detect if we need to wait for throughput reasons on CGRA group -- easy
    // peasy if(cur_cycle < _delay_group_until[group]) {
    //  continue;
    //}
    int num_issue = 0;
    for (int i = 0; i < prev_issued_group.size(); ++i) {
      num_issue += prev_issued_group[i];
    }
    if (num_issue >= _soft_config.group_thr[group].first) {
      continue;
    }

    // Detect if we are ready to fire
    auto &active_ports = _soft_config.in_ports_active_group[group];

    if (active_ports.size() == 0)
      continue;
    unsigned min_ready = 10000000;
    for (int i = 0; i < active_ports.size(); ++i) {
      int cur_port = active_ports[i];
      min_ready =
          std::min(_port_interf.in_port(cur_port).num_ready(), min_ready);
      // if(min_ready==0 && _accel_index==0) {
      //  timestamp(); cout << "Port " << cur_port << "not ready\n";
      //}
    }

    // Now fire on all cgra ports
    if (min_ready > 0) {
      forward_progress();
      //_delay_group_until[group]=cur_cycle+_soft_config.group_thr[group];
      execute_dfg(0, group); // Note that this will set backpressure variable

      // if(in_roi()) {
      //  _stat_ss_insts+=_dfg->num_insts();
      //}
      // pop the elements from inport as they have been processed
      for (unsigned i = 0; i < active_ports.size(); ++i) {
        uint64_t port_index = active_ports[i];
        port_data_t &in_port = _port_interf.in_port(port_index);

        auto *port = _sched->vportOf(make_pair(true /*input*/, port_index));
        auto *vec_in = dynamic_cast<SSDfgVecInput *>(port);
        // skip popping if backpressure is on
        if (!vec_in->backPressureOn()) {
          // only increment repeated if no backpressure
          bool should_pop = in_port.inc_repeated();
          if (should_pop) {
            in_port.pop(1);
          }
        }
      }
    }
  }

  // some previously produced outputs might be ready at this point,
  // so, lets quickly check. (could be broken out as a separate function)
  if (!_cgra_output_ready.empty()) {
    auto iter = _cgra_output_ready.begin();
    if (cur_cycle >= iter->first) {

      for (auto &out_port_num : iter->second) {
        _port_interf.out_port(out_port_num).set_out_complete();
      }
      _cgra_output_ready.erase(iter); // delete from list
    }
  }
}

void accel_t::cycle_cgra() {
  if (!_dfg)
    return;

  // printf("Going to compute with ACCEL ID: %d\n", _lsq->getCpuId());
  if (_back_cgra) {
    cycle_cgra_backpressure();
  } else {
    cycle_cgra_backpressure();
    cycle_cgra_fixedtiming();
  }
  if (in_roi()) {
    _stat_cgra_busy_cycles += (_cgra_issued > 0);
  }

  if (_cgra_issued > 0 && SS_DEBUG::COMP && SS_DEBUG::NET_REQ) {
    printf("ACCEL ID: %d\n", _lsq->getCpuId());
  }
}

void accel_t::execute_dfg(unsigned instance, int group) {
  if (SS_DEBUG::COMP) {
    *_cgra_dbg_stream << "inputs (group" << group << "):";
  }
  _dfg->set_dbg_stream(_cgra_dbg_stream);

  auto &active_ports = _soft_config.in_ports_active_group[group];

  // timestamp(); cout << " EXECUTE ACCEL " << accel_index() << "\n";

  // send each fifo's data to corresponding dfg input
  for (unsigned i = 0; i < active_ports.size(); ++i) {
    auto &cur_in_port = _port_interf.in_port(active_ports[i]);

    // for each active vector port
    for (unsigned port_idx = 0; port_idx < cur_in_port.port_cgra_elem();
         ++port_idx) {
      int cgra_port = cur_in_port.cgra_port_for_index(port_idx);
      if (_soft_config.cgra_in_ports_active[cgra_port] == false) {
        continue;
      }

      // get the data of the instance of CGRA FIFO
      SBDT val = cur_in_port.value_of(port_idx, instance);
      bool valid = cur_in_port.valid_of(port_idx, instance);

      // for each cgra port and associated dfg input
      _soft_config.input_dfg_node[group][i][port_idx]->set_value(val, valid);

      if (SS_DEBUG::COMP) {
        if (valid)
          *_cgra_dbg_stream << std::hex << val << ", " << std::dec;
        else
          *_cgra_dbg_stream << "inv, ";
      }

      if (SS_DEBUG::VERIF_PORT) {
        in_port_verif << hex << setw(16) << setfill('0') << val << " ";
      }
    }
  }

  if (in_roi()) {
    for (auto i : _soft_config.inst_histo) {
      _total_histo[i.first] += i.second;
    }
    for (unsigned i = 0; i < _soft_config.in_ports_active.size(); ++i) {
      unsigned cur_p = _soft_config.in_ports_active[i];
      _vport_histo[_port_interf.in_port(cur_p).port_cgra_elem()]++;
    }
    for (unsigned i = 0; i < _soft_config.out_ports_active.size(); ++i) {
      unsigned cur_p = _soft_config.out_ports_active[i];
      _vport_histo[_port_interf.out_port(cur_p).port_cgra_elem()]++;
    }
  }

  bool print = false;
  if (SS_DEBUG::COMP) {
    print = true;
    *_cgra_dbg_stream << "\n";
  }

  if (SS_DEBUG::VERIF_PORT) {
    in_port_verif << "\n";
  }

  // perform computation
  int num_computed = _dfg->compute(print, SS_DEBUG::VERIF_CGRA, group);

  if (in_roi()) {
    _stat_ss_insts += num_computed;
  }

  uint64_t cur_cycle = now();

  _cgra_issued_group[group] = true;
  _cgra_issued++;
  _dedicated_cgra_issued++;

  auto &active_out_ports = _soft_config.out_ports_active_group[group];

  // send outputs to corresponding output fifo
  // TODO: Impelement backpressure in simulator here!
  for (unsigned i = 0; i < active_out_ports.size(); ++i) {
    int pnum = active_out_ports[i];
    auto &cur_out_port = _port_interf.out_port(pnum);

    int num_discarded = 0;

    for (unsigned port_idx = 0; port_idx < cur_out_port.port_cgra_elem();
         ++port_idx) {
      // int cgra_port = cur_out_port.cgra_port_for_index(port_idx);
      SSDfgOutput *n = _soft_config.output_dfg_node[group][i][port_idx];

      uint64_t val = n->retrieve();
      bool valid = !n->parent_invalid();
      cur_out_port.push_cgra_port(port_idx, val,
                                  valid); // retreive from last inst

      if (SS_DEBUG::COMP) {
        *_cgra_dbg_stream << "output:" << hex << val << ", valid:" << valid
                          << dec << "\n";
      }

      if (SS_DEBUG::VERIF_PORT) {
        out_port_verif << hex << setw(16) << setfill('0') << val << " ";
      }
      num_discarded += !valid;
    }

    cur_out_port.inc_ready(1); // we just did one instance
    cur_out_port.set_in_flight();
    int lat = _soft_config.out_ports_lat[pnum];
    _cgra_output_ready[cur_cycle + lat].push_back(pnum);
  }

  if (SS_DEBUG::VERIF_PORT) {
    out_port_verif << "\n";
  }

  if (in_roi()) {
    _stat_comp_instances += 1;
  }
}

// Print out a string on one line indicating hardware status for the previous
// cycle
// Buffer Sizes                                     |      Bus Activity
// ip 1:5 2:5 7:7; op 1:2 scr_wr:1 cq:1 mem_req:14  | ip: op: scr_rd: scr_wr:
// mr: mw:
void accel_t::cycle_status() {
  if (_soft_config.in_ports_active.size() == 0) {
    return;
  }

  timestamp();
  cout << "cq" << _cmd_queue.size();

  for (int group = 0; group < NUM_GROUPS; ++group) {
    auto &active_ports = _soft_config.in_ports_active_group[group];
    if (active_ports.size()) {
      cout << "|";
      for (unsigned i = 0; i < active_ports.size(); ++i) {
        unsigned cur_p = active_ports[i];
        cout << "i" << cur_p << ":" << _port_interf.in_port(cur_p).mem_size()
             << "," << _port_interf.in_port(cur_p).num_ready();
        auto &in_port = _port_interf.in_port(active_ports[i]);
        if (in_port.in_use()) {
          cout << base_stream_t::loc_short_name(in_port.loc());
          if (in_port.completed()) {
            cout << "#";
          }
        }
        cout << " ";
      }
    }
  }

  // for(unsigned i = START_IND_PORTS; i < STOP_IND_PORTS; ++i ) {  //ind read
  // ports
  //  unsigned cur_p = i;
  //  cout << " " << cur_p << ": "
  //            << _port_interf.in_port(cur_p).mem_size()  << ","
  //            << _port_interf.in_port(cur_p).num_ready() <<" ";
  //}
  cout << "\t";
  for (int group = 0; group < NUM_GROUPS; ++group) {
    auto &active_ports = _soft_config.out_ports_active_group[group];
    if (active_ports.size()) {
      cout << "|";
      for (unsigned i = 0; i < active_ports.size(); ++i) {
        unsigned cur_p = active_ports[i];
        auto &out_port = _port_interf.out_port(cur_p);
        cout << "o" << cur_p << ":" << out_port.num_in_flight() << "-"
             << out_port.num_ready() << "," << out_port.mem_size();
        if (out_port.in_use()) {
          cout << base_stream_t::loc_short_name(out_port.loc());
        }
        cout << " ";
      }
    }
  }

  cout << "m_req:" << _dma_c.mem_reqs() << " ";

  cout << "\t|";

  //  cout << "req:"
  cout << "s_rd" << _stat_scr_bytes_rd << " s_wr:" << _stat_scr_bytes_wr
       << " m_rd:" << _stat_mem_bytes_rd << " m_wr:" << _stat_mem_bytes_wr
       << " ";
  //  cout << "sat:" << " m_rd:" << _stat_mem_bytes_rd_sat << " ";
  //                 << " m_wr:" << _stat_mem_bytes_wr_sat;

  // Just the indirect ports
  //  for(unsigned i = 24; i < 32; ++i) {
  //    int cur_p=i;
  //    if(_port_interf.in_port(cur_p).in_use()) {
  //      cout << cur_p << " "  <<
  //      (_port_interf.in_port(cur_p).completed()?"(completed)":"");
  //    }
  //  }

  // Just the indirect ports
  //  for(unsigned i = 24; i < 32; ++i) {
  //    int cur_p=i;
  //    if(_port_interf.out_port(cur_p).in_use()) {
  //      cout << cur_p << " " <<
  //      (_port_interf.out_port(cur_p).completed()?"(completed)":"");
  //    }
  //  }

  clear_cycle();
}

void accel_t::cycle_status_backcgra() {
  if (_soft_config.in_ports_active_backcgra.size() == 0) {
    return;
  }

  timestamp();
  cout << "cq" << _cmd_queue.size();
  auto &active_in_ports = _soft_config.in_ports_active_backcgra;

  if (active_in_ports.size()) {
    cout << "|";
    for (unsigned i = 0; i < active_in_ports.size(); ++i) {
      unsigned cur_p = active_in_ports[i];
      cout << "i" << cur_p << ":" << _port_interf.in_port(cur_p).mem_size()
           << "," << _port_interf.in_port(cur_p).num_ready();
      auto &in_port = _port_interf.in_port(active_in_ports[i]);
      if (in_port.in_use()) {
        cout << base_stream_t::loc_short_name(in_port.loc());
        if (in_port.completed()) {
          cout << "#";
        }
      }
      cout << " ";
    }
  }

  // for(unsigned i = START_IND_PORTS; i < STOP_IND_PORTS; ++i ) {  //ind read
  // ports
  //  unsigned cur_p = i;
  //  cout << " " << cur_p << ": "
  //            << _port_interf.in_port(cur_p).mem_size()  << ","
  //            << _port_interf.in_port(cur_p).num_ready() <<" ";
  //}
  cout << "\t";
  auto &active_out_ports = _soft_config.out_ports_active;
  if (active_out_ports.size()) {
    cout << "|";
    for (unsigned i = 0; i < active_out_ports.size(); ++i) {
      unsigned cur_p = active_out_ports[i];
      auto &out_port = _port_interf.out_port(cur_p);
      cout << "o" << cur_p << ":" << out_port.num_in_flight() << "-"
           << out_port.num_ready() << "," << out_port.mem_size();
      if (out_port.in_use()) {
        cout << base_stream_t::loc_short_name(out_port.loc());
      }
      cout << " ";
    }
  }

  cout << "m_req:" << _dma_c.mem_reqs() << " ";

  cout << "\t|";

  //  cout << "req:"
  cout << "s_rd" << _stat_scr_bytes_rd << " s_wr:" << _stat_scr_bytes_wr
       << " m_rd:" << _stat_mem_bytes_rd << " m_wr:" << _stat_mem_bytes_wr
       << " ";
  //  cout << "sat:" << " m_rd:" << _stat_mem_bytes_rd_sat << " ";
  //                 << " m_wr:" << _stat_mem_bytes_wr_sat;

  // Just the indirect ports
  for (unsigned i = 24; i < 32; ++i) {
    int cur_p = i;
    if (_port_interf.in_port(cur_p).in_use()) {
      cout << cur_p << " "
           << (_port_interf.in_port(cur_p).completed() ? "(completed)" : "");
    }
  }

  // Just the indirect ports
  for (unsigned i = 24; i < 32; ++i) {
    int cur_p = i;
    if (_port_interf.out_port(cur_p).in_use()) {
      cout << cur_p << " "
           << (_port_interf.out_port(cur_p).completed() ? "(completed)" : "");
    }
  }
  clear_cycle();
}

void accel_t::clear_cycle() {
  // std::map<int,int> _stat_ivp_put;
  // std::map<int,int> _stat_ivp_get;
  // std::map<int,int> _stat_ovp_put;
  // std::map<int,int> _stat_ovp_get;
  //
  // if(_ssim->in_roi()) {
  //  _stat_tot_mem_stored+=_stat_mem_bytes_wr;
  //}

  _stat_mem_bytes_wr = 0;
  _stat_mem_bytes_rd = 0;
  _stat_scr_bytes_wr = 0;
  _stat_scr_bytes_rd = 0;
  _stat_mem_bytes_wr_sat = 0;
  _stat_mem_bytes_rd_sat = 0;

  //_stat_cmds_issued=0;
  //_stat_cmds_complete=0;
}

void accel_t::print_status() {
  if (done(false, 0)) {
    return;
  }
  if (!_dfg) {
    std::cout << "No DFG configured\n";
    return;
  }
  cout << "---- ACCEL " << _accel_index << " STATUS ----\n";
  cout << "MEM REQs OUTSTANDING: " << _dma_c.mem_reqs() << "\n";
  cout << "Active SEs:\n";
  _dma_c.print_status();
  _scr_r_c.print_status();
  _scr_w_c.print_status();
  _port_c.print_status();
  _net_c.print_status();

  cout << "Waiting SEs: (" << _cmd_queue.size() << ")\n";
  for (auto i : _cmd_queue) {
    i->print_status();
  }

  cout << "Ports:\n";
  for (unsigned i = 0; i < _soft_config.in_ports_active.size(); ++i) {
    unsigned cur_p = _soft_config.in_ports_active[i];
    string &s = _soft_config.in_ports_name[cur_p];
    std::cout << "In Port " << cur_p << " " << s << ": ";
    std::cout << "  Mem Size: " << _port_interf.in_port(cur_p).mem_size() << "";
    std::cout << "  Num Ready: " << _port_interf.in_port(cur_p).num_ready()
              << " Rem writes: " << _port_interf.in_port(cur_p).get_rem_wr() // remove this!
              << "\n";
  }

  for (unsigned i = START_IND_PORTS; i < STOP_IND_PORTS; ++i) { // ind read
                                                                // ports
    unsigned cur_p = i;
    std::cout << "Ind In Port " << cur_p << ": ";
    std::cout << "  Mem Size: " << _port_interf.in_port(cur_p).mem_size() << "";
    std::cout << "  Num Ready: " << _port_interf.in_port(cur_p).num_ready()
              << " Rem writes: " << _port_interf.in_port(cur_p).get_rem_wr() // remove this!
              << "\n";
  }

  for (unsigned i = 0; i < _soft_config.out_ports_active.size(); ++i) {
    unsigned cur_p = _soft_config.out_ports_active[i];
    string &s = _soft_config.out_ports_name[cur_p];
    std::cout << "Out Port " << cur_p << " " << s << ": ";
    std::cout << "  In Flight: " << _port_interf.out_port(cur_p).num_in_flight()
              << "";
    std::cout << "  Num Ready: " << _port_interf.out_port(cur_p).num_ready()
              << "";
    std::cout << "  Mem Size: " << _port_interf.out_port(cur_p).mem_size()
              << "\n";
  }

  done(true, 0); // print why not done
}

// void accel_t::reset_statistics() { //currently unused
//  _stat_comp_instances = 0;
//  _stat_scratch_read_bytes = 0;
//  _stat_scratch_write_bytes = 0;
//  _stat_scratch_reads = 0;
//  _stat_scratch_writes = 0;
//
//  _stat_start_cycle = 0;
//  _stat_stop_cycle = 0;
//  _stat_commands_issued = 0;
//
//  _stat_tot_mem_fetched=0;
//  _stat_tot_mem_stored=0;
//
//  _stat_tot_loads=0;
//  _stat_tot_stores=0;
//  _stat_tot_mem_store_acc=0;
//  _stat_tot_mem_load_acc=0;
//}

void accel_t::pedantic_statistics(std::ostream &out) {
  double l2_acc_per_cyc =
      ((double)(_stat_tot_loads + _stat_tot_stores)) / ((double)roi_cycles());
  out << "L2 accesses per cycle: " << l2_acc_per_cyc << "\n\n";
  double l2_miss_per_cyc =
      ((double)(_stat_tot_mem_load_acc + _stat_tot_mem_store_acc)) /
      ((double)roi_cycles());
  out << "L2 misses per cycle:       " << l2_miss_per_cyc << "\n\n";

  out << "CGRA Activity Histogram (inst/switch:times used)\n";
  for (auto i : _total_histo) {
    out << name_of_inst(i.first) << ":" << i.second << "\n";
  }
  out << "\n Port Activity Histogram (size:times used)\n ";
  for (auto i : _vport_histo) {
    out << i.first << ":" << i.second << "\n";
  }

  out << "\n Stream Length Statistics\n";
  _stream_stats.print(out);

  out.flush();
}

uint64_t accel_t::roi_cycles() { return _ssim->roi_cycles(); }

void accel_t::print_statistics(std::ostream &out) {
  if (SS_DEBUG::VERIF_PORT) { // flush all the log files
    in_port_verif.flush();
    out_port_verif.flush();
  }
  if (SS_DEBUG::VERIF_SCR) {
    scr_rd_verif.flush();
    scr_wr_verif.flush();
  }
  if (SS_DEBUG::VERIF_CMD) {
    cmd_verif.flush();
  }
  if (SS_DEBUG::VERIF_CGRA) {
    cgra_multi_verif.flush();
  }

  if (SS_DEBUG::SUPRESS_STATS) {
    return; // If we don't want to print stats
  }

  out << "\nACCEL " << _accel_index << " STATS ***\n";
  out.precision(4);
  out << dec;
  // out << "Start Cycle: " << _stat_start_cycle << "\n";
  // out << "Stop  Cycle: " << _stat_stop_cycle << "\n\n";
  // out << "BYTES READ AT PORT 5: " << _bytes_rd5 << "\n";

  out << "Commands Issued: " << _stat_commands_issued << "\n";
  out << "CGRA Instances: " << _stat_comp_instances << " -- Activity Ratio: "
      << ((double)_stat_cgra_busy_cycles) / ((double)roi_cycles())
      << ", DFGs / Cycle: "
      << ((double)_stat_comp_instances) / ((double)roi_cycles()) << "\n";
  out << "For backcgra, Average thoughput of all ports (overall): "
    <<
    ((double)_stat_comp_instances)/((double)roi_cycles()*_dfg->num_vec_output()) // gives seg fault when no dfg
    << ", CGRA outputs/cgra busy cycles: "
    <<  ((double)_stat_comp_instances)/((double)_stat_cgra_busy_cycles)  <<
    "\n";
  out << "CGRA Insts / Computation Instance: "
      << ((double)_stat_ss_insts) / ((double)_stat_comp_instances) << "\n";
  out << "CGRA Insts / Cycle: "
      << ((double)_stat_ss_insts) / ((double)roi_cycles())
      << " (overall activity factor)\n";
  out << "Mapped DFG utilization: "
      << ((double)_stat_ss_dfg_util) / ((double)roi_cycles()) << "\n";
  // FIXME: see it's use
  out << "Data availability ratio: "
      << ((double)_stat_ss_data_avail_ratio) / ((double)roi_cycles()) << "\n";
  // out << "Atomic scr executed cycles: "
  //     << _stat_cycles_atomic_scr_executed << "\n";
  // out << "Atomic scr issued cycles: "
  //     << _num_cycles_issued << "\n";
  
  // out << "Memory initiation interval: "
  //   << _stat_mem_initiation_interval << "\n";
   
  out << "input port imbalance (%age dgra nodes could not fire): "
      << ((double)_stat_port_imbalance) / ((double)_stat_cgra_busy_cycles) << "\n";
      // << ((double)_stat_port_imbalance) / ((double)roi_cycles()) << "\n";
   out << "Percentage bank conflicts: "
      << ((double)_stat_cycles_atomic_scr_executed) /
             _stat_cycles_atomic_scr_pushed
      << "\n";
  out << "L1 cache hit rate: "
      << ((double)_stat_hit_bytes_rd) / ((double)(_stat_hit_bytes_rd+_stat_miss_bytes_rd)) << "\n";
  out << "Avg wait cycles on a byte read: "
      << ((double)_stat_tot_mem_wait_cycles) / ((double)_stat_mem_bytes_rd) << "\n";



  // out << "Allowed input port consumption rate: ";
  // for (int i = 0; i < NUM_OUT_PORTS; ++i) {
  //   out << ((double)_slot_avail[i] / (double)roi_cycles()) << ", ";
  // }
  // out << "\n";
  // out << "percentage time we could not serve input ports: ";
  // for (int i = 0; i < NUM_OUT_PORTS; ++i) {
  //   out << ((double)_could_not_serve[i] / (double)roi_cycles()) << ", ";
  // }
  // out << "\n";

  out << "Cycle Breakdown: ";
  _pipe_stats.print_histo(out, roi_cycles());
  out << "\n";

  /*
   * Depricated
  out << "\n";
  out << "Avg. Scratch Read Port Req Size:  "
    << ((double)_stat_scratch_read_bytes)/((double)_stat_scratch_reads) << "\n";
  out << "Avg. Scratch Write Port Req Size: "
    << ((double)_stat_scratch_write_bytes)/((double)_stat_scratch_writes) <<
  "\n";

  out << "Scratch reads per cycle:  " <<
  ((double)_stat_scratch_reads)/((double)roi_cycles()) << "\n"; out << "Scratch
  writes per cycle: " << ((double)_stat_scratch_writes)/((double)roi_cycles())
  << "\n"; out << "\n";


  out << "Avg. Mem Load Port Req Size:  "
    << ((double)_stat_tot_mem_fetched)/((double)_stat_tot_loads) << "\n";
  out << "Avg. Mem Store Port Req Size: "
    << ((double)_stat_tot_mem_stored)/((double)_stat_tot_stores) << "\n";

  out << "Mem loads per cycle:    " <<
  ((double)_stat_tot_loads)/((double)roi_cycles()) << "\n"; out << "Mem stores
  per cycle:   " << ((double)_stat_tot_stores)/((double)roi_cycles()) << "\n";
  out << "\n";
*/

  /*
   // TODO: FIXME: This is useful info -- figure out how to git it
  out << "Mem load misses per cycle:  " <<
  ((double)_stat_tot_mem_load_acc)/((double)roi_cycles()) << "\n"; out << "Mem
  store misses per cycle: " <<
  ((double)_stat_tot_mem_store_acc)/((double)roi_cycles()) << "\n"; out << "\n";
*/

  // const std::vector<LOC> alocs {LOC::DMA, LOC::SCR};
  static const std::vector<LOC> locs{LOC::NONE, LOC::DMA, LOC::SCR, LOC::PORT,
                                     LOC::CONST};

  auto print_bwm = [&](LOC l1, LOC l2) {
    auto &p = _bw_map[make_pair(l1, l2)];
    if (p.second == 0)
      out << "(0 B/c, 0 B/r) ";
    else {
      out.precision(3);
      out << ((double)p.first / (double)roi_cycles());
      out << "(" << ((double)p.second / (double)roi_cycles()) << "B/c";
      out << ", " << ((double)p.second / (double)p.first) << "B/r) ";
    }
  };
  auto print_src = [&](string name, LOC l1) {
    out << name;
    print_bwm(l1, LOC::TOTAL);
    out << " -- ";
    for (auto l2 : locs) {
      if (l2 == LOC::TOTAL)
        continue;
      auto &p = _bw_map[make_pair(l1, l2)];
      if (p.first > 0) {
        out << base_stream_t::loc_name(l2) << ":";
        print_bwm(l1, l2);
      }
    }
    out << "\n";
  };

  auto print_dest = [&](string name, LOC l2) {
    out << name;
    print_bwm(LOC::TOTAL, l2);
    out << " -- ";
    for (auto l1 : locs) {
      if (l1 == LOC::TOTAL)
        continue;
      auto &p = _bw_map[make_pair(l1, l2)];
      if (p.first > 0) {
        out << base_stream_t::loc_name(l1) << ":";
        print_bwm(l1, l2);
      }
    }
    out << "\n";
  };

  out << "Bandwidth Table: (B/c=Bytes/cycle, B/r=Bytes/request) -- Breakdown "
         "(sources/destinatinos): \n";
  print_src("SP_READ:\t", LOC::SCR);
  print_dest("SP_WRITE:\t", LOC::SCR);
  print_src("DMA_LOAD:\t", LOC::DMA);
  print_dest("DMA_STORE:\t", LOC::DMA);
  print_dest("REC_BUS_READ:\t", LOC::REC_BUS);
  print_dest("NETWORK:\t", LOC::NETWORK);
}

// wait and print stats
void accel_t::print_stats() {
  print_statistics(std::cout);
  print_status();

  ofstream stat_file;
  if (char *name = getenv("SS_RUN_NAME")) {
    stat_file.open(string("stats/") + name + ".ss-stats",
                   ofstream::trunc | ofstream::out);
  } else {
    stat_file.open("stats/default.ss-stats", ofstream::trunc | ofstream::out);
  }

  assert(stat_file.is_open());
  print_statistics(stat_file);
  pedantic_statistics(stat_file);
}

// pushed implicit stream in cmd queue -- I guess not being issued
void accel_t::push_net_in_cmd_queue(base_stream_t *s) {
  _ssim->add_bitmask_stream(s);
}

// --------------------------SCHEDULE STREAMS ONTO
// CONTROLLERS----------------------- This is essentially the stream dispatcher
void accel_t::schedule_streams() {

  // FIXME: check with Tony!
  if(_cleanup_mode) return;

  int str_width = _ssconfig->dispatch_width();
  int str_issued = 0;

  bool blocked_ivp[64] = {0}; // These are set by stream registers
  bool blocked_ovp[64] = {0};
  bool prior_scratch_read = false;
  bool prior_scratch_write = false;
  bool bar_scratch_read = false; // These are set by scratch
  bool bar_scratch_write = false;
  bool bar_remote_scratch_write = false;

  // schedule for ports (these need to go in program order per-vp)
  for (auto i = _cmd_queue.begin();
       i != _cmd_queue.end() && str_issued < str_width;) {
    base_stream_t *ip = i->get();
    port_data_t *out_vp = NULL;
    port_data_t *out_vp2 = NULL; // for atomic stream

    bool scheduled = false;
    int repeat = ip->repeat_in();
    int repeat_str = ip->repeat_str();
    bool repeat_flag = ip->repeat_flag();
    LOC unit = ip->unit();

    bool ivps_can_take = true;
    // FIXME: check is this needs to be changed with repeat_flag!
    for (int in_port : ip->in_ports()) {
      port_data_t *in_vp = &_port_interf.in_port(in_port);
      ivps_can_take = ivps_can_take && (!blocked_ivp[in_port]) &&
                      in_vp->can_take(unit, repeat, repeat_str);
    }

    bool blocked_by_barrier = false;
    // FIXME: Check if this is correct!
    if ((ip->src() & LOC::SCR) != LOC::NONE) {
      // scratch reads cannot proceed before write barriers
      blocked_by_barrier |= bar_scratch_write;
      blocked_by_barrier |= bar_remote_scratch_write;
    } else if ((ip->dest() & LOC::SCR) != LOC::NONE) {
      // scratch writes cannot proceed before read barriers or write barriers
      blocked_by_barrier |= bar_scratch_read;
      blocked_by_barrier |= bar_scratch_write;
      blocked_by_barrier |= bar_remote_scratch_write;
    }

    if (blocked_by_barrier) { // This handles all barrier insts
      for (int in_port : ip->in_ports()) {
        blocked_ivp[in_port] = true;
      }
      if (ip->out_port() != -1) {
        blocked_ovp[ip->out_port()] = true;
      }
      //    } else if(auto stream = dynamic_cast<scr_scr_stream_t*>(ip)) {
      //      if(stream->_is_source) { //do not schedule, but check out port
      //      available
      //        if(stream->_is_ready) {  //writer is ready
      //          scheduled = _scr_r_c.schedule_scr_scr(*stream);
      //        }
      //      } else { //destination waits for data
      //        scheduled = _scr_w_c.schedule_scr_scr(*stream);
      //        if(scheduled) {
      //          stream->_remote_stream->_is_ready=true; //writer says its okay
      //        }
      //      }
    } else if (auto stream = dynamic_cast<stream_barrier_t *>(ip)) {
      bool blocked = (stream->bar_scr_rd() && prior_scratch_read) ||
                     (stream->bar_scr_wr() && prior_scratch_write) ||
                     (stream->bar_scr_wr_df() && prior_scratch_write);
      blocked |= !done_concurrent(false, stream->_mask);
      if (!blocked) {
        scheduled = true;
        if (SS_DEBUG::SCR_BARRIER) {
          timestamp();
          std::cout << "BARRIER ISSUED, Scratch Write Complete\n";
        }

      } else { // blocked, so prevent younger streams
        bar_scratch_read |= stream->bar_scr_rd();
        bar_scratch_write |= stream->bar_scr_wr();
        bar_remote_scratch_write |= stream->bar_scr_wr_df();
      }
    } else if (auto stream = dynamic_cast<remote_core_net_stream_t *>(ip)) {
      // should I check if those ports not busy (Although they should not be)
      out_vp = &_port_interf.out_port(NET_ADDR_PORT);
      out_vp2 = &_port_interf.out_port(NET_VAL_PORT);
      assert(out_vp->can_take(LOC::PORT) && !blocked_ovp[NET_ADDR_PORT]);
      assert(out_vp2->can_take(LOC::PORT) && !blocked_ovp[NET_VAL_PORT]);
      scheduled = _scr_w_c.schedule_network_stream(*stream);
      out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
      out_vp2->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
    } else if (auto stream = dynamic_cast<affine_read_stream_t *>(ip)) {
      if (ivps_can_take) {
        if (unit == LOC::DMA) {
          scheduled = _dma_c.schedule_dma_port(*stream);
        } else {
          scheduled = _scr_r_c.schedule_scr_port(*stream);
        }
      }
    } else if (auto stream = dynamic_cast<remote_port_stream_t *>(ip)) {
      if (stream->_is_source) { // do not schedule, but check out port available
        int out_port = stream->_out_port;
        out_vp = &_port_interf.out_port(out_port);

        if ((scheduled =
                 (out_vp->can_take(LOC::PORT) && !blocked_ovp[out_port]))) {
          out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
          stream->_remote_stream->_is_ready = true; // we'll check this in dest.
        }
      } else { // destination, time for action!
        if (ivps_can_take && stream->_is_ready /*src is ready*/) {
          scheduled = _port_c.schedule_remote_port(*stream);
        }
      }
    } else if (auto rem_scr_stream =
                   dynamic_cast<direct_remote_scr_stream_t *>(ip)) {
      int val_port = rem_scr_stream->_out_port;

      out_vp = &_port_interf.out_port(val_port); // this is in data out port

      if (out_vp->can_take(LOC::PORT) && !blocked_ovp[val_port] &&
          (scheduled = _net_c.schedule_direct_remote_scr(*rem_scr_stream))) {
        out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
      }
    } else if (auto ind_rem_scr_stream =
                   dynamic_cast<remote_scr_stream_t *>(ip)) {
      int addr_port = ind_rem_scr_stream->_addr_port;
      int val_port = ind_rem_scr_stream->_out_port;

      out_vp = &_port_interf.out_port(addr_port); // this is addr out port
      out_vp2 = &_port_interf.out_port(val_port); // this is in data out port

      if (out_vp->can_take(LOC::PORT) && !blocked_ovp[addr_port] &&
          out_vp2->can_take(LOC::PORT) && !blocked_ovp[val_port] &&
          (scheduled = _net_c.schedule_remote_scr(*ind_rem_scr_stream))) {
        out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
        out_vp2->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
      }
    } else if (auto rem_port_multicast_stream =
                   dynamic_cast<remote_port_multicast_stream_t *>(ip)) {
      // make sure the base stream is lower in the comparison
      int out_port = rem_port_multicast_stream->_out_port;
      out_vp = &_port_interf.out_port(out_port); // this is data output port

      if (out_vp->can_take(LOC::PORT) && !blocked_ovp[out_port] &&
          (scheduled = _net_c.schedule_remote_port_multicast(
               *rem_port_multicast_stream))) {
        // printf("Remote port stream is scheduled\n");
        out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
      }
    } else if (auto port_port_stream = dynamic_cast<port_port_stream_t *>(ip)) {
      int out_port = port_port_stream->_out_port;
      out_vp = &_port_interf.out_port(out_port);
      // cout << "Out port for this recurrence stream: " << out_port << endl;
      auto rpt_vp = &_port_interf.out_port(1); // not sure, check with Tony
      if(port_port_stream->repeat_flag()) {
        // cout << "Repeat port for this recurrence stream: " << port_port_stream->repeat_in() << endl;
        int rpt_port = port_port_stream->repeat_in();
        rpt_vp = &_port_interf.out_port(rpt_port);
      } else {
        // cout << "Repeat flag off" << endl;
      }
      // cout << "Output of my condition1: " << (!port_port_stream->repeat_flag()) << endl;
      bool cond1 = (!port_port_stream->repeat_flag());
      bool cond2 = (port_port_stream->repeat_flag() && rpt_vp->can_take(LOC::PORT));
      bool res_cond = cond1 || cond2;
      if (ivps_can_take && out_vp->can_take(LOC::PORT) &&
          res_cond && 
          !blocked_ovp[out_port] &&
          (scheduled = _port_c.schedule_port_port(*port_port_stream))) {
        scheduled = true;
        out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
        if(port_port_stream->repeat_flag()) {
          rpt_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
        }
      }

    } else if (auto const_port_stream =
                   dynamic_cast<const_port_stream_t *>(ip)) {
      if (ivps_can_take) {
        scheduled = _port_c.schedule_const_port(*const_port_stream);
      }
    } else if (auto stream = dynamic_cast<affine_write_stream_t *>(ip)) {
      int port = stream->_out_port;
      out_vp = &_port_interf.out_port(port);
      if (out_vp->can_take(unit) && !blocked_ovp[port]) {
        if (unit == LOC::DMA) {
          scheduled = _dma_c.schedule_port_dma(*stream);
        } else {
          scheduled = _scr_w_c.schedule_port_scr(*stream);
        }
        if (scheduled)
          out_vp->set_status(port_data_t::STATUS::BUSY, unit);
      }
    } else if (auto ind_stream = dynamic_cast<indirect_stream_t *>(ip)) {
      int ind_port = ind_stream->_ind_port;            // grab output res
      auto *ind_vp = &_port_interf.out_port(ind_port); // indirect output port
      out_vp = ind_vp;

      if (ivps_can_take && ind_vp->can_take(LOC::PORT) &&
          !blocked_ovp[ind_port]) {

        // ports okay, schedule to scratch or dma!
        bool succ = false;
        if (ind_stream->scratch())
          succ = _scr_r_c.schedule_indirect(*ind_stream);
        else
          succ = _dma_c.schedule_indirect(*ind_stream);

        if (succ) {
          scheduled = true;
          ind_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
        }
      }
    } else if (auto ind_stream = dynamic_cast<indirect_wr_stream_t *>(ip)) {
      int ind_port = ind_stream->_ind_port; // grab output res
      int out_port = ind_stream->_out_port;
      auto *ind_vp = &_port_interf.out_port(ind_port); // indirect output port
      out_vp = &_port_interf.out_port(out_port);
      out_vp2 = ind_vp;

      if (out_vp->can_take(LOC::PORT) && ind_vp->can_take(LOC::PORT) &&
          !blocked_ovp[out_port] && !blocked_ovp[ind_port]) {
        bool succ = false;
        if (ind_stream->scratch())
          succ = _scr_w_c.schedule_indirect_wr(*ind_stream);
        else
          succ = _dma_c.schedule_indirect_wr(*ind_stream);
        if (succ) {
          scheduled = true;
          out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
          ind_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
        }
      }
    } else if (auto atomic_scr_stream =
                   dynamic_cast<atomic_scr_stream_t *>(ip)) {
      int addr_port = atomic_scr_stream->_out_port;
      int val_port = atomic_scr_stream->_val_port;

      out_vp = &_port_interf.out_port(addr_port); // this is addr output port
      out_vp2 =
          &_port_interf.out_port(val_port); // this is increment value port

      if (out_vp->can_take(LOC::PORT) && out_vp2->can_take(LOC::PORT) &&
          !blocked_ovp[addr_port] && !blocked_ovp[val_port] &&
          (scheduled = _scr_w_c.schedule_atomic_scr_op(*atomic_scr_stream))) {
        out_vp->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
        out_vp2->set_status(port_data_t::STATUS::BUSY, LOC::PORT);
      }
    } else if (auto const_scr_stream = dynamic_cast<const_scr_stream_t *>(ip)) {
      scheduled = _scr_w_c.schedule_const_scr(*const_scr_stream);
    }

    // prevent out-of-order access
    for (int in_port : ip->in_ports())
      blocked_ivp[in_port] = true;
    if (out_vp) {
      blocked_ovp[out_vp->port()] = true;
    }
    if (out_vp2)
      blocked_ovp[out_vp2->port()] = true;

    prior_scratch_read |= ((ip->src() & LOC::SCR) != LOC::NONE);
    prior_scratch_write |= ((ip->dest() & LOC::SCR) != LOC::NONE);

    if (scheduled) {

      for (int in_port : ip->in_ports()) {
        port_data_t *in_vp = &_port_interf.in_port(in_port);
        in_vp->set_status(port_data_t::STATUS::BUSY, ip->unit());
        // it means different for recurrence stream: confirm if this makes sense
        if (auto port_port_stream = dynamic_cast<port_port_stream_t *>(ip)) {
          (void) (port_port_stream);
          if(!ip->repeat_flag()) in_vp->set_repeat(repeat, repeat_str, repeat_flag);
        } else {
          in_vp->set_repeat(repeat, repeat_str, repeat_flag);
        }
      }

      str_issued++;
      if (SS_DEBUG::COMMAND_I) {
        timestamp();
        cout << " ISSUED \tid:" << ip->id() << " ";
        ip->print_status();
      }

      // delete ip; this is a std::shared_ptr now
      i = _cmd_queue.erase(i);
      if (_ssim->in_roi()) {
        _stat_commands_issued++;
      }
    } else {
      // we failed to schedule anything!
      if (_ssconfig->dispatch_inorder()) { // if INORDER-dispatch, stop!
        break;
      } else { // if we're in OOO-dispatch mode, just keep going...
        ++i;
      }
    }
  }
}

void data_controller_t::add_bw(LOC l1, LOC l2, uint64_t times, uint64_t bytes) {
  _accel->add_bw(l1, l2, times, bytes);
}

ssim_t *data_controller_t::get_ssim() { return _accel->get_ssim(); }

bool data_controller_t::is_shared() { return _accel->is_shared(); }

void data_controller_t::timestamp() { _accel->timestamp(); }

bool dma_controller_t::schedule_dma_port(affine_read_stream_t &new_s) {
  auto *s = new affine_read_stream_t(new_s);
  // UPDATE CONTEXT ADDRESS FOR SIMT-STYLE DMA INTERACTIONS
  auto address = s->cur_addr() + _accel->accel_index() * new_s.ctx_offset();
  std::fill(s->address.begin(), s->address.end(), address);

  _dma_port_streams.push_back(s);
  _read_streams.push_back(s);
  return true;
}

bool dma_controller_t::schedule_indirect(indirect_stream_t &new_s) {
  auto *s = new indirect_stream_t(new_s);
  _indirect_streams.push_back(s);
  _read_streams.push_back(s);
  return true;
}

bool dma_controller_t::schedule_port_dma(affine_write_stream_t &new_s) {
  auto *s = new affine_write_stream_t(new_s);
  auto address = s->cur_addr() + _accel->accel_index() * new_s.ctx_offset();
  std::fill(s->address.begin(), s->address.end(), address);

  _port_dma_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}

bool dma_controller_t::schedule_indirect_wr(indirect_wr_stream_t &new_s) {
  auto *s = new indirect_wr_stream_t(new_s);
  _indirect_wr_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}

// bool scratch_read_controller_t::schedule_scr_scr(scr_scr_stream_t& new_s) {
//  for(auto& s : _scr_scr_streams) {
//    if(s.empty()) {
//      s=new_s; //copy the values
//      //cout << s._remote_stream << " became " << &s << "\n";
//
//      s._remote_stream->_remote_stream = &s;//update pointer to remote
//      //cout << "hooked up writer to stable reader\n";
//      return true;
//    }
//  }
//  return false;
//}

bool scratch_read_controller_t::schedule_scr_port(affine_read_stream_t &new_s) {
  auto *s = new affine_read_stream_t(new_s);
  _scr_port_streams.push_back(s);
  _read_streams.push_back(s);
  return true;
}

bool scratch_read_controller_t::schedule_indirect(indirect_stream_t &new_s) {
  auto *s = new indirect_stream_t(new_s);
  _ind_port_streams.push_back(s);
  _read_streams.push_back(s);
  return true;
}

// For port->remote port multicast affine stream
bool network_controller_t::schedule_remote_port_multicast(
    remote_port_multicast_stream_t &new_s) {
  auto *s = new remote_port_multicast_stream_t(new_s);
  _remote_port_multicast_streams.push_back(s);
  _remote_streams.push_back(s);
  return true;
}

// For port->remote scr indirect stream
bool network_controller_t::schedule_remote_scr(remote_scr_stream_t &new_s) {
  auto *s = new remote_scr_stream_t(new_s);
  _remote_scr_streams.push_back(s);
  _remote_streams.push_back(s);
  return true;
}

// For port->remote scr direct stream
bool network_controller_t::schedule_direct_remote_scr(
    direct_remote_scr_stream_t &new_s) {
  auto *s = new direct_remote_scr_stream_t(new_s);
  _direct_remote_scr_streams.push_back(s);
  _remote_streams.push_back(s);
  return true;
}

bool scratch_write_controller_t::schedule_network_stream(
    remote_core_net_stream_t &new_s) {
  auto *s = new remote_core_net_stream_t(new_s);
  _network_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}

bool scratch_write_controller_t::schedule_indirect_wr(
    indirect_wr_stream_t &new_s) {
  auto *s = new indirect_wr_stream_t(new_s);
  _ind_wr_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}

// Atomic stream update: only 1 stream is allowed to be doing this at a time!
bool scratch_write_controller_t::schedule_atomic_scr_op(
    atomic_scr_stream_t &new_s) {
  auto *s = new atomic_scr_stream_t(new_s);
  _atomic_scr_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}

bool scratch_write_controller_t::schedule_const_scr(const_scr_stream_t &new_s) {
  auto *s = new const_scr_stream_t(new_s);
  _const_scr_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}

// bool scratch_write_controller_t::schedule_scr_scr(scr_scr_stream_t& new_s) {
//  for(auto& s : _scr_scr_streams) {
//    if(s.empty()) {
//      s=new_s; //copy the values
//      //cout << s._remote_stream << " became " << &s << "\n";
//      s._remote_stream->_remote_stream = &s;//update pointer to remote
//      //cout << "hooked up reader to stable writer\n";
//      return true;
//    }
//  }
//  return false;
//}

bool scratch_write_controller_t::schedule_port_scr(
    affine_write_stream_t &new_s) {
  auto *s = new affine_write_stream_t(new_s);
  _port_scr_streams.push_back(s);
  _write_streams.push_back(s);
  return true;
}
bool port_controller_t::schedule_port_port(port_port_stream_t &new_s) {
  for (auto &s : _port_port_streams) {
    if (s.empty()) {
      s = new_s;
      return true;
    } else {
      // cout << "Could not schedule because earlier port-port stream available\n";
      // s.print_status(); // delete later
    }
  }
  return false;
  
}
bool port_controller_t::schedule_remote_port(remote_port_stream_t &new_s) {
  for (auto &s : _remote_port_streams) {
    if (s.empty()) {
      s = new_s;
      return true;
    }
  }
  return false;
}

bool port_controller_t::schedule_const_port(const_port_stream_t &new_s) {
  for (auto &s : _const_port_streams) {
    if (s.empty()) {
      s = new_s;
      return true;
    }
  }
  return false;
}

void apply_mask(uint8_t *raw_data, vector<bool> mask,
                std::vector<uint8_t> &data) {
  assert(mask.size()==64);
  for (int i = 0; i < mask.size(); ++i) {
    if (mask[i]) {
      data.push_back(raw_data[i]);
    }
  }
}

void apply_map(uint8_t *raw_data, const vector<int> &imap,
               std::vector<uint8_t> &data) {
  assert(imap.size() != 0);
  data.resize(imap.size());
  for (int i = 0; i < imap.size(); ++i) {
    // cout << "at imap[" << imap[i] << "], data=" << raw_data[imap[i]] << "\n";
    data[i] = raw_data[imap[i]];
  }
}

void dma_controller_t::port_resp(unsigned cur_port) {

  if (Minor::LSQ::LSQRequestPtr response =
          _accel->_lsq->findResponse(cur_port)) {
  
    // First check if we haven't discared the memory request
    // this will only be the case if reqs are equal to zero
    if (_accel->_cleanup_mode) {
      _accel->_lsq->popResponse(cur_port);
      _mem_read_reqs--;
      return;
    }

    if (_accel->_accel_index == response->sdInfo->which_accel) {

      auto &pi = _accel->port_interf();

      PacketPtr packet = response->packet;
      if (packet->getSize() != MEM_WIDTH) {
        assert(0 && "weird memory response size");
      }

      vector<uint8_t> data;
      if (response->sdInfo->mask.size() > 0) {
        apply_mask(packet->getPtr<uint8_t>(), response->sdInfo->mask, data);
      } else if (response->sdInfo->map.size() > 0) {
        apply_map(packet->getPtr<uint8_t>(), response->sdInfo->map, data);
      }


      //-----------------

      bool port_in_okay = true;
      // push in byte-by-byte at the ports
      for (int in_port : response->sdInfo->ports) {
        port_data_t &in_vp = pi.in_port(in_port);
        port_in_okay = port_in_okay && (in_vp.can_push_bytes_vp(data.size()));
      }

      if (port_in_okay) {
        bool last = response->sdInfo->last;

        if (SS_DEBUG::MEM_REQ) {
          _accel->timestamp();
          std::cout << "response for " << std::hex << packet->getAddr()
                    << std::dec << "for port " << cur_port
                    << ", size in bytes: " << data.size() << " elements"
                    << (last ? "(last)" : "") << "\n";
        }
        
        // FIXME: check if stats are reset at roi
        // _accel->_stat_mem_bytes_rd += data.size();

        for (int in_port : response->sdInfo->ports) {
          port_data_t &in_vp = pi.in_port(in_port);
          in_vp.push_data(data);

          if (response->sdInfo->stride_hit) {
            if (response->sdInfo->fill_mode == STRIDE_ZERO_FILL ||
                response->sdInfo->fill_mode == STRIDE_DISCARD_FILL) {
              in_vp.fill(response->sdInfo->fill_mode);
            }
          }
          if (last) {
            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: DMA->PORT2 (port:" << cur_port << ")\n";
            }

            in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                             response->sdInfo->fill_mode);
          }
        }

        // cache hit stats collection
        /*if(_accel->_ssim->in_roi()) {
          _accel->_stat_tot_mem_wait_cycles += (_accel->get_cur_cycle()-response->sdInfo->request_cycle);
          _accel->_stat_mem_bytes_rd += data.size();
          if(_accel->get_cur_cycle()-response->sdInfo->request_cycle<20) { // probably L1 hit
            _accel->_stat_hit_bytes_rd += data.size();
          } else {
            _accel->_stat_miss_bytes_rd += data.size();
          } 
        }*/

        if(_accel->_ssim->in_roi()) {
          _accel->_stat_mem_bytes_rd += data.size();
        }
        data.clear();
        _accel->_lsq->popResponse(cur_port);

        _mem_read_reqs--;
        return;
      }
    }
  }
}
/*
    if(get_ssim()->scratch_wr_free(response->_context_bitmask)) {

      PacketPtr packet = response->packet;
      if(packet->getSize()!=MEM_WIDTH) {
        assert(0 && "weird memory response size");
      }
      vector<SBDT> data;
      apply_mask(packet->getPtr<uint64_t>(), response->sdInfo->mask, data);

      for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
        if(_context_bitmask & b) {

        if(_scr_w_c->_buf_dma_write.push_data(response->sdInfo->scr_addr, data))
   { if(SS_DEBUG::MEM_REQ) { _accel->timestamp(); std::cout << "data into
   scratch " << response->sdInfo->scr_addr
                      << ":" << data.size() << "elements, ctx=" "\n";
          }
          if(_accel->_ssim->in_roi()) {
            _accel->_stat_mem_bytes_rd+=data.size();
          }

      }
      //handled_req=true;
      _fake_scratch_reqs--;
      _mem_read_reqs--;

      _accel->_lsq->popResponse(SCR_STREAM);
    }
*/

// ---------------------STREAM CONTROLLER TIMING
// ------------------------------------ If response, can issue load Limitations:
// 1 Response per cycle (512 bits/cycle)
void dma_controller_t::cycle() {
  // Memory read to config
  if (Minor::LSQ::LSQRequestPtr response =
          _accel->_lsq->findResponse(CONFIG_STREAM)) {
    PacketPtr packet = response->packet;
    uint64_t context = response->sdInfo->which_accel;
    for (uint64_t i = 0, b = 1; i < NUM_ACCEL_TOTAL; ++i, b <<= 1) {
      if (context & b) {
        _accel->_ssim->accel_arr[i]->configure(packet->getAddr(),
                                               packet->getSize() / 8,
                                               packet->getPtr<uint64_t>());
      }
    }
    _accel->_lsq->popResponse(CONFIG_STREAM);
  }

  // Memory Read to Ports
  for (unsigned i = 0; i < _accel->_soft_config.in_ports_active.size(); ++i) {
    int cur_port = _accel->_soft_config.in_ports_active[i];
    port_resp(cur_port);
  }
  for (unsigned i = START_IND_PORTS; i < STOP_IND_PORTS; ++i) { // ind read
                                                                // ports
    port_resp(i);
  }

  make_read_request();  // read request
  make_write_request(); // write request
}

void dma_controller_t::print_status() {
  for (auto &i : _read_streams) {
    if (!i->empty()) {
      i->print_status();
    }
  }
  for (auto &i : _write_streams) {
    if (!i->empty()) {
      i->print_status();
    }
  }
}

void dma_controller_t::finish_cycle() {}

// Delete Streams implementation
template <typename T>
void delete_stream_internal(int i, T *s, std::vector<T *> &vec,
                            std::vector<base_stream_t *> &base_vec) {
 
  base_vec.erase(base_vec.begin() + i);
  vec.erase(std::remove(vec.begin(), vec.end(), s), vec.end());
}

void dma_controller_t::delete_stream(int i, affine_read_stream_t *s) {
  delete_stream_internal(i, s, _dma_port_streams, _read_streams);
}
void dma_controller_t::delete_stream(int i, indirect_stream_t *s) {
  delete_stream_internal(i, s, _indirect_streams, _read_streams);
}
void dma_controller_t::delete_stream(int i, affine_write_stream_t *s) {
  delete_stream_internal(i, s, _port_dma_streams, _write_streams);
}
void dma_controller_t::delete_stream(int i, indirect_wr_stream_t *s) {
  delete_stream_internal(i, s, _indirect_wr_streams, _write_streams);
}

void scratch_read_controller_t::delete_stream(int i, affine_read_stream_t *s) {
  delete_stream_internal(i, s, _scr_port_streams, _read_streams);
}
void scratch_read_controller_t::delete_stream(int i, indirect_stream_t *s) {
  delete_stream_internal(i, s, _ind_port_streams, _read_streams);
}

void network_controller_t::delete_stream(int i,
                                         remote_port_multicast_stream_t *s) {
  delete_stream_internal(i, s, _remote_port_multicast_streams, _remote_streams);
}
void network_controller_t::delete_stream(int i, remote_scr_stream_t *s) {
  delete_stream_internal(i, s, _remote_scr_streams, _remote_streams);
}
void network_controller_t::delete_stream(int i, direct_remote_scr_stream_t *s) {
  delete_stream_internal(i, s, _direct_remote_scr_streams, _remote_streams);
}

void scratch_write_controller_t::delete_stream(int i,
                                               affine_write_stream_t *s) {
  delete_stream_internal(i, s, _port_scr_streams, _write_streams);
}
void scratch_write_controller_t::delete_stream(int i, const_scr_stream_t *s) {
  delete_stream_internal(i, s, _const_scr_streams, _write_streams);
}
void scratch_write_controller_t::delete_stream(int i, atomic_scr_stream_t *s) {
  delete_stream_internal(i, s, _atomic_scr_streams, _write_streams);
}
void scratch_write_controller_t::delete_stream(int i, indirect_wr_stream_t *s) {
  delete_stream_internal(i, s, _ind_wr_streams, _write_streams);
}

void dma_controller_t::make_write_request() {
  for (unsigned i = 0; i < _write_streams.size(); ++i) {
    _which_wr = (_which_wr + 1) >= _write_streams.size() ? 0 : _which_wr + 1;
    base_stream_t *s = _write_streams[_which_wr];

    if (auto *sp = dynamic_cast<affine_write_stream_t *>(s)) {
      auto &stream = *sp;
      if (stream.stream_active()) {
        port_data_t &out_port =
            _accel->port_interf().out_port(stream._out_port);
        if ((out_port.mem_size() > 0) && // TODO:  unoptimal if we didn't wait?
            (stream.garbage() ||
             (_accel->_lsq->canRequest() &&
              _accel->_lsq->sd_transfers[MEM_WR_STREAM].canReserve()))) {

          if (!stream.garbage()) {
            _accel->_lsq->sd_transfers[MEM_WR_STREAM].reserve();
            req_write(stream, out_port);
          } else { // it's garbage
            while (stream.stream_active() && out_port.mem_size() > 0) {
              out_port.pop_out_data(); // get rid of data
              // timestamp(); cout << "POPPED b/c port->dma " << out_port.port()
              // << " " << out_port.mem_size() << "\n";
              stream.pop_addr(out_port.get_port_width()); // get rid of addr
            }
          }

          bool is_empty = stream.check_set_empty();
          if (is_empty) {
            _accel->process_stream_stats(stream);
            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: PORT->DMA\n";
            }
            out_port.set_status(port_data_t::STATUS::FREE);
            delete_stream(_which_wr, sp);
          }
          return;
        }
      }
    } else if (auto *sp = dynamic_cast<indirect_wr_stream_t *>(s)) {
      auto &stream = *sp;
      if (stream.stream_active() && _accel->_lsq->canRequest() &&
          _accel->_lsq->sd_transfers[MEM_WR_STREAM].canReserve()) {

        port_data_t &out_port =
            _accel->port_interf().out_port(stream._out_port);
        port_data_t &ind_port =
            _accel->port_interf().out_port(stream._ind_port);

        if (out_port.mem_size() > 0 && ind_port.mem_size() >= stream._index_bytes) {
          _accel->_lsq->sd_transfers[MEM_WR_STREAM].reserve();
          ind_write_req(stream);

          if (stream.empty()) {
            delete_stream(_which_wr, sp);
          }

          return;
        }
      }
    } else {
      assert(0 && "invalid type");
    }
  }
}

#define MAX_PORT_READY 100000

void dma_controller_t::make_read_request() {
  //--------------------------
  float min_port_ready = -2;
  //----------------------------

  for (unsigned i = 0; i < _read_streams.size(); ++i) {
    _which_rd = (_which_rd + 1) >= _read_streams.size() ? 0 : _which_rd + 1;
    base_stream_t *s = _read_streams[_which_rd];

    if (auto *sp = dynamic_cast<affine_read_stream_t *>(s)) {
      auto &stream = *sp;
      if (stream.stream_active()) {

        //-----------------------------
        bool skip_check = SS_DEBUG::UNREAL_INPUTS;

        if (min_port_ready == -2) {
          min_port_ready = calc_min_port_ready();
        }
        if (!skip_check && min_port_ready >= MAX_PORT_READY)
          continue;
        //--------------------------------


        int in_port = stream.first_in_port();
        auto &in_vp = _accel->port_interf().in_port(in_port);

        if (_accel->_lsq->sd_transfers[in_port].unreservedRemainingSpace() >
                0 &&
            _accel->_lsq->canRequest()) {

          _accel->_lsq->sd_transfers[in_port].reserve();

          req_read(stream);
          if (stream.empty()) {
            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: DMA->PORT \n";
            }
            in_vp.set_status(port_data_t::STATUS::COMPLETE, LOC::DMA);
            delete_stream(_which_rd, sp);
          }
          return;
        }
      }
    } else if (auto *sp = dynamic_cast<indirect_stream_t *>(s)) {
      indirect_stream_t &stream = *sp;

      auto &ind_vp = _accel->port_interf().out_port(stream._ind_port);
      auto &subsize_vp = _accel->port_interf().out_port(stream._sn_port);
      int in_port = stream.first_in_port();

      if (stream.stream_active()) {

        //-----------------------------
        bool skip_check = SS_DEBUG::UNREAL_INPUTS;

        if (min_port_ready == -2) {
          min_port_ready = calc_min_port_ready();
        }
        if (!skip_check && min_port_ready >= MAX_PORT_READY)
          continue;
        //--------------------------------
        // cout << "memory in ind_vp: " << ind_vp.mem_size() << "\n"; 
        // cout << "susize mem: " << subsize_vp.mem_size() << "\n";
        // cout << "lsq sd transfer: " <<  _accel->_lsq->sd_transfers[in_port].unreservedRemainingSpace() << endl;
        // cout << "cal lsq request: " << _accel->_lsq->canRequest() << endl;

        // if (ind_vp.mem_size() >= stream._index_bytes && 
        //   (!stream._is_2d_stream || (stream._is_2d_stream && subsize_vp.mem_size() >= stream._index_bytes)) && 
          if (ind_vp.mem_size() >= stream._index_bytes/ind_vp.get_port_width() && 
           (!stream._is_2d_stream || (stream._is_2d_stream && subsize_vp.mem_size() >= stream._index_bytes/subsize_vp.get_port_width())) && 
     
            _accel->_lsq->sd_transfers[in_port].unreservedRemainingSpace() >
                0 && // 8 && // 16 && // 8 && // 1 && // 0 && -- leave for done (TODO: check)
            _accel->_lsq->canRequest()) {

          auto &in_vp = _accel->port_interf().in_port(in_port);
          if (in_vp.num_can_push() > 8) { // FIXME:CHECKME: make sure vp isn't full
            // _accel->_lsq->sd_transfers[in_port].reserve();

            // pull_data_indirect(ind_s,data,mem_complete_cyc);
            ind_read_req(stream);

            if (stream.empty()) {
              // destination ivp
              if (SS_DEBUG::VP_SCORE2) {
                cout << "SOURCE: Indirect DMA->PORT \n";
              }
              in_vp.set_status(port_data_t::STATUS::COMPLETE, LOC::DMA);

              // out_ind_vp is where we keep track of the resource
              if (SS_DEBUG::VP_SCORE2) {
                cout << "SOURCE: Indirect DMA->PORT \n";
              }
              ind_vp.set_status(port_data_t::STATUS::FREE);
              delete_stream(_which_rd, sp);
            }

            return;
          }
        }
      }
    } else {
      assert(0 && "invalid type");
    }
  }
}

// this works only when stride is less than 64 (straddle is for just the next cache line) --
// basically stride should be equal to the data type or does aligned malloc not work?
int dma_controller_t::req_read(affine_read_stream_t &stream) {
  int data_width = stream.data_width();
  addr_t prev_addr = 0;
  addr_t addr = stream.cur_addr();
  addr_t base_addr = addr & MEM_MASK; // this is the request address...

  // base_addr is base of cache line
  addr_t max_addr = base_addr + MEM_WIDTH;

  assert(addr != 0 && "cannot load address 0x0");
  std::fill(mask.begin(), mask.end(), 0);
  for(int i=0; i<stream.straddle_bytes(); ++i) {
    mask[i]=1;
  }
  stream.set_straddle_bytes(0);
  // int words=0;
  int elems = 0;
  while (addr < max_addr && addr > prev_addr && stream.stream_active()) {
    prev_addr = addr;
    if(addr-base_addr+data_width>64) {
      stream.set_straddle_bytes(addr-base_addr+data_width-64);
    }
    for (int i = 0; i < data_width; ++i) {
      mask[addr - base_addr + i] = 1;
    }
    addr = stream.pop_addr();
    elems += 1;
    if (stream.stride_fill() && stream.stride_hit()) {
      break;
    }
  }
  
  // TODO: add l2_miss statistics -- probably somewhere else....
  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, elems * data_width);

    _accel->_stat_tot_mem_fetched += elems * data_width;
    _accel->_stat_tot_loads += 1;
  }

  bool last = stream.check_set_empty(); // do this first so last is set
  if (last) {
    _accel->process_stream_stats(stream);
  }

  SSMemReqInfoPtr sdInfo = NULL;
  sdInfo =
      new SSMemReqInfo(stream.id(), _accel->_accel_index, stream.in_ports(),
                       mask, _accel->get_cur_cycle(), last, stream.fill_mode(), stream.stride_hit());

  // make request
  _accel->_lsq->pushRequest(_accel->cur_minst(), true /*isLoad*/, NULL /*data*/,
                            MEM_WIDTH /*cache line*/, base_addr, 0 /*flags*/,
                            0 /*res*/, sdInfo);

  if (SS_DEBUG::MEM_REQ) {
    _accel->timestamp();
    std::cout << "request for " << std::hex << base_addr << std::dec << " for "
              << elems << " needed elements"
              << "\n";
    std::cout << " -- ";
    stream.print_status();
  }

  _mem_read_reqs++;
  return elems;
}

// It works for both direct/indirect network streams
void scratch_write_controller_t::write_scratch_remote_ind(
    remote_core_net_stream_t &stream) {
  port_data_t &addr_vp = _accel->port_interf().out_port(stream._addr_port);
  port_data_t &val_vp = _accel->port_interf().out_port(stream._val_port);

  int bytes_written = 0;
  SBDT val[8];
  while(addr_vp.mem_size() && val_vp.mem_size() && stream.stream_active() &&
      bytes_written < 64) {
    uint64_t meta_info = addr_vp.pop_out_data(); // no offset here
    // this addr should have both num_bytes and addr info
    addr_t addr = meta_info & 65535; // for 16-bits
    if (SS_DEBUG::NET_REQ) {
      cout << "addr is: " << addr << "\n";
    }
    int num_bytes = meta_info >> 16;
    assert(num_bytes <= 64);
    for (int j = 0; j < num_bytes / 8; ++j) {
      val[j] = val_vp.pop_out_data();
      if (SS_DEBUG::NET_REQ) {
        timestamp();
        cout << "val being written to scratchpad: " << val[j] << "\n";
      }
    }
    _accel->write_scratchpad(addr, &val[0], num_bytes, stream.id());
    bytes_written += num_bytes;
  }
  _remote_scr_writes += bytes_written;

  bool is_empty = false; // stream.check_set_empty();
  if (is_empty) {
    _accel->process_stream_stats(stream);

    if (SS_DEBUG::VP_SCORE2) {
      cout << "SOURCE: INDIRECT PORT -> SCR\n";
    }
    addr_vp.set_status(port_data_t::STATUS::FREE);
    val_vp.set_status(port_data_t::STATUS::FREE);
  }

  if (_accel->_ssim->in_roi()) {
    add_bw(LOC::PORT, LOC::SCR, 1, bytes_written);
    _accel->_stat_scratch_writes++;
  }

  _accel->_stat_scr_bytes_wr += bytes_written;
  _accel->_stat_scratch_write_bytes += bytes_written;
}

void scratch_write_controller_t::write_scratch_ind(
    indirect_wr_stream_t &stream) {
  port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);
  port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);

  int bytes_written = 0;

  while (ind_vp.mem_size() >= stream._index_bytes && out_vp.mem_size() && stream.stream_active() &&
         bytes_written < 64) {
    
    uint64_t ind=0;
    // FIXME: need to fix according to ind mem
    // This work for ind_vp being lower data width than indirect value config
    // (And also 8?)
    /*for(int i=0; i<stream._index_bytes; ++i) {
      uint8_t temp = ind_vp.peek_out_data(i);
      ind = ind | (temp << i*8);
    }*/
    if(stream._index_bytes > ind_vp.get_port_width()) {
      for(int i=0; i<stream._index_bytes/ind_vp.get_port_width(); ++i) {
        SBDT temp = ind_vp.peek_out_data(i);
        ind = ind | (temp << i*8*ind_vp.get_port_width());
      }
    } else {
      SBDT temp_val = ind_vp.peek_out_data();
      for(int i=0; i<stream._index_bytes; ++i) {
        uint8_t temp = (temp_val >> i*8) & 255;
        ind = ind | (temp << i*8);
      }
    }

    addr_t addr = stream.cur_addr(ind);
    // addr_t addr = stream.cur_addr(ind_vp.peek_out_data());

    bool pop_ind_vp = stream.pop_elem();
    if (pop_ind_vp) { // this is for sub-word granularity reads
      for(int i=0; i<stream._index_bytes; ++i) {
        ind_vp.pop_out_data();
      }
      // ind_vp.pop_out_data();
    }

    uint64_t val = stream.cur_value(out_vp.peek_out_data());

    // push the entry into the write bank queues
    _accel->write_scratchpad(addr, &val, stream._data_bytes, stream.id());

    bytes_written += stream._data_bytes;
    stream._ind_bytes_complete += stream._data_bytes;

    if (stream._ind_bytes_complete == 8) {
      out_vp.pop_out_data();
      stream._ind_bytes_complete = 0;
    }
  }

  bool is_empty = stream.check_set_empty();
  if (is_empty) {
    _accel->process_stream_stats(stream);

    if (SS_DEBUG::VP_SCORE2) {
      cout << "SOURCE: INDIRECT PORT -> SCR\n";
    }
    out_vp.set_status(port_data_t::STATUS::FREE);
    ind_vp.set_status(port_data_t::STATUS::FREE);
  }

  // have the same bank queues thing here
  if (_accel->_ssim->in_roi()) {
    add_bw(LOC::PORT, LOC::SCR, 1, bytes_written);
    _accel->_stat_scratch_writes++;
  }

  _accel->_stat_scr_bytes_wr += bytes_written;
  _accel->_stat_scratch_write_bytes += bytes_written;
}

void scratch_read_controller_t::read_linear_scratch_ind(
    indirect_stream_t &stream,
    uint64_t scr_addr /*if scr is dest, also todo*/) {

  port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);
  auto &subsize_vp = _accel->port_interf().out_port(stream._sn_port);
  if(!stream._is_2d_stream) { 
    assert(stream._sn_port==1);
  }

  bool first = true;
  addr_t base_addr = 0;
  addr_t max_addr = 0;

  // vector<int> imap;
  int bytes_read=0;

  while (ind_vp.mem_size() >= stream._index_bytes && stream.stream_active()
     && (!stream._is_2d_stream || (stream._is_2d_stream && subsize_vp.mem_size() >= stream._index_bytes))) {


    if(stream._is_2d_stream && stream._first_ss_access) {
      uint64_t ind2=0;
      /*for(int i=0; i<stream._index_bytes; ++i) {
        uint8_t temp = subsize_vp.peek_out_data(i);
        ind2 = ind2 | (temp << i*8);
      }*/
      if(stream._index_bytes > subsize_vp.get_port_width()) {
        for(int i=0; i<stream._index_bytes/subsize_vp.get_port_width(); ++i) {
          SBDT temp = subsize_vp.peek_out_data(i);
          ind2 = ind2 | (temp << i*8*subsize_vp.get_port_width());
        }
      } else {
        SBDT temp_val = subsize_vp.peek_out_data();
        for(int i=0; i<stream._index_bytes; ++i) {
          uint8_t temp = (temp_val >> i*8) & 255;
          ind2 = ind2 | (temp << i*8);
        }
      }
      
      stream._sstream_size = ind2; // should be done only in the first size
      stream._first_ss_access=false;
    }



    uint64_t ind=0;
    /*for(int i=0; i<stream._index_bytes; ++i) {
      uint8_t temp = ind_vp.peek_out_data(i);
      ind = ind | (temp << i*8); // FIXME: check the endianness
    }*/
    if(stream._index_bytes > ind_vp.get_port_width()) {
      for(int i=0; i<stream._index_bytes/ind_vp.get_port_width(); ++i) {
        SBDT temp = ind_vp.peek_out_data(i);
        ind = ind | (temp << i*8*ind_vp.get_port_width());
      }
    } else {
      SBDT temp_val = ind_vp.peek_out_data();
      for(int i=0; i<stream._index_bytes; ++i) {
        uint8_t temp = (temp_val >> i*8) & 255;
        ind = ind | (temp << i*8);
      }
    }
    addr_t addr = stream.cur_addr(ind);
    // addr_t addr = stream.cur_addr(ind_vp.peek_out_data());

    // cout << "idx:" << idx << "\taddr:" << hex << addr << dec << "\n";
    if (first) {
      first = false;
      base_addr = addr & SCR_MASK;
      max_addr = base_addr + SCR_WIDTH;
    } else { // not first
      if (addr >= max_addr || addr < base_addr) {
        break;
      }
    }

    int data_bytes = stream._data_bytes;
    SBDT val=0;
    _accel->read_scratchpad(&val, addr, data_bytes, stream.id());
    // assert: port's data width should be equal to the data_bytes to access
    for (int in_port : stream.in_ports()) {
      // cout << "PUSHING DATA TO THE INPUT PORTS: " << data_bytes << " bytes\n";
      port_data_t &in_vp = _accel->port_interf().in_port(in_port);
      in_vp.push_data(in_vp.get_byte_vector(val,data_bytes));
    }

    if (SS_DEBUG::SCR_ACC) {
      _accel->timestamp();
      cout << "linear scr indirect read for " << std::hex << base_addr << std::dec
           << " for " << data_bytes << " needed bytes"
           << " and size of stream: " << stream._num_elements
           << "\n";
    }

    bool pop_ind_vp = stream.pop_elem();
    if (pop_ind_vp) {
      stream._first_ss_access=true;
      for(int i=0; i<stream._index_bytes; ++i) {
        ind_vp.pop_out_data();
        if(stream._is_2d_stream) subsize_vp.pop_out_data();
      }
      // ind_vp.pop_out_data();
    }

    bytes_read += data_bytes;
  }

  if (bytes_read!=0 && _accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, bytes_read);
    _accel->_stat_scr_bytes_rd += bytes_read;
    _accel->_stat_scratch_read_bytes += bytes_read;
    _accel->_stat_scratch_reads++;
  }
}

void scratch_read_controller_t::read_scratch_ind(
    indirect_stream_t &stream,
    uint64_t scr_addr /*if scr is dest, also todo*/) {

  port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);

  // STAGE 1: Push the requests to ports
  if (stream.stream_active() && ind_vp.mem_size() >= stream._index_bytes &&
      _ind_ROB.size() < _accel->_ind_rob_size) {
    // don't use this anymore because data goes into the request instead
    // uint8_t* raw_ptr = (uint8_t*)&stream._cur_ind_val;

    int logical_banks = NUM_SCRATCH_BANKS / stream._data_bytes;

    ind_reorder_entry_t *reorder_entry = new ind_reorder_entry_t();
    _ind_ROB.push(reorder_entry);
    reorder_entry->stream = &stream;
    reorder_entry->data_bytes = stream._data_bytes;
    reorder_entry->size = 0; // paranoia
    reorder_entry->last = false;

    // just distribute the elements to the queues
    while (ind_vp.mem_size() >= stream._index_bytes && stream.stream_active() &&
           reorder_entry->size < 64) {

      uint64_t ind=0;
       for(int i=0; i<stream._index_bytes; ++i) {
         uint8_t temp = ind_vp.peek_out_data(i);
         ind = ind | (temp << i*8);
       }
       addr_t addr = stream.cur_addr(ind);
      // addr_t addr = stream.cur_addr(ind_vp.peek_out_data());

      bool pop_ind_vp = stream.pop_elem();
      if (pop_ind_vp) { // this is for sub-word granularity reads
        for(int i=0; i<stream._index_bytes; ++i) {
          ind_vp.pop_out_data();
        }
        // ind_vp.pop_out_data();
      }

      // push the entry into the read bank queues
      // _accel->read_scratchpad(raw_ptr+stream._ind_bytes_complete, addr,
      // stream._data_bytes, stream.id());
      indirect_scr_read_req request;
      request.ptr = reorder_entry->data + reorder_entry->size;
      // +stream._ind_bytes_complete;
      request.addr = addr;
      request.bytes = stream._data_bytes;
      request.reorder_entry = reorder_entry;

      reorder_entry->size += stream._data_bytes; // increment size of request

      // Assuming linear mapping by default
      int bank_id = addr & (logical_banks - 1);

      _indirect_scr_read_requests[bank_id].push(request);

      assert(reorder_entry->size > 0 && reorder_entry->size <= 64);
    }

    if (!stream.stream_active()) { // vps -> complete state
      reorder_entry->last = true;
      _accel->process_stream_stats(stream);
      if (SS_DEBUG::VP_SCORE2) {
        cout << "SOURCE: Indirect SCR->PORT (queue)\n";
      }

      // the in_vp isn't really full yet because we have to wait until
      // the conflict-free data arrives
      for (int in_port : stream.in_ports()) {
        port_data_t &in_vp = _accel->port_interf().in_port(in_port);
        in_vp.set_status(port_data_t::STATUS::COMPLETE, LOC::SCR);
      }

      if (SS_DEBUG::VP_SCORE2) {
        cout << "SOURCE: Indirect SCR->PORT queue)\n";
      }
      ind_vp.set_status(port_data_t::STATUS::FREE);
    }
  }

  // Stage 2: Carry out the reads to ROB
  int bytes_read = 0;
  for (int i = 0; i < _indirect_scr_read_requests.size(); ++i) {
    if (!_indirect_scr_read_requests[i].empty()) {

      indirect_scr_read_req request = _indirect_scr_read_requests[i].front();
      ind_reorder_entry_t &reorder_entry = *request.reorder_entry;

      _accel->read_scratchpad(request.ptr, request.addr, request.bytes,
                              reorder_entry.stream->id());
      reorder_entry.completed += reorder_entry.data_bytes;
      assert(reorder_entry.size > 0 && reorder_entry.size <= 64);
      assert(reorder_entry.completed <= reorder_entry.size);
      bytes_read += reorder_entry.data_bytes;
      _indirect_scr_read_requests[i].pop();
    }
  }

  if (bytes_read > 0) {
    _accel->_stat_scr_bytes_rd += bytes_read;
    _accel->_stat_scratch_read_bytes += bytes_read;
    if (_accel->_ssim->in_roi()) {
      add_bw(LOC::SCR, LOC::PORT, 1, bytes_read);
      _accel->_stat_scratch_reads++;
    }
  }

  // STAGE 3
  // See if we can pop an item from the indirect rob
  // int bytes_pushed=0;
  if (_ind_ROB.size() > 0) {
    ind_reorder_entry_t *reorder_entry = _ind_ROB.front();

    // cout << "Reorder entry size is: " << reorder_entry->size << " and the completed size: " << reorder_entry->completed << endl;

    if (reorder_entry->size == reorder_entry->completed) {
      // The entry is ready for transfer!

      auto &stream = *reorder_entry->stream;

      for (int i = 0; i < reorder_entry->size; ++i) {
        uint8_t data_byte = reorder_entry->data[i];
        for (int in_port : stream.in_ports()) {
          port_data_t &in_vp = _accel->port_interf().in_port(in_port);
          in_vp.push_data_byte(data_byte);
        }
      }

      // ivp totally free
      if (reorder_entry->last && stream.check_set_empty()) {
        if (SS_DEBUG::VP_SCORE2) {
          cout << "SOURCE: Indirect SCR->PORT \n";
        }

        for (int in_port : stream.in_ports()) {
          port_data_t &in_vp = _accel->port_interf().in_port(in_port);
          in_vp.set_status(port_data_t::STATUS::FREE, LOC::SCR);
        }
      }
      delete reorder_entry;
      _ind_ROB.pop();
    }
  }
}

// This function does two things:
// 1. Check the reorder buffer if the top entry is ready to complete
// 2. Carry out the actual reads
// TODO:FIXME: This function actually can send data on the bus to the
// ports... so it should really be arbitrated
int scratch_read_controller_t::cycle_read_queue() {
  // First, check if we can commit one request from the reorder buffer
  int bytes_pushed = 0;

  return bytes_pushed;
}

void dma_controller_t::ind_read_req(indirect_stream_t &stream) {
  port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);
  auto &subsize_vp = _accel->port_interf().out_port(stream._sn_port);
  if(!stream._is_2d_stream) { 
    assert(stream._sn_port==1);
  }

  bool first = true;
  addr_t base_addr = stream.cur_base_addr;
  addr_t max_addr = 0;

  vector<int> imap;
  // bool first_ss_access=true; // used for 2d stream
  
  if (stream.straddle_bytes()>0) { 
    first=false; 
    for (int i = 0; i < stream.straddle_bytes(); ++i) {
      imap.push_back(i);
    }
    stream.set_straddle_bytes(0);
  }
  else { stream.cur_base_addr = 0; }

  // while (ind_vp.mem_size() >= stream._index_bytes && stream.stream_active()
     //&& (!stream._is_2d_stream || (stream._is_2d_stream && subsize_vp.mem_size() >= stream._index_bytes))) {
  while (ind_vp.mem_size() >= stream._index_bytes/ind_vp.get_port_width() && stream.stream_active() && (!stream._is_2d_stream || (stream._is_2d_stream && subsize_vp.mem_size() >= stream._index_bytes/subsize_vp.get_port_width()))) {

    if(stream._is_2d_stream && stream._first_ss_access) {
      uint64_t ind2=0;

      // this works when subsize_vp width < stream index bytes
      for(int i=0; i<stream._index_bytes/subsize_vp.get_port_width(); ++i) {
        SBDT ssize = subsize_vp.peek_out_data(i);
        ind2 = ind2 | (ssize << i*8*subsize_vp.get_port_width());
        // ind2 = (ind2 << i*8*subsize_vp.get_port_width()) | ssize;
      }
      
      // If sub-stream size is 0, then pop the corresponding element from ind
      // port, also again set first ss access
      if(ind2==0) {
        for(int i=0; i<stream._index_bytes/ind_vp.get_port_width(); ++i) {
          ind_vp.pop_out_data();
        }
        for(int i=0; i<stream._index_bytes/subsize_vp.get_port_width(); ++i) {
          subsize_vp.pop_out_data();
        }
        continue;
      } else {
        stream._sstream_size = ind2; // should be done only in the first size
        // cout << "SIZE PORT NUMBER: " << stream._sn_port << " config bytes: " << stream._index_bytes << endl;
        // cout << "UPDATED SSTREAM SIZE TO: " << stream._sstream_size << endl;
        stream._first_ss_access=false;
      }
    }

    uint64_t ind=0;

    for(int i=0; i<stream._index_bytes/ind_vp.get_port_width(); ++i) {
      SBDT temp = ind_vp.peek_out_data(i);
      // cout << "8-bit value from ind port: " << temp << endl;
      // ind = (ind << i*8*ind_vp.get_port_width()) | temp;
      // FIXME:IMP: lower one works for the simple outer product
      ind = ind | (temp << i*8*ind_vp.get_port_width());
    }
    addr_t addr = stream.cur_addr(ind);
    // cout << "Current indirect address sent is: " << std::hex << addr << endl;
    // addr_t addr = stream.cur_addr(ind_vp.peek_out_data());

    if (first) {
      first = false;
      base_addr = addr & MEM_MASK;
      max_addr = base_addr + MEM_WIDTH;
    } else { // not first
      if (addr >= max_addr || addr < base_addr) {
        break;
      }
    }

    int index = addr - base_addr;

    int data_bytes = stream._data_bytes;

    // if index+i>64, then what? (send 2 cache line requests)
    // cout << "Max map in the cache line: " << index+data_bytes;
    if(index + data_bytes > 64) { 
      cout << "STRADDLING CACHE LINES\n";
      stream.set_straddle_bytes(index+data_bytes-64);
      stream.cur_base_addr=max_addr; 
    }
 
    // for (int i = 0; i < data_bytes; ++i) {
    for (int i = 0; i < data_bytes && (index+i < 64); ++i) {
      imap.push_back(index + i);
    }

    bool pop_ind_vp = stream.pop_elem();
    if (pop_ind_vp) {
      stream._first_ss_access=true;
      // need to consider data width of indirect port as well
      for(int i=0; i<stream._index_bytes/ind_vp.get_port_width(); ++i) {
        ind_vp.pop_out_data();
      }
      if(stream._is_2d_stream) {
        for(int i=0; i<stream._index_bytes/subsize_vp.get_port_width(); ++i) {
          subsize_vp.pop_out_data();
        }
      }
    }

    // if(stream._is_2d_stream) stream.print_status();
  }
  bool last = stream.check_set_empty();

  if (last) {
    _accel->process_stream_stats(stream);
  }

  if(imap.size()==0) return; // case when data-dependent sub-stream size was 0

  // FIXME: moved this here, hopefully this is okay
  int in_port = stream.first_in_port();
  _accel->_lsq->sd_transfers[in_port].reserve();

  if (SS_DEBUG::MEM_REQ) {
    _accel->timestamp();
    cout << "indirect request for " << std::hex << base_addr << std::dec
         << " for " << imap.size() << " needed bytes"
         << "\n";
  }

  SSMemReqInfoPtr sdInfo = NULL;
  sdInfo = new SSMemReqInfo(stream.id(), _accel->_accel_index,
                            stream.in_ports(), imap, _accel->get_cur_cycle(), last, stream.fill_mode());
                            // stream.in_ports(), imap, _accel->_ssim->now(), last, stream.fill_mode());

  // make request
  _accel->_lsq->pushRequest(stream.minst(), true /*isLoad*/, NULL /*data*/,
                            MEM_WIDTH /*cache line*/, base_addr, 0 /*flags*/,
                            0 /*res*/, sdInfo);

  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, imap.size());
    _accel->_stat_tot_mem_fetched += imap.size();
    _accel->_stat_tot_loads += 1;
  }
  // imap.clear();

  _mem_read_reqs++;
}

void dma_controller_t::ind_write_req(indirect_wr_stream_t &stream) {
 
  port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);
  port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);

  bool first = true;
  addr_t init_addr = 0;
  addr_t prev_addr = 0;

  unsigned bytes_written = 0;

  std::vector<uint8_t> data;
  data.resize(MEM_WIDTH);
  uint8_t *data8 = (uint8_t *)data.data();
  // uint16_t* data16 = (uint16_t*)data8;
  uint64_t *data64 = (uint64_t *)data8;

  // FIXME: The size of the indirect stream should be made configurable!
  // int stream_size = stream._data_bytes;
  int stream_size = 8;

  int index = 0;
  while (out_vp.mem_size() && ind_vp.mem_size() >= stream._index_bytes && stream.stream_active()) {

    uint64_t ind=0;
    for(int i=0; i<stream._index_bytes; ++i) {
      uint8_t temp = ind_vp.peek_out_data(i);
      ind = ind | (temp << i*8);
    }
    addr_t addr = stream.cur_addr(ind);
    // addr_t addr = stream.cur_addr(ind_vp.peek_out_data());
 
    if (first) {
      first = false;
      // base_addr = addr & MEM_MASK;
      init_addr = addr;
    } else { // not first
      if (prev_addr + stream_size !=
          addr) { // addr > max_addr || addr < base_addr) {
        // cout <<"prev:" << prev_addr << " new:" << addr << "\n";
        break;
      }
    }

    SBDT val = out_vp.peek_out_data();

    prev_addr = addr;
    data64[index++] = val;

    out_vp.pop_out_data();
    // timestamp(); cout << "POPPED b/c INDIRECT WRITE: " << out_vp.port() <<
    // "\n";
    bytes_written += sizeof(SBDT);

    bool pop_ind_vp = stream.pop_elem();
    if (pop_ind_vp) {
      for(int i=0; i<stream._index_bytes; ++i) {
        ind_vp.pop_out_data();
      }
      // ind_vp.pop_out_data();
    }
  }

  SSMemReqInfoPtr sdInfo =
      new SSMemReqInfo(stream.id(), _accel->_accel_index, MEM_WR_STREAM);

  // cout << "bytes written: " << bytes_written << "addr: " << std::hex <<
  // init_addr
  //  << std::dec << " first elem: " << *data64 << "\n";
  // make store request
  _accel->_lsq->pushRequest(stream.minst(), false /*isLoad*/, data8,
                            bytes_written, init_addr, 0 /*flags*/, 0 /*res*/,
                            sdInfo);

  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, bytes_written);

    _accel->_stat_mem_bytes_wr += bytes_written;
    _accel->_stat_tot_stores++;
  }

  bool is_empty = stream.check_set_empty();
  if (is_empty) {
    _accel->process_stream_stats(stream);

    if (SS_DEBUG::VP_SCORE2) {
      cout << "SOURCE: INDIRECT PORT -> DMA\n";
    }
    out_vp.set_status(port_data_t::STATUS::FREE);
    ind_vp.set_status(port_data_t::STATUS::FREE);
  }
}

// Creates a write request for a contiguous chunk of data smaller than one cache
// line
void dma_controller_t::req_write(affine_write_stream_t &stream,
                                 port_data_t &ovp) {
  addr_t addr = stream.cur_addr();
  addr_t init_addr = addr;
  addr_t base_addr = addr & MEM_MASK;
  addr_t max_addr = base_addr + MEM_WIDTH;

  assert(addr != 0 && "cannot store to address 0x0");

  unsigned elem_written = 0;

  int data_width = stream.data_width();
  /*
  if(stream._shift_bytes==2) {
    data_width = 2;
  }
  */
  addr_t prev_addr = addr - data_width;

  std::vector<uint8_t> data;
  data.resize(MEM_WIDTH);
  uint8_t *data8 = data.data();
  uint16_t *data16 = (uint16_t *)data8;
  uint32_t *data32 = (uint32_t *)data8;
  uint64_t *data64 = (uint64_t *)data8;

  // go while stream and port does not run out
  while (addr < max_addr && (addr == (prev_addr + data_width)) &&
         stream.stream_active() && ovp.mem_size() > 0) {
    SBDT val = ovp.peek_out_data();

    if (data_width == 1) {
      data8[elem_written++] = (uint8_t)(val & 0xFF);
    } else if (data_width == 2) {
      data16[elem_written++] = (uint16_t)(val & 0xFFFF);
    } else if (data_width == 4) { // assuming data_width=64
      data32[elem_written++] = (uint32_t)(val & 0xFFFFFFFF);
    } else { // assuming data_width=64
      data64[elem_written++] = val;
    }

    /*
if(stream._shift_bytes==2) {
  data16[elem_written++]=(uint16_t)(val&0xFFFF);
} else { //assuming data_width=64
  data64[elem_written++]=val;
}
    */
    ovp.pop_out_data(); // pop the one we peeked
    // timestamp(); cout << "POPPED b/c Mem Write: " << vp.port() << " " <<
    // vp.mem_size() << "\n";

    prev_addr = addr;
    addr = stream.pop_addr();
  }

  unsigned bytes_written = elem_written * data_width;
  SSMemReqInfoPtr sdInfo =
      new SSMemReqInfo(stream.id(), _accel->_accel_index, MEM_WR_STREAM);

  // make store request
  _accel->_lsq->pushRequest(stream.minst(), false /*isLoad*/, data8,
                            bytes_written, init_addr, 0 /*flags*/, 0 /*res*/,
                            sdInfo);

  if (SS_DEBUG::MEM_REQ) {
    _accel->timestamp();
    cout << bytes_written
         << "-byte write request for port->dma, addr:" << std::hex << init_addr
         << ", data:";
    for (int i = 0; i < elem_written; ++i) {
      cout << data64[i] << ", ";
    }
    cout << "\n" << std::dec;
  }

  _accel->_stat_mem_bytes_wr += bytes_written;

  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, bytes_written);
    _accel->_stat_tot_stores++;
    _accel->_stat_tot_mem_stored += bytes_written;
    // bool l2_miss=(cycle_mem_complete-start_cycle)>5;
    // if(l2_miss) {
    //  _accel->_stat_tot_mem_load_acc++;
    //}
  }
}

static bool addr_valid_dir(int64_t acc_size, uint64_t addr, uint64_t prev_addr,
                           uint64_t base_addr, uint64_t max_addr) {
  if (acc_size >= 0) {
    return (addr < max_addr) && (addr > prev_addr || prev_addr == SCRATCH_SIZE);
  } else { // reverse access enabled
    return (addr >= base_addr) && (addr < prev_addr);
  }
}

// serve only when it comes from the requested location, is_banked used only in
// asser
vector<uint8_t> scratch_read_controller_t::read_scratch(affine_read_stream_t &stream, bool is_banked){
  vector<uint8_t> data;
  addr_t prev_addr = SCRATCH_SIZE;
  addr_t addr = stream.cur_addr(); // this is scratch addr
  addr_t base_addr = addr & SCR_MASK;
  addr_t max_addr = base_addr + SCR_WIDTH;

  if (SS_DEBUG::SCR_BARRIER) {
    _accel->timestamp();
    cout << "scr_rd " << hex << addr << " -> " << max_addr << "\n";
  }
  int data_width = stream.data_width(); // should be 2 for my case

  std::fill(mask.begin(), mask.end(), 0);

  uint8_t val[8]; // maximum value that data width can be
  for (int i = 0; i < 8; ++i) {
    val[i] = 0;
  }

  // cout << "CAME FROM BANKED PARTITION? " << is_banked
  //      << " Is the address linear? " << _accel->isLinearSpad(addr)
  //      << " data_width: " << stream.data_width()
  //      << " addr is: " << addr << "\n";

  // go while stream and port does not run out
  while (stream.stream_active() &&
         // (!_accel->_linear_spad || (is_banked ^ _accel->isLinearSpad(addr))) &&
         addr_valid_dir(stream.access_size(), addr, prev_addr, base_addr,
                        max_addr)) {
    for (int i = 0; i < data_width; ++i) {
      val[i] = 0;
    }
    if (is_banked) {
      assert(addr + data_width <= SCRATCH_SIZE);
    } else {
      if(!(addr + data_width >= SCRATCH_SIZE &&
             addr + data_width <= SCRATCH_SIZE + LSCRATCH_SIZE)) {
        cout << "Addr: " << addr << " data_width: " << data_width << endl;
      }
      assert(addr + data_width >= SCRATCH_SIZE &&
             addr + data_width <= SCRATCH_SIZE + LSCRATCH_SIZE);
    }

    SBDT check_value=0;
    _accel->read_scratchpad(&check_value, addr, data_width, stream.id());
    // check_value = check_value & ((1<<8*data_width)-1);
    // cout << "COrrected value read from scratchpad at addr: " << addr << " with data_width: " << data_width << " is: " << check_value << endl;
    _accel->read_scratchpad(&val[0], addr, data_width, stream.id());


    // _accel->read_scratchpad(&check_value, scr_addr, request._output_bytes, 0);

    if (SS_DEBUG::SCR_ACC) {
      // assume 16-bit to debug just for now
      cout << "scr_addr:" << hex << addr << " read " << (val[1] << 8 | val[0])
           << " to port " << stream.first_in_port()
           << " with input port width: " << data_width << "\n";
    }
    for (int i = 0; i < data_width; ++i) {
      data.push_back(val[i]);
    }
    prev_addr = addr;
    for (int i = 0; i < data_width; ++i) {
      mask[addr - base_addr + i] = 1;
    }

    addr = stream.pop_addr();

    if (stream.stride_fill() && stream.stride_hit()) {
      break;
    }
  }
  if (SS_DEBUG::VERIF_SCR) {
    _accel->scr_rd_verif << hex << setw(8) << setfill('0') << base_addr << " ";
    for (uint64_t i = base_addr; i < max_addr; ++i) {
      _accel->scr_rd_verif << setw(2) << setfill('0')
                           << (unsigned)_accel->scratchpad[i];
    }
    _accel->scr_rd_verif << " ";
    for (unsigned i = 0; i < SCR_WIDTH / data_width; ++i) {
      _accel->scr_rd_verif << setw(1) << setfill('0') << (unsigned)mask[i];
    }
    _accel->scr_rd_verif << "\n";
  }

  _accel->_stat_scr_bytes_rd += data.size() * data_width;
  _accel->_stat_scratch_read_bytes += data.size() * data_width;

  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, data.size() * data_width);
    _accel->_stat_scratch_reads++;
  }
  return data;
}

// Figure out which port is the most needy
// this has linear read streams as well!
float scratch_read_controller_t::calc_min_port_ready(bool is_banked) {
  float min_port_ready = MAX_PORT_READY;
  for (int i = 0; i < _scr_port_streams.size(); ++i) {
    auto &stream = *_scr_port_streams[i];
    if (stream.stream_active() && (is_banked ^ checkLinearSpadStream(stream))) {
      auto &in_vp = _accel->port_interf().in_port(stream.first_in_port());
      int data_width = stream.data_width();
      if (in_vp.can_push_vp(SCR_WIDTH / data_width)) {
        float num_ready = in_vp.instances_ready();
        if (num_ready < min_port_ready) {
          min_port_ready = num_ready;
        }
      }
    }
  }
  return min_port_ready;
}

// considering ind banked read streams
float scratch_read_controller_t::calc_min_ind_port_ready() {
  float min_port_ready = MAX_PORT_READY;
  for (int i = 0; i < _ind_port_streams.size(); ++i) {
    auto &stream = *_ind_port_streams[i];
    port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);
    if ((ind_vp.mem_size() && stream.stream_active() && !checkLinearSpadStream(stream)) || indirect_scr_read_requests_active()) {
      auto &in_vp = _accel->port_interf().in_port(stream.first_in_port());
      int data_width = stream.data_width();
      if (in_vp.can_push_vp(SCR_WIDTH / data_width)) {
        float num_ready = in_vp.instances_ready();
        if (num_ready < min_port_ready) {
          min_port_ready = num_ready;
        }
      }
    }
  }
  return min_port_ready;
}

float dma_controller_t::calc_min_port_ready() {
  float min_port_ready = MAX_PORT_READY;
  for (int i = 0; i < _read_streams.size(); ++i) {
    auto &stream = *_read_streams[i];
    if (stream.stream_active()) {
      auto &in_vp = _accel->port_interf().in_port(stream.first_in_port());
      int data_width = stream.data_width();
      if (in_vp.can_push_vp(SCR_WIDTH / data_width)) {
        float num_ready = in_vp.instances_ready();
        if (num_ready < min_port_ready) {
          min_port_ready = num_ready;
        }
      }
    }
  }
  return min_port_ready;
}

// FIXME: test when the input and output port is 1-byte
void network_controller_t::multicast_data(
    remote_port_multicast_stream_t &stream, int message_size) {
  port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);
  int remote_in_port = stream._remote_port;
  uint64_t bytes_written = 0;
  uint64_t remote_elem_sent = 0;

  int data_width = out_vp.get_port_width(); // in bytes
  uint8_t val[64]; // number of 8-byte elements to send

  if (stream.stream_active() && out_vp.mem_size() >= message_size && out_vp.mem_size()) {
    while (stream.stream_active() // enough in dest
           && out_vp.mem_size() &&
           bytes_written < 64) { // enough in source (64-bytes)
      // TODO: peek out and pop later if message buffer size was full?
      
      // POP data_width amount of data
      SBDT data = out_vp.pop_out_data();
      for(int i=0; i<data_width; i++){
        val[i+remote_elem_sent*data_width] = (data >> (i*8)) & 255;
      }

      bytes_written += data_width;
      remote_elem_sent+=1;
      stream._num_elements--;

      if (SS_DEBUG::NET_REQ) {
        timestamp();
        cout << "POPPED b/c port->remote port WRITE: " << out_vp.port() << " "
             << out_vp.mem_size() << " data: " << std::hex << data << "\n";
        printf("After issue: ");
        stream.print_status();
      }
    }
    // if(stream._out_port==7) { cout << "Core id: " << _accel->get_core_id() << " REMOTE BYTES SENT: " << (remote_elem_sent*data_width) << endl; }
    _accel->_lsq->push_spu_req(stream._out_port, remote_in_port, val, remote_elem_sent*data_width, stream._core_mask);
  }
  // TODO: do we need n/w ctrl bus bandwidth util
  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, bytes_written);
    _accel->_stat_port_multicast += remote_elem_sent;
  }
}

// indirect stream
// TODO: incomplete
void network_controller_t::write_remote_scr(remote_scr_stream_t &stream) {
  /*
  port_data_t& val_vp = _accel->port_interf().out_port(stream._out_port);
  port_data_t& addr_vp = _accel->port_interf().out_port(stream._addr_port);
  int64_t base_addr = stream._remote_scr_base_addr;
  uint64_t bytes_written=0;
  uint64_t remote_elem_sent=0;
  while(stream.stream_active() //enough in dest
            && val_vp.mem_size() && addr_vp.mem_size() && bytes_written<64) {
  //enough in source (64-bytes)
    // peek out and pop later if message buffer size was full?
    SBDT val = val_vp.pop_out_data();
    SBDT index = addr_vp.pop_out_data();

          addr_t final_scr_addr = base_addr + index;
          // TODO:FIXME: decide on addressing bits
          // printf("val: %ld and addr: %ld\n",val,final_scr_addr);
    // Total address bits = log((linear_spad_size*C),2)
    int spad_offset_bits = 0;
          uint64_t scr_offset=0;

    spad_offset_bits = (int)log2(SCRATCH_SIZE+LSCRATCH_SIZE);
          scr_offset = final_scr_addr & (SCRATCH_SIZE+LSCRATCH_SIZE-1);
    // TODO: can i get num_cores info here somehow?
    int cores = 64;
        int dest_core_id = (final_scr_addr >> spad_offset_bits) & (cores-1); //
  last bits? if(SS_DEBUG::NET_REQ){ std::cout << "dest_core_id: " <<
  dest_core_id << "\n"; std::cout << "scr_offset: " << scr_offset << "\n";
        }
    // _accel->_lsq->push_spu_scr_wr_req(stream._scr_type, val, scr_offset,
  dest_core_id, stream.id());
    // _accel->_lsq->push_spu_scr_wr_req(stream._scr_type, val, scr_offset,
  dest_core_id, stream.id());

    bytes_written += DATA_WIDTH;
    remote_elem_sent+=1;
    stream._num_elements--;

        if(SS_DEBUG::NET_REQ){
      timestamp(); cout << "POPPED b/c port->remote scr WRITE: " <<
  addr_vp.port() << " " << addr_vp.mem_size() << " and " << val_vp.port() << " "
  << val_vp.mem_size() << "\n";

          printf("After issue: ");
          stream.print_status();
    }
  }
  if(_accel->_ssim->in_roi()) {
    // add_bw(stream.src(), stream.dest(), 1, bytes_written);
    _accel->_stat_remote_scratch_writes+=remote_elem_sent;
  }
  */
}

void network_controller_t::write_direct_remote_scr(
    direct_remote_scr_stream_t &stream) {
  port_data_t &val_vp = _accel->port_interf().out_port(stream._out_port);
  uint64_t bytes_written = 0;
  uint64_t remote_elem_sent = 0;

  uint8_t val[64]; // there will be only 1 base address
  int message_size = 64/stream.data_width(); // num of data_width elements to be written
  if(stream._num_elements<64/stream.data_width()){
    message_size=stream._num_elements;
  }
  if (stream.stream_active() && val_vp.mem_size() >= message_size) {
    addr_t final_scr_addr = stream.cur_addr();
    addr_t base_addr = final_scr_addr & SCR_MASK;
    addr_t max_addr = base_addr + SCR_WIDTH;

    int spad_offset_bits = 0;
    uint64_t scr_offset=0;
    spad_offset_bits = (int)log2(SCRATCH_SIZE+LSCRATCH_SIZE);
    // TODO: set this as environment variable
    int cores = NUM_SPU_CORES;
    int dest_core_id = (final_scr_addr >> spad_offset_bits) & (cores-1); // last bits?
    if(SS_DEBUG::NET_REQ){
      std::cout << "dest_core_id: " << dest_core_id << "\n";
      std::cout << "scr_offset: " << scr_offset << "\n";
    }
    while (final_scr_addr < max_addr && stream.stream_active() // enough in dest
           && val_vp.mem_size() &&
           bytes_written < 64) { // enough in source (64-bytes)
      // peek out and pop later if message buffer size was full?
      SBDT data = val_vp.pop_out_data();
      for(int i=0; i<stream.data_width(); i++){
        val[remote_elem_sent*stream.data_width()+i] = (data >> (i*8)) & 255;
      }

      // addr_t final_scr_addr = base_addr + index;
      // printf("val: %ld and addr: %ld\n",val,final_scr_addr);
      // FIXME: check if this is correct
      final_scr_addr = stream.cur_addr();
      bytes_written += stream.data_width();
      remote_elem_sent+=1;
      // stream._num_elements--;

      if (SS_DEBUG::NET_REQ) {
        timestamp();
        cout << "POPPED b/c direct port->remote scr WRITE: " << val_vp.port()
             << " " << val_vp.mem_size() << "\n";
        printf("After issue: ");
        stream.print_status();
      }
    }
    // _accel->_lsq->push_spu_scr_wr_req(stream._scr_type, val, scr_offset, dest_core_id, stream.id());
    // num_bytes = remote_elem_sent*8
    _accel->_lsq->push_spu_scr_wr_req(&val[0], remote_elem_sent*stream.data_width(), 
                                      scr_offset, dest_core_id, stream.id());
  }
  if (_accel->_ssim->in_roi()) {
    add_bw(stream.src(), stream.dest(), 1, bytes_written);
    _accel->_stat_remote_scratch_writes += remote_elem_sent;
  }
}

void network_controller_t::cycle() {
  int i = 0;
  // cycle over all the streams
  for (i = 0; i < _remote_streams.size(); ++i) {
    _which_remote =
        (_which_remote + 1) >= _remote_streams.size() ? 0 : _which_remote + 1;
    base_stream_t *s = _remote_streams[_which_remote];

    // derivative should always come first
    if (auto *sp = dynamic_cast<direct_remote_scr_stream_t *>(s)) {
      auto &stream = *sp;
      if (stream.stream_active()) {
        port_data_t &val_vp = _accel->port_interf().out_port(stream._out_port);
        // we can send max of message_size*bus_width messages in 1 cycle or till
        // the queue is full (Oh, this bus is also 512-bit bus)
        write_direct_remote_scr(stream);

        bool is_empty = stream.check_set_empty();
        if (is_empty) {
          _accel->process_stream_stats(stream);
          if (SS_DEBUG::VP_SCORE2) {
            cout << "SOURCE: DIRECT PORT->REMOTE SCR\n";
          }
          if (SS_DEBUG::NET_REQ) {
            printf("Direct remote scratchpad write stream empty\n");
          }
          val_vp.set_status(port_data_t::STATUS::FREE);
          delete_stream(_which_remote, sp);
        }
        break;
      }
    }
    if (auto *sp = dynamic_cast<remote_scr_stream_t *>(s)) {
      auto &stream = *sp;
      if (stream.stream_active()) {
        port_data_t &val_vp = _accel->port_interf().out_port(stream._out_port);
        port_data_t &addr_vp =
            _accel->port_interf().out_port(stream._addr_port);
        // we can send max of message_size*bus_width messages in 1 cycle or till
        // the queue is full (Oh, this bus is also 512-bit bus)
        write_remote_scr(stream);

        bool is_empty = stream.check_set_empty();
        if (is_empty) {
          _accel->process_stream_stats(stream);
          if (SS_DEBUG::VP_SCORE2) {
            cout << "SOURCE: INDIRECT PORT->REMOTE SCR\n";
          }
          if (SS_DEBUG::NET_REQ) {
            printf("Indirect remote scratchpad write stream empty\n");
          }
          val_vp.set_status(port_data_t::STATUS::FREE);
          addr_vp.set_status(port_data_t::STATUS::FREE);
          delete_stream(_which_remote, sp);
        }
        break;
      }
    } else if (auto *sp = dynamic_cast<remote_port_multicast_stream_t *>(s)) {
      auto &stream = *sp;
      port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);
      int message_size=100000; // infinity
      if (stream.stream_active()) {

        int data_width = out_vp.get_port_width(); // in bytes
        message_size = 64/data_width; // num of data_width elements to be written
        if(stream._num_elements<64/data_width){
          message_size=stream._num_elements;
        }
       if(stream.timeout()) {
          message_size = out_vp.mem_size() < message_size ? out_vp.mem_size() : message_size;
        } else if(out_vp.mem_size() < message_size) { // this should be here if no timeout
          stream.inc_wait_cycles();
          continue;
        }
      }
      if (stream.stream_active() && out_vp.mem_size() >= message_size) {

        // we can send max of message_size*bus_wisth messages in 1 cycle or till
        // the queue is full (Oh, this bus is also 512-bit bus)
        multicast_data(stream, message_size);
        
        bool is_empty = stream.check_set_empty();
        if (is_empty) {
          _accel->process_stream_stats(stream);
          if (SS_DEBUG::VP_SCORE2) {
            cout << "SOURCE: PORT->REMOTE PORT\n";
          }
          if (SS_DEBUG::NET_REQ) {
            printf("Multicast stream empty\n");
          }
          out_vp.set_status(port_data_t::STATUS::FREE);
          delete_stream(_which_remote, sp);
        }
        break;
      }
    }
  }
}

void network_controller_t::finish_cycle() {}

void network_controller_t::print_status() {
  for (auto &i : _remote_streams) {
    if (!i->empty()) {
      i->print_status();
    }
  }
}

bool scratch_read_controller_t::checkLinearSpadStream(indirect_stream_t& stream) {
  port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);
  addr_t addr = stream.cur_addr(ind_vp.peek_out_data());
  return _accel->isLinearSpad(addr);
}

bool scratch_read_controller_t::checkLinearSpadStream(affine_read_stream_t& stream) {
  addr_t addr = stream.cur_addr(); // this is scratch addr
  return _accel->isLinearSpad(addr);
}

void scratch_read_controller_t::cycle(bool &performed_read) {
  float min_port_ready = -2;
  float min_ind_port_ready = -2;

  cycle_read_queue();

  int i = 0;
  // FOR BANKED SCRATCHPAD
  for (i = 0; i < _read_streams.size(); ++i) {
    _which_rd = (_which_rd + 1) >= _read_streams.size() ? 0 : _which_rd + 1;
    base_stream_t *s = _read_streams[_which_rd];

    if (auto *sp = dynamic_cast<affine_read_stream_t *>(s)) {
      auto &stream = *sp;

      if (stream.stream_active()) {
        int data_width = stream.data_width();
        bool skip_check = SS_DEBUG::UNREAL_INPUTS;

        if (min_port_ready == -2) {
          min_port_ready = calc_min_port_ready(true);
        }
        if (!skip_check && min_port_ready >= MAX_PORT_READY)
          continue;

        auto &first_in_vp =
            _accel->port_interf().in_port(stream.first_in_port());
        float num_ready = first_in_vp.instances_ready();

        // if(skip_check || (first_in_vp.can_push_vp(SCR_WIDTH/DATA_WIDTH)
        if ((skip_check || (first_in_vp.can_push_vp(SCR_WIDTH / data_width) &&
                           num_ready == min_port_ready)) && !checkLinearSpadStream(stream)) {
          // vector<SBDT> data = read_scratch(stream, true); // true means it is
          // banked
          vector<uint8_t> data = read_scratch(stream, true);

          for (int in_port : stream.in_ports()) {
            auto &in_vp = _accel->port_interf().in_port(in_port);
            // data is a vector of bytes
            in_vp.push_data(data); // just push in the vector
            /*for(auto d : data) {
              in_vp.push_data_byte(d);
            }*/

            if (stream.stride_fill() && stream.stride_hit()) {
              in_vp.fill(true);
            }
          }

          bool is_empty = stream.check_set_empty();
          if (is_empty) {
            _accel->process_stream_stats(stream);

            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: SCR->PORT\n";
            }
            for (int in_port : stream.in_ports()) {
              auto &in_vp = _accel->port_interf().in_port(in_port);
              in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                               stream.fill_mode());
            }
            delete_stream(_which_rd, sp);
          }
          break;
        }
      }

    } else if (auto *sp = dynamic_cast<indirect_stream_t *>(s)) {
      auto &stream = *sp;
      port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);

      if ((ind_vp.mem_size() >= stream._index_bytes && stream.stream_active() && !checkLinearSpadStream(stream)) ||
          indirect_scr_read_requests_active()) {
        //-----------------------------------
        // cout << "Detected as a banked scratchpad\n";
        bool skip_check = SS_DEBUG::UNREAL_INPUTS;

        if (min_ind_port_ready == -2) {
          min_ind_port_ready = calc_min_ind_port_ready();
        }
        if (!skip_check && min_ind_port_ready >= MAX_PORT_READY)
          continue;
        //-------------------------------------


        // if(checkLinearSpadStream(stream)) continue;
        read_scratch_ind(stream, -1 /*scratch*/);
        if (stream.empty()) {
          delete_stream(_which_rd, sp);
        }
        break;
      }
    } else {
    }
    //} else if(auto* sp = dynamic_cast<scr_scr_stream_t*>(s)) {
    //  auto& stream = *sp;
    //  if(stream.stream_active()) {
    //    if(is_shared()) { //destination is remote write buf
    //      if(get_ssim()->can_push_shs_buf(8,stream._scratch_addr,
    //                                      stream._remote_bitmask)) {
    //        //get data, push it to all scratchpads
    //        vector<SBDT> data = read_scratch(stream);

    //        for(uint64_t i=0,b=1; i < NUM_ACCEL_TOTAL; ++i, b<<=1) {
    //          if(stream._remote_bitmask & b) {
    //            auto& buf = get_ssim()->get_acc(i)->scr_w_c()->_buf_shs_write;
    //            buf.push_data(stream._scratch_addr, data);
    //            buf.set_last_stream_id(stream.id());
    //          }
    //        }

    //        bool is_empty = stream.check_set_empty();
    //        if(is_empty) {
    //          _accel->process_stream_stats(stream);
    //          stream._remote_stream->reset(); //free for later
    //        }
    //
    //        stream._scratch_addr+=data.size()*DATA_WIDTH;
    //        break;
    //      }
    //    } else { //destination is local read buf
    //      if(stream.stream_active() && _buf_shs_read.can_push_addr(8,
    //                                                stream._scratch_addr)) {
    //        bool last =
    //        xfer_stream_buf(stream,_buf_shs_read,stream._scratch_addr);
    //        if(last) {
    //          stream._remote_stream->reset(); //free for later
    //        }
    //        break;
    //      }
    //    }
    //  }
    //}
  }
  if (i < _read_streams.size()) {
    // performed_read = false;
    performed_read = true;
  } else {
    // performed_read = true;
    performed_read = false;
  }
}

// FIXME: have to do this because of weird atomic scr logic, TODO: combine scratch
// read and write controllers
void scratch_read_controller_t::linear_scratch_cycle() {
  // FOR LINEAR SCRATCHPAD
  if (_accel->_linear_spad) {
    float min_port_ready=-2;
    for (int i = 0; i < _read_streams.size(); ++i) {
      _which_rd = (_which_rd + 1) >= _read_streams.size() ? 0 : _which_rd + 1;
      // _which_rd=(_which_rd+1)>=_scr_port_streams.size() ? 0:_which_rd+1;
      base_stream_t *s = _read_streams[_which_rd];

      if (auto *sp = dynamic_cast<affine_read_stream_t *>(s)) {
        auto &stream = *sp;

        if (stream.stream_active()) {
          bool skip_check = SS_DEBUG::UNREAL_INPUTS;

          if (min_port_ready == -2) {
            min_port_ready = calc_min_port_ready(false);
          }
          if (!skip_check && min_port_ready >= MAX_PORT_READY)
            continue;

          auto &first_in_vp =
              _accel->port_interf().in_port(stream.first_in_port());
          float num_ready = first_in_vp.instances_ready();
          int data_width = first_in_vp.get_port_width();

          if ((skip_check || (first_in_vp.can_push_vp(SCR_WIDTH / data_width) &&
                             num_ready == min_port_ready)) && checkLinearSpadStream(stream)) {
            vector<uint8_t> data = read_scratch(stream, false);

            for (int in_port : stream.in_ports()) {
              auto &in_vp = _accel->port_interf().in_port(in_port);
              in_vp.push_data(data);
              if (stream.stride_fill() && stream.stride_hit()) {
                in_vp.fill(true);
              }
            }

            bool is_empty = stream.check_set_empty();
            if (is_empty) {
              _accel->process_stream_stats(stream);

              if (SS_DEBUG::VP_SCORE2) {
                cout << "SOURCE: LINEAR SCR->PORT\n";
              }
              for (int in_port : stream.in_ports()) {
                auto &in_vp = _accel->port_interf().in_port(in_port);
                in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                                 stream.fill_mode());
              }
              delete_stream(_which_rd, sp);
            }
            break;
          }
        }
      } else if (auto *sp = dynamic_cast<indirect_stream_t *>(s)) {
        auto &stream = *sp;
        port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);
        auto &subsize_vp = _accel->port_interf().out_port(stream._sn_port);

        if (ind_vp.mem_size() && stream.stream_active() && checkLinearSpadStream(stream)
     && (!stream._is_2d_stream || (stream._is_2d_stream && subsize_vp.mem_size() >= stream._index_bytes))) {
          // || indirect_scr_read_requests_active()) {
          // TODO: check the address belongs to the linear spad
          // if(!checkLinearSpadStream(stream)) continue;
          // cout << "Detected as a linear scratchpad\n";
          read_linear_scratch_ind(stream, -1 /*scratch*/);

          bool is_empty = stream.check_set_empty();
          if (is_empty) {
            _accel->process_stream_stats(stream);

            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: INDIRECT LINEAR SCR->PORT\n";
            }
      
            // free ind port
            port_data_t &ind_vp = _accel->port_interf().out_port(stream._ind_port);
            ind_vp.set_status(port_data_t::STATUS::FREE);
            // free input ports
            for (int in_port : stream.in_ports()) {
              auto &in_vp = _accel->port_interf().in_port(in_port);
              in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                               stream.fill_mode());
            }
            delete_stream(_which_rd, sp);
          }
          /*
          if (stream.empty()) {
            delete_stream(_which_rd, sp);
          }
          */
          break;
        }
      }
    }
  }
}

void scratch_read_controller_t::finish_cycle() {}

void scratch_read_controller_t::print_status() {
  for (auto &i : _read_streams) {
    if (!i->empty()) {
      i->print_status();
    }
  }
}

bool scratch_write_controller_t::crosssar_backpressureOn() {
  for (int i = 0; i < NUM_SCRATCH_BANKS; ++i) {
    if (_atomic_scr_issued_requests[i].size() == MAX_BANK_BUFFER_SIZE)
      return true;
  }
  return false;
}

// TODO: Do it in a neater way!
void scratch_write_controller_t::scr_write(addr_t addr,
                                           affine_write_stream_t &stream,
                                           port_data_t &out_vp) {
  int data_width = out_vp.get_port_width();
  if (data_width == 1) {
    uint8_t val =
        out_vp.pop_out_custom_data<uint8_t>(); // this is te SBDT form of data
    _accel->write_scratchpad(addr, &val, data_width, stream.id());
  } else if (data_width == 2) {
    uint16_t val =
        out_vp.pop_out_custom_data<uint16_t>(); // this is te SBDT form of data
    _accel->write_scratchpad(addr, &val, data_width, stream.id());
  } else if (data_width == 4) {
    uint32_t val =
        out_vp.pop_out_custom_data<uint32_t>(); // this is te SBDT form of data
    _accel->write_scratchpad(addr, &val, data_width, stream.id());
  } else {
    uint64_t val =
        out_vp.pop_out_custom_data<uint64_t>(); // this is te SBDT form of data
    _accel->write_scratchpad(addr, &val, data_width, stream.id());
  }
}

void scratch_write_controller_t::serve_atomic_requests(bool &performed_atomic_scr) {
  SBDT input_val=0;
  addr_t scr_addr; 
  int opcode = 0;
  SBDT inc, bytes_written=0;
  // int logical_banks = NUM_SCRATCH_BANKS / stream._value_bytes;
  for (int i = 0; i < _logical_banks; ++i) {
    if (!_atomic_scr_issued_requests[i].empty()) {
      atomic_scr_op_req request = _atomic_scr_issued_requests[i].front();
      input_val = 0; // reset
      scr_addr = request._scr_addr;
      inc = request._inc;
      opcode = request._opcode;

      // cout << "Initial input val of atomic scr: " << input_val << endl;
      // _accel->read_scratchpad(&input_val+8-request._output_bytes, scr_addr, request._output_bytes, stream.id());
      // FIXME: confirm if I can fix the ID of atomic scr
      // _accel->read_scratchpad(&input_val+8-request._output_bytes, scr_addr, request._output_bytes, 0);
      SBDT check_value=0;
      _accel->read_scratchpad(&check_value, scr_addr, request._output_bytes, 0);
      // cout << "ATOMIC SCR: addr it rd/writes to: " << scr_addr << " the value: " << input_val << " and inc: " << inc << endl;
      // cout << "CHECKED CORRECT VALUE: " << check_value << endl;
      input_val = check_value & ((1<<request._output_bytes*8)-1);

      if (SS_DEBUG::COMP) {
        // std::cout << (_accel->now()-_accel->_ssim->_stat_start_cycle) << " ";
        timestamp();
        printf("ACCEL ID: %d", _accel->_lsq->getCpuId());
        std::cout << " REAL EXECUTION, update at scr_addr: " << scr_addr
                  << " at bankid: " << i << " with inc value: " << inc
                  <<  " and old value: " << input_val
                  << "\n";
        // std::cout << "Available requests at the bank queue are: "
        //           << _atomic_scr_issued_requests[i].size() << "\n";
      }

      switch (opcode) {
        case 0: input_val += inc;
                break;
        case 1: input_val = std::max(input_val, inc);
                break;
        case 2: input_val = std::min(input_val, inc);
                break;
        case 3: input_val = inc; // update
                break;
        default: cout << "Invalid opcode\n";
                 break;
      }

      uint16_t correct_val = input_val & ((1<<request._output_bytes*8)-1);

      // cout << "FInal output of atomic scr: " << correct_val << endl;
      /*
      _accel->write_scratchpad(scr_addr,
                               &input_val + 8 - request._output_bytes,
                               request._output_bytes, 0);
      */
                               // request._output_bytes, stream.id());

      _accel->write_scratchpad(scr_addr,
                               &correct_val,
                               request._output_bytes, 0);
      
      check_value=0;
      _accel->read_scratchpad(&check_value, scr_addr, request._output_bytes, 0);
      // cout << "Updated value at the address: " << scr_addr << " is: " << check_value << endl;
      _accel->_stat_scratch_bank_requests_executed++;
      bytes_written += request._value_bytes;
      _accel->_stat_scr_bytes_wr += request._value_bytes;
      _accel->_stat_scratch_write_bytes += request._value_bytes;
      _atomic_scr_issued_requests[i].pop();
    }
  }

  if (_accel->_ssim->in_roi()) {
    // add_bw(stream.src(), stream.dest(), 1, bytes_written);
    add_bw(LOC::PORT, LOC::SCR, 1, bytes_written);
    _accel->_stat_scratch_writes += 1;
    _accel->_stat_cycles_atomic_scr_executed++;
  }
  // to implement the functionality of common scratch controller
  performed_atomic_scr = true;
}

void scratch_write_controller_t::push_atomic_update_req(int scr_addr, int opcode, int val_bytes, int out_bytes, uint64_t inc) {
  struct atomic_scr_op_req temp_req;
  temp_req._scr_addr = scr_addr;
  temp_req._inc = inc; 
  temp_req._opcode = opcode;
  temp_req._value_bytes = val_bytes;
  temp_req._output_bytes = out_bytes;
    
  // FIXME: not sure if this is correct!
  int logical_banks = NUM_SCRATCH_BANKS / val_bytes;

  int bank_id = 0;

  // by default is row interleaving for now
  if (_accel->_banked_spad_mapping_strategy &&
      (strcmp(_accel->_banked_spad_mapping_strategy, "COL") == 0)) {
    // bank_id = (scr_addr >> 9) & (logical_banks - 1);
    bank_id = (scr_addr >> 4) & (logical_banks - 1); // log64 = 6 = log2(k)
  } else {
    bank_id = (scr_addr >> 1) & (logical_banks - 1);
  }

  assert(bank_id < logical_banks);
  _atomic_scr_issued_requests[bank_id].push(temp_req);
}

void scratch_write_controller_t::atomic_scratch_update(atomic_scr_stream_t &stream) {
  SBDT loc;
  addr_t scr_addr, max_addr;
  int logical_banks = NUM_SCRATCH_BANKS;
  int bank_id = 0;
  // added to correct bank conflict calculation
  int num_addr_pops = 0;
  int num_val_pops = 0;

  port_data_t &out_addr = _accel->port_interf().out_port(stream._out_port);
  port_data_t &out_val = _accel->port_interf().out_port(stream._val_port);

  // strides left in the stream or requests left in the queue
  // if(stream.stream_active() || atomic_scr_issued_requests_active()) {
  if (stream.stream_active()) {
    logical_banks = NUM_SCRATCH_BANKS / stream._value_bytes;

    addr_t base_addr = stream._mem_addr; // this is like offset
    int n=0;

    if (out_addr.mem_size() > 0 && out_val.mem_size() > 0 &&
        stream._value_bytes*n < 64 && // number of pushes should be max 
        !crosssar_backpressureOn()) { // enough in src and dest
      n++;

      // hopefully it pushes data here
      if (_accel->_ssim->in_roi()) {
        _accel->_stat_cycles_atomic_scr_pushed++;
      }

      num_addr_pops = 0;
      num_val_pops = 0;
      loc = out_addr.peek_out_data();
      if (SS_DEBUG::COMP) {
        std::cout << "1 cycle execution : "
                  << "\n";
        std::cout << "64-bit value at the addr port is: " << loc;
      }
      loc = stream.cur_addr(loc);
      // cout << "cur addr index: " << stream._cur_addr_index << " addr bytes: " << stream._addr_bytes << endl;
      // std::cout << "updated loc: " << loc << "\n";

      base_addr = stream.cur_offset() * stream._output_bytes;
      // std::cout << "updated base address: " << base_addr << "\n";
      scr_addr = base_addr + loc * stream._output_bytes;
      // std::cout << "updated scr address: " << scr_addr << "\n";
      max_addr = (scr_addr & SCR_MASK)+SCR_WIDTH;


      // cout << "SCR ADDR: " << scr_addr << " BASE ADDR: " << base_addr << "\n";

      // FIXME: check if this correct
      // max_addr = (scr_addr & stream._value_mask) + stream._value_bytes;

      // go while stream and port does not run out
      // number of iterations of this loop cannot be more than 8*64-bits
      // => cannot pop more than 8 values from the port

      // num_value_pops should be less than 64 bytes?
      // Stage 1: push requests in scratch banks
      while (scr_addr < max_addr && stream._num_strides > 0 &&
             out_addr.mem_size() && out_val.mem_size() &&
             num_val_pops < 64/stream._value_bytes) {
             // num_val_pops < 64) {
        if (SS_DEBUG::COMP) {
          std::cout << "\tupdate at index location: " << loc
                    << " and scr_addr: " << scr_addr
                    << " and scr_size is: " << SCRATCH_SIZE;
        }
        
        // TODO: instead of assert, if this condition holds true, send a remote
        // request (for now let's send a complete packet -- I don't think we
        // want to say about decomposable n/w but we can study that by
        // increasing the network bandwidth)

        // if(scr_addr + stream._output_bytes > SCRATCH_SIZE) { cout << "scratch_addr: " << scr_addr << endl; }
        // assert(scr_addr + stream._output_bytes <= SCRATCH_SIZE);

        struct atomic_scr_op_req temp_req;
        SBDT cur_value = out_val.peek_out_data();
        // cout << "64-bit value popped from out_val port: " << cur_value << endl;
        temp_req._scr_addr = scr_addr;
        temp_req._inc = stream.cur_val(cur_value);
        temp_req._opcode = stream._op_code;
        temp_req._value_bytes = stream._value_bytes;
        temp_req._output_bytes = stream._output_bytes;

        // by default is row interleaving for now
        if (_accel->_banked_spad_mapping_strategy &&
            (strcmp(_accel->_banked_spad_mapping_strategy, "COL") == 0)) {
          // bank_id = (scr_addr >> 9) & (logical_banks - 1);
          // bank_id = (scr_addr >> 6) & (logical_banks - 1); // log64 = 6 = log2(k)
          bank_id = (scr_addr >> 5) & (logical_banks - 1); // log64 = 6 = log2(k)
        } else {
          bank_id = (scr_addr >> 1) & (logical_banks - 1);
        }

        assert(bank_id < logical_banks);

        int local_core_id = (scr_addr + stream._output_bytes)/SCRATCH_SIZE;
        if(SS_DEBUG::NET_REQ) {
          cout << "Sending remote atomic update for scratch_addr: " << scr_addr << " from own core id: " << _accel->_ssim->get_core_id() << " to remote core id: " << local_core_id <<  endl; 
        }
        int local_scr_addr = scr_addr;
        uint64_t x = temp_req._inc;
        _accel->_lsq->push_rem_atom_op_req(x, local_scr_addr, temp_req._opcode, temp_req._value_bytes, temp_req._output_bytes);
 
        _accel->_stat_scratch_bank_requests_pushed++;
        stream._num_strides--;

        if (SS_DEBUG::COMP) {
          std::cout << "\tvalues input: " << temp_req._inc << "\tbank_id: " << bank_id
                    << "\n";
          std::cout << "stream strides left are: " << stream._num_strides
                    << "\n";
        }

        stream.inc_val_index();
        stream.inc_addr_index();

        // pop only if index in word is last?
        if (stream.can_pop_val()) {
          stream._cur_val_index = 0;
          num_val_pops++;
          out_val.pop_out_data();
          if (SS_DEBUG::COMP) {
            std::cout << "\tpopped data from val port";
          }
        }
        if (stream.can_pop_addr()) {
          stream._cur_addr_index = 0;
          // for bank conflicts calc purposes
          num_addr_pops++;
          out_addr.pop_out_data();
          if (SS_DEBUG::COMP) {
            std::cout << "\tpopped data from addr port";
          }
        }
        if (SS_DEBUG::COMP) {
          std::cout << "\n";
        }
        // should be decremented after every computation
        // new loop added for vector ports
        if (out_addr.mem_size() > 0 && out_val.mem_size() > 0) {
          loc = out_addr.peek_out_data();
          if (SS_DEBUG::COMP) {
            std::cout << "64-bit value at the addr port is: " << loc;
          }
          loc = stream.cur_addr(loc);
          // std::cout << "loc: " << loc << "\n";
          // making offset also configurable (same configuration as the
          // address)
          base_addr = stream.cur_offset() * stream._value_bytes;

          // scr_addr = base_addr + loc*stream._value_bytes;
          scr_addr = base_addr + loc * stream._value_bytes;
          // max_addr = (scr_addr & SCR_MASK)+SCR_WIDTH;
          max_addr = (scr_addr & stream._value_mask) + stream._value_bytes;
        }
      }
    }
  }
  if (_accel->_ssim->in_roi()) {
    _accel->_num_cycles_issued++;
  }
  // this should be the case, right?
  // break;
}

void scratch_write_controller_t::cycle(bool can_perform_atomic_scr,
                                       bool &performed_atomic_scr) {

  // bool issued_linear_on_banked_scr = false;
  // bool is_linear_addr_banked = false;

  // FOR THE BANKED SCRATCHPAD
  for (unsigned i = 0; i < _write_streams.size(); ++i) {
    _which_wr = (_which_wr + 1) >= _write_streams.size() ? 0 : _which_wr + 1;
    base_stream_t *s = _write_streams[_which_wr];

    if (auto *sp = dynamic_cast<affine_write_stream_t *>(s)) { // write from
                                                               // port
      auto &stream = *sp;

      if (stream.stream_active()) {
        port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);

        if (out_vp.mem_size() > 0) {
          addr_t addr = stream.cur_addr();
          addr_t base_addr = addr & SCR_MASK;
          addr_t max_addr = base_addr + SCR_WIDTH;

          // go while stream and port does not run out
          uint64_t bytes_written = 0;
          while (!_accel->isLinearSpad(addr) && addr < max_addr &&
                 stream.stream_active()
                 && out_vp.mem_size()) { // enough in source

            int data_width = out_vp.get_port_width();
            assert(((int)addr >= 0) && (addr + data_width <= SCRATCH_SIZE));
            scr_write(addr, stream, out_vp);

            if (SS_DEBUG::SCR_ACC) {
              timestamp();
              cout << "POPPED b/c port->scratch WRITE: " << out_vp.port() << " "
                   << out_vp.mem_size() << "\n";
            }

            // _accel->write_scratchpad(addr, &val, data_width, stream.id());

            addr = stream.pop_addr();

            bytes_written += data_width;
            _accel->_stat_scr_bytes_wr += data_width;
            _accel->_stat_scratch_write_bytes += data_width;
          }
          if (_accel->_ssim->in_roi()) {
            add_bw(stream.src(), stream.dest(), 1, bytes_written);
            _accel->_stat_scratch_writes += 1;
          }

          bool is_empty = stream.check_set_empty();
          if (is_empty) {
            _accel->process_stream_stats(stream);
            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: PORT->SCR\n";
            }
            // to debug
            // _accel->print_spad_addr(0,16);
            out_vp.set_status(port_data_t::STATUS::FREE);
            delete_stream(_which_wr, sp);
          }
          break;
        }
      }

    } else if (auto *sp =
                   dynamic_cast<const_scr_stream_t *>(s)) { // write from port
      auto &stream = *sp;

      if (stream.stream_active()) {
        int const_width = stream._const_width;
        // uint64_t total_pushed=0;
        // addr_t addr = stream._scratch_addr;
        int addr = stream.cur_scratch_addr(); // first will definitely get in
        if(_accel->isLinearSpad(addr)) continue;
        // cout << "IDENTIFIED AS BANKED SCRATCHPAD STREAM\n";

        int base_addr = addr & SCR_MASK;
        int max_addr = base_addr + SCR_WIDTH;

        uint64_t bytes_written = 0;
        while (addr < max_addr && stream.stream_active() &&
               bytes_written < SCR_WIDTH) {
          
          // cout << "Enter addr: " << addr << " and max_addr: " << max_addr << endl;
          // stream.print_status();
          
          SBDT val = stream._constant;

          assert(addr + const_width <= SCRATCH_SIZE);
          _accel->write_scratchpad(addr, &val, const_width, stream.id());
          bytes_written += const_width;
          // cout << "CONST SCR: addr it writes to: " << addr << " the value: " << val << endl;

          if(addr + stream._const_width < max_addr && bytes_written < SCR_WIDTH) {
            addr = stream.cur_scratch_addr();
          } else {
            addr = max_addr;
          }

          _accel->_stat_scr_bytes_wr += const_width;
          _accel->_stat_scratch_write_bytes += const_width;
          // cout << "Exit addr: " << addr << " and max_addr: " << max_addr << endl;
          // stream._iters_left--;
          // addr += const_width;
          // total_pushed++;
        }
        // }
        // add_bw(stream.src(), stream.dest(), 1, total_pushed*DATA_WIDTH);

        if (_accel->_ssim->in_roi()) {
          add_bw(stream.src(), stream.dest(), 1, bytes_written);
          _accel->_stat_scratch_writes += 1;
        }

        bool is_empty = !stream.stream_active(); // stream.check_set_empty();
        if (is_empty) {
          _accel->process_stream_stats(stream);
          if (SS_DEBUG::VP_SCORE2) {
            cout << "SOURCE: CONST->SCR\n";
          }
          delete_stream(_which_wr, sp);
        }
        break;
      }
    } else if (auto *sp = dynamic_cast<indirect_wr_stream_t *>(s)) {
      auto &stream = *sp;
      if (stream.stream_active()) {
        port_data_t &out_port =
            _accel->port_interf().out_port(stream._out_port);
        port_data_t &ind_port =
            _accel->port_interf().out_port(stream._ind_port);

        if (out_port.mem_size() > 0 && ind_port.mem_size() >= stream._index_bytes) {
          write_scratch_ind(stream);
          if (stream.empty()) {
            delete_stream(_which_wr, sp);
          }
          break;
        }
      }
    } else if (auto *sp = dynamic_cast<atomic_scr_stream_t *>(s)) {
      auto &stream = *sp;
      port_data_t &out_addr = _accel->port_interf().out_port(stream._out_port);
      port_data_t &out_val = _accel->port_interf().out_port(stream._val_port);

      // FIXME: mem_size based on config (this should be greater than
      // addr_bytes/out_addr.data_width
      if(out_addr.mem_size() && out_val.mem_size() && stream.stream_active()) {
        // Constraint: This should be same for all active atomic streams
        _logical_banks = NUM_SCRATCH_BANKS / stream._value_bytes;
        atomic_scratch_update(stream);
        bool is_empty = stream.check_set_empty();
        if (is_empty) {
          _accel->process_stream_stats(stream);
          if (SS_DEBUG::VP_SCORE2) {
            cout << "SOURCE: PORT->SCR\n";
          }
          out_addr.set_status(port_data_t::STATUS::FREE);
          out_val.set_status(port_data_t::STATUS::FREE);
          // delete_stream(_which_wr, sp); // _which_wr should not get to 0 though
        }
        // break;
      } 
      if(!(stream.stream_active() || atomic_scr_issued_requests_active())) {
        delete_stream(_which_wr, sp); // if both are done
      }
      /*
      if(atomic_scr_issued_requests_active()) {
        if(can_perform_atomic_scr) {
          serve_atomic_requests(stream, performed_atomic_scr);
          break;
        }
      }*/
    } else if (auto *sp = dynamic_cast<remote_core_net_stream_t *>(s)) {
      auto &stream = *sp;
      // int bytes_written=0;
      if (stream.stream_active()) { // should always be true
        port_data_t &addr_port =
            _accel->port_interf().out_port(stream._addr_port);
        port_data_t &val_port =
            _accel->port_interf().out_port(stream._val_port);

        if (addr_port.mem_size() > 0 && val_port.mem_size() > 0) {
          // printf("COmes to check remote core net stream\n");
          write_scratch_remote_ind(stream);
          /*
          if(stream.empty()) {
            delete_stream(_which_wr,sp);
          }
          */
          break;
        }
      }
    }
  }


  // FIXME:CHECKME:atomic scr requests should be served every cycle -- no matter which stream
  // was issued
  if(atomic_scr_issued_requests_active()) {
    if(can_perform_atomic_scr) { 
      // I see, this info should not be associated with the stream, but in bank buffers
      // serve_atomic_requests(stream, performed_atomic_scr);
      serve_atomic_requests(performed_atomic_scr);
      // break;
    }
  }

  // FOR LINEAR WR STREAMS ON THE LINEAR SCRATCHPAD (issues only if the address
  // belongs to linear spad)
  // if((!issued_linear_on_banked_scr) || (issued_linear_on_banked_scr &&
  // !is_linear_addr_banked)) {
}

void scratch_write_controller_t::linear_scratch_write_cycle() {
  if(!_accel->_linear_spad) return;
  if (_accel->_linear_spad) {
    // for(unsigned i = 0; i < (_port_scr_streams.size()); ++i) {
    for (unsigned i = 0; i < (_write_streams.size()); ++i) {
      // _which_linear_wr=(_which_linear_wr+1)>=(_port_scr_streams.size())?0:_which_linear_wr+1;
      _which_linear_wr = (_which_linear_wr + 1) >= (_write_streams.size()) ? 0 : _which_linear_wr + 1;

      base_stream_t *s = _write_streams[_which_linear_wr];
      if (auto *sp = dynamic_cast<affine_write_stream_t *>(s)) {

        auto &stream = *sp;

        if (stream.stream_active()) {
          port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);

          if (out_vp.mem_size() > 0) {
            addr_t addr = stream.cur_addr();
            addr_t base_addr = addr & SCR_MASK;
            addr_t max_addr = base_addr + SCR_WIDTH;

            // go while stream and port does not run out
            uint64_t bytes_written = 0;
            while (_accel->isLinearSpad(addr) && addr < max_addr &&
                   stream.stream_active()  // enough in dest
                   && out_vp.mem_size()) { // enough in source

              int data_width = stream.data_width();
              // cout << "LINEAR ADDR: " << addr << " is_linear: " << _accel->isLinearSpad(addr) << endl;
              assert(((int)addr >= 0) && (addr + data_width >= SCRATCH_SIZE) &&
                     (addr + data_width <= SCRATCH_SIZE + LSCRATCH_SIZE));
              scr_write(addr, stream, out_vp);

              if (SS_DEBUG::SCR_ACC) {
                timestamp();
                cout << "POPPED b/c port->linear scratch WRITE: "
                     << stream._out_port
                     << " with width: " << stream.data_width()
                     << " strides left: " << stream.num_strides() << " "
                     << out_vp.mem_size() << "\n";
              }

              addr = stream.pop_addr();

              bytes_written += data_width;
              _accel->_stat_scr_bytes_wr += data_width;
              _accel->_stat_scratch_write_bytes += data_width;
            }
            if (_accel->_ssim->in_roi()) {
              add_bw(stream.src(), stream.dest(), 1, bytes_written);
              _accel->_stat_scratch_writes += 1;
            }

            bool is_empty = stream.check_set_empty();
            if (is_empty) {
              _accel->process_stream_stats(stream);
              if (SS_DEBUG::VP_SCORE2) {
                cout << "SOURCE: PORT->SCR\n";
              }
              // _accel->print_spad_addr(SCRATCH_SIZE, SCRATCH_SIZE+16);
              out_vp.set_status(port_data_t::STATUS::FREE);
              delete_stream(_which_wr, sp);
            }
            break;
          }
        }
      } else if (auto *sp = dynamic_cast<const_scr_stream_t *>(s)) {

        auto &stream = *sp;

        if (stream.stream_active()) {
          int const_width = stream._const_width;
          // uint64_t total_pushed=0;
          addr_t addr = stream._scratch_addr;
          addr_t base_addr = addr & SCR_MASK;
          addr_t max_addr = base_addr + SCR_WIDTH;
          if(!_accel->isLinearSpad(addr)) continue;
          // cout << "IDENTIFIED AS LINEAR SCRATCHPAD STREAM\n";

          uint64_t bytes_written = 0;
          while (addr < max_addr && stream._iters_left > 0 &&
                 bytes_written < SCR_WIDTH) {
            SBDT val = stream._constant;

            assert(addr + const_width >= SCRATCH_SIZE);
            assert(addr + const_width <= SCRATCH_SIZE+LSCRATCH_SIZE);
            _accel->write_scratchpad(addr, &val, sizeof(const_width), stream.id());

            bytes_written += const_width;
            _accel->_stat_scr_bytes_wr += const_width;
            _accel->_stat_scratch_write_bytes += const_width;
            stream._iters_left--;
            // total_pushed++;
            addr += sizeof(const_width);
          }
          // }
          // add_bw(stream.src(), stream.dest(), 1, total_pushed*DATA_WIDTH);

          if (_accel->_ssim->in_roi()) {
            add_bw(stream.src(), stream.dest(), 1, bytes_written);
            _accel->_stat_scratch_writes += 1;
          }

          bool is_empty = stream.check_set_empty();
          if (is_empty) {
            _accel->process_stream_stats(stream);
            if (SS_DEBUG::VP_SCORE2) {
              cout << "SOURCE: CONST->SCR\n";
            }
            delete_stream(_which_wr, sp);
          }
          break;
        }
      }
    }
  }
}

void scratch_write_controller_t::push_remote_wr_req(uint8_t *val, int num_bytes,
                                                    addr_t scr_addr) {
  port_data_t &addr_vp = _accel->port_interf().out_port(NET_ADDR_PORT);
  port_data_t &val_vp = _accel->port_interf().out_port(NET_VAL_PORT);

  // assert(num_bytes % 8 == 0);
  int64_t meta_info =
      num_bytes << 16 | scr_addr; // Unnecessary encoding/decoding
  // addr_vp.push_mem_data(meta_info);
  addr_vp.push_data(meta_info); // this is by-default 8-bytes
  // addr_vp.push_mem_data(addr);
  for (int i = 0; i < num_bytes; i++) {
    // val_vp.push_data_byte(val[i]);
    val_vp.push_data(val_vp.get_byte_vector(val[i],1));
    // cout << "data_byte being pushed to net_in ports: " << val[i] << "\n";
  }
}

void scratch_write_controller_t::finish_cycle() {}

void scratch_write_controller_t::print_status() {
  for (auto &i : _write_streams) {
    if (dynamic_cast<remote_core_net_stream_t*>(i)) continue;
    if (!i->empty()) {
      i->print_status();
    }
  }
}

void port_controller_t::cycle() {
  // Port-Port
  for (unsigned i = 0; i < _port_port_streams.size(); ++i) {
    _which_pp = (_which_pp + 1) == _port_port_streams.size()
                    ? 0
                    : _which_pp + 1; // rolling incr
    auto &pi = _accel->port_interf();

    auto &stream = _port_port_streams[_which_pp];
    if (!stream.stream_active()) {
      continue;
    }

    port_data_t &vp_out = pi.out_port(stream._out_port);

    bool port_in_okay = true;
    for (int in_port : stream.in_ports()) {
      port_data_t &vp_in =
          pi.in_port(in_port); // just wait until 8 items can come
      port_in_okay = port_in_okay && (vp_in.mem_size() < VP_LEN - 8*vp_in.get_port_width()/8);
    }

    //----------------------------
    /*
    if(stream.data_width()%stream.src_data_width()!=0) {
      cout << "dest: " << stream.data_width() << " src: " << stream.src_data_width() << endl;
    }
    assert(stream.data_width()%stream.src_data_width()==0 && "Dest datatype not a multiple of src datatype");
    */
    assert(stream.src_data_width()%stream.data_width()==0 && "Dest datatype not a multiple of src datatype");
    // cout << "recurrence stream data width: " << stream.data_width() << " src data width: " << stream.src_data_width() << " and repeat flag: " << stream.repeat_flag() << endl;

    //-----------------------
    // int n = stream.data_width()/stream.src_data_width();
    int n = 1; // stream.data_width(); // stream.src_data_width()/stream.data_width();
    // TODO: make it a function
    // Let's use repeat flag of a stream here (otherwise repeat at out port is
    // not supported)
    // cout << "Num times repeated till now: " << vp_out.num_times_repeated() << endl;
    if(stream.repeat_flag() && vp_out.num_times_repeated()==0 && vp_out.mem_size()>=n && port_in_okay) {
      auto &repeat_prt = pi.out_port(stream.repeat_in());
      if(repeat_prt.mem_size()) { // assuming non-vector port
        uint64_t x = repeat_prt.pop_out_data();
        // cout << "POPPED FROM REPEAT PORT-1\n";
        if(SS_DEBUG::MEM_REQ) {
          cout << "NEW REPEAT COUNT: " << x << endl;
        }
        int y = (x)*(1<<REPEAT_FXPNT);
        vp_out.set_cur_repeat_lim(y);
      } else { // sstream_size data is not yet available
        vp_out.set_cur_repeat_lim(-1*(1<<REPEAT_FXPNT));
      }
    }

    // cout << "Mem size in vp_out: " << vp_out.mem_size() << " and n: " << n << " port in okay: " << port_in_okay << " cur repeat lim: " << vp_out.cur_repeat_lim() << endl;

    if (vp_out.mem_size() >= n && port_in_okay && vp_out.cur_repeat_lim()>=0) { // okay go for it
      // cout << "Let's try to send data for recurrence stream\n";

      int data_width = stream.data_width(); 
      uint64_t total_pushed=0;
      bool should_push = true;
      for(int i = 0; i < PORT_WIDTH &&
            vp_out.mem_size() >= n && stream.stream_active() && vp_out.cur_repeat_lim()>=0; i+=data_width) {
        SBDT val = 0;

        // TODO: assumes out port has higher or equal data width
        for(int i=0; i<n; ++i) {
          SBDT temp = vp_out.peek_out_data(i);
          val = (val << i*8*stream.src_data_width()) | temp;
          // val = val | (temp << i*8*stream.src_data_width());
        }

        
        // pop n values only if inc repeated is done
        
        // int x1 = vp_out.cur_repeat_lim();
        bool should_pop = vp_out.inc_repeated();
 
        // int x2 = vp_out.cur_repeat_lim();
        // assert(x1==x2);
        // cout << "out-recu: " << stream._out_port << " ";
        // cout << "Value extracted from source port: " << val << " repeat_flag: " << stream.repeat_flag() << " and should push: " << should_push << endl;
        
        if(stream.repeat_flag() && vp_out.cur_repeat_lim()==0) {
          // cout << "Current repeat lim was 0\n";
          should_pop = true;
          vp_out.set_num_times_repeated(0);
          should_push = false;
        } else {
          should_push = true;
        }

        if(should_pop) {
          for(int i=0; i<n; ++i) {
            vp_out.pop_out_data();
          }
          stream._num_elements--;
          if(stream.repeat_flag()) {
            vp_out.set_cur_repeat_lim(-1*(1<<REPEAT_FXPNT)); // need to reset so that it cannot be used again

            if(vp_out.mem_size()>=n && i+data_width<PORT_WIDTH) {
              assert(vp_out.num_times_repeated()==0);
              auto &repeat_prt = pi.out_port(stream.repeat_in());
              if(repeat_prt.mem_size()) { // assuming non-vector port
                uint64_t x = repeat_prt.pop_out_data();
                // cout << "POPPED FROM REPEAT PORT-2\n";
                if(SS_DEBUG::MEM_REQ) {
                  cout << "NEW REPEAT COUNT: " << x << endl;
                }
                int y = (x)*(1<<REPEAT_FXPNT);
                vp_out.set_cur_repeat_lim(y);
              } else { // sstream_size data is not yet available
                vp_out.set_cur_repeat_lim(-1*(1<<REPEAT_FXPNT));
              }
            }
          }
        }

        if (stream._padding_size != NO_PADDING) {
          assert(stream._padding_size);
          (++stream._padding_cnt) %= stream._padding_size;
        }

        if(should_push) {
          for (int in_port : stream.in_ports()) {
            port_data_t &vp_in = pi.in_port(in_port);
            // vp_in could be lower width
            // for(int i=0; i<stream.src_data_width()/stream.data_width(); ++i) {
            for(int i=stream.src_data_width()/stream.data_width()-1; i>=0; --i) {
              SBDT temp = val >> (i*8*stream.data_width());
              // cout << "Allowed to push in recurrence: " << temp << " with repeat_flag: " << stream.repeat_flag() << " in-recu port: " << in_port << endl;
              vp_in.push_data(vp_in.get_byte_vector(temp, data_width));
            }
            if (stream._padding_size != NO_PADDING && stream._padding_cnt == 0) {
              if (stream.fill_mode() == STRIDE_ZERO_FILL ||
                  stream.fill_mode() == STRIDE_DISCARD_FILL) {
                vp_in.fill(stream.fill_mode());
              }
            }
          }

          total_pushed++;
        }
      }

      add_bw(stream.src(), stream.dest(), 1, total_pushed * data_width);

      bool is_empty = stream.check_set_empty();
      if (is_empty) {
        _accel->process_stream_stats(stream);

        if (SS_DEBUG::VP_SCORE2) {
          cout << "SOURCE: PORT->PORT\n";
        }

        for (int in_port : stream.in_ports()) {
          port_data_t &in_vp = pi.in_port(in_port);
          in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                           stream.fill_mode());
        }

        if (SS_DEBUG::VP_SCORE2) {
          cout << "SOURCE: PORT->PORT\n";
        }
        port_data_t &out_vp = _accel->port_interf().out_port(stream._out_port);
        out_vp.set_status(port_data_t::STATUS::FREE);
        if(stream.repeat_flag()) {
          port_data_t &repeat_vp = _accel->port_interf().out_port(stream.repeat_in());
          repeat_vp.set_status(port_data_t::STATUS::FREE);
        }
        // delete_stream(_which_pp, &stream);
      }

      break;
    } 
  }

  // Const-Port
  // Assumption: you can have as many constants as you want
  for (unsigned i = 0; i < _const_port_streams.size(); ++i) {
    _which_cp = (_which_cp + 1) == _const_port_streams.size()
                    ? 0
                    : _which_cp + 1; // rolling incr
    auto &pi = _accel->port_interf();
    auto &stream = _const_port_streams[_which_cp];
    // if (!stream.stream_active()) {
    // Reset didn't make it inactive
    if (stream.empty()) {
      continue;
    }

    int const_width = stream._const_width;

    port_data_t &first_vp_in = pi.in_port(stream.first_in_port());

    if(first_vp_in.mem_size() < VP_LEN*8/const_width) { // enough space, so go for it
      uint64_t total_pushed=0;
      int data_width = stream.data_width();

      for(int i = 0; i < PORT_WIDTH && first_vp_in.mem_size() < VP_LEN*8/const_width
                  && stream.stream_active(); i+=data_width) {

        for(int in_port : stream.in_ports()) {
          port_data_t& vp_in = pi.in_port(in_port);
          // should send vector version of that
          SBDT val = stream.pop_item(); // data width of const val should be >= port width
          // cout << "const_width: " << const_width << " data_width: " << data_width << endl;
          int num_bits = 8*data_width;
          for(int i=0; i<(const_width/data_width); ++i) {
            /*SBDT mask = (1 << num_bits) - 1;
            if(mask==0) { 
              mask = (1 << (num_bits-1)) - 1;
            }*/

            SBDT test_mask = (((uint64_t)1)<<(num_bits-1))-1;
            // cout << "test mask: " << hex << test_mask << endl;
            // SBDT temp = (val >> (i*num_bits)) & mask;
            SBDT temp = (val >> (i*num_bits)) & test_mask;
            vp_in.push_data(vp_in.get_byte_vector(temp, data_width));
          }
          // vp_in.push_data(stream.pop_item());
        }
        total_pushed++;
      }
      add_bw(stream.src(), stream.dest(), 1, total_pushed * data_width);

      /*if(!stream.stream_active()) {
        cout << "const stream inactive\n";
        stream.print_status();
      }*/
      bool is_empty = stream.check_set_empty(); // this set it to empty
      if (is_empty) {
        _accel->process_stream_stats(stream);

        if (SS_DEBUG::VP_SCORE2) {
          cout << "SOURCE: CONST->PORT\n";
        }

        if (SS_DEBUG::COMMAND_O) {
          cout << "Completed: CONST->PORT\n";
          stream.print_status();
        }

        for (int in_port : stream.in_ports()) {
          port_data_t &in_vp = _accel->port_interf().in_port(in_port);
          in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                           stream.fill_mode());
        }
      }
    }
  }

  // Remote-Port
  for (unsigned i = 0; i < _remote_port_streams.size(); ++i) {
    _which_rp =
        (_which_rp + 1) == _remote_port_streams.size() ? 0 : _which_rp + 1;
    auto &pi = _accel->port_interf();
    auto &stream = _remote_port_streams[_which_rp];
    if (!stream.stream_active()) {
      continue;
    }
    // vp out comes from a different core!
    // port_data_t& vp_out = pi.out_port(stream._out_port);

    int acc_index = _accel->accel_index() - stream._which_core;
    if (acc_index == -1) {
      acc_index = (NUM_ACCEL - 1);
    }
    if (acc_index == (NUM_ACCEL)) {
      acc_index = 0;
    }
    accel_t *rem_acc = _accel->get_ssim()->get_acc(acc_index);
    auto &rem_pi = rem_acc->port_interf();
    port_data_t &vp_out = rem_pi.out_port(stream._out_port);

    bool port_in_okay = true;
    for (int in_port : stream.in_ports()) {
      port_data_t &vp_in =
          pi.in_port(in_port); // just wait until 8 items can come
      port_in_okay = port_in_okay && (vp_in.mem_size() < VP_LEN - 8*8/vp_in.get_port_width());
      // port_in_okay = port_in_okay && (vp_in.mem_size() < VP_LEN - 8);
    }

    if (vp_out.mem_size() && port_in_okay) { // okay go for it
      uint64_t total_pushed = 0;
      for (int i = 0;
           i < PORT_WIDTH && vp_out.mem_size() && stream.stream_active(); ++i) {
        SBDT val = vp_out.pop_out_data();
        // timestamp(); cout << "POPPED b/c port -> remote_port WRITE: " <<
        // vp_out.port() << " " << vp_out.mem_size() <<  "\n";

        stream._num_elements--;
        if (stream._padding_size != NO_PADDING)
          (++stream._padding_cnt) %= stream._padding_size;

        for (int in_port : stream.in_ports()) {
          port_data_t &in_vp = _accel->port_interf().in_port(in_port);
          in_vp.push_data(val);
          if (stream._padding_size != NO_PADDING && stream._padding_cnt == 0) {
            if (stream.fill_mode() == STRIDE_ZERO_FILL ||
                stream.fill_mode() == STRIDE_DISCARD_FILL) {
              in_vp.fill(stream.fill_mode());
            }
          }
        }

        total_pushed++;
      }

      add_bw(stream.src(), stream.dest(), 1, total_pushed * DATA_WIDTH);

      bool is_empty = stream.check_set_empty();
      if (is_empty) {
        _accel->process_stream_stats(stream);

        if (SS_DEBUG::VP_SCORE2) {
          cout << "SOURCE: PORT->PORT\n";
        }
        for (int in_port : stream.in_ports()) {
          port_data_t &in_vp = _accel->port_interf().in_port(in_port);
          in_vp.set_status(port_data_t::STATUS::FREE, LOC::NONE,
                           stream.fill_mode());
        }

        if (SS_DEBUG::VP_SCORE2) {
          cout << "SOURCE: PORT->PORT\n";
        }
        vp_out.set_status(port_data_t::STATUS::FREE);
      }

      break;
    }
  }
}

void port_controller_t::finish_cycle() {
  //  for(auto& i : _port_port_streams)  {i.finish_cycle();}  //TODO: maybe
  //  optimize later? for(auto& i : _const_port_streams) {i.finish_cycle();}
}

void port_controller_t::print_status() {
  for (auto &i : _port_port_streams) {
    if (!i.empty()) {
      i.print_status();
    }
  }
  for (auto &i : _const_port_streams) {
    if (!i.empty()) {
      i.print_status();
    }
  }
  for (auto &i : _remote_port_streams) {
    if (!i.empty()) {
      i.print_status();
    }
  }
}

bool accel_t::done(bool show, int mask) {
  bool d = done_internal(show, mask);

  // Is it correct? -- this gives can't free if already free error
  if(d) {
    _cleanup_mode = false;
  }

  if (show)
    return d;

  if (mask == 0 && d) {
    // nothing left to clean up
    _cleanup_mode = false;
  }

  if (SS_DEBUG::CHECK_SCR_ALIAS) {
    if (mask == 0 && d) { // WAIT ALL is true
      for (int i = 0; i < SCRATCH_SIZE; i += 1) {
        scratchpad_readers[i] = 0;
        scratchpad_writers[i] = 0;
      }
    }
  }

  // This is a kind of stream I guess...
  if(_stream_cleanup_mode) {
    d=false;
  }

  if (mask == GLOBAL_WAIT && d) {
    // _cleanup_mode = false; // Should this be here?
    cout << "Came to set this core done: " << _lsq->getCpuId() << endl;
    cout << "Number of active threads: " << _ssim->num_active_threads() << endl;
    _lsq->set_spu_done(_lsq->getCpuId());
    d = _lsq->all_spu_done(_ssim->num_active_threads());
    if(d) {
      // if(SS_DEBUG::WAIT) {
      if(1) {
        cout << "GLOBAL WAIT RELEASED for core id: " << _lsq->getCpuId() << "\n";
        cout << "CORE ID: " << _lsq->getCpuId() << "\n";
      }
      // cout << "Called for global barrier for core: " << _lsq->getCpuId() << endl;
      _lsq->set_spu_global_wait_released(_lsq->getCpuId());
      if(_lsq->is_last_spu(_ssim->num_active_threads())) { // last global wait spu
        // cout << "IT WAS LAST SPU TO RELEASE GLOBAL BARRIER\n";
        _lsq->reset_all_spu();
        _lsq->reset_all_spu_global_wait();
      }
    }
  }

  if (SS_DEBUG::WAIT) {
    timestamp();
    if (d) {
      cout << "Done Check -- Done (" << mask << ")\n";
    } else {
      cout << "Done Check -- NOT DONE: (" << mask << ")\n";
      done_internal(true, mask);
    }
  }

  return d;
}

// checks only stream engines to see if concurrent operations are done
bool accel_t::done_concurrent(bool show, int mask) {
  bool done = true;

  if (!_dma_c.done(show, mask) || !_scr_r_c.done(show, mask) ||
      !_scr_w_c.done(show, mask) || !_port_c.done(show, mask) ||
      !_net_c.done(show, mask)) {
    done = false;
  }

  bool check_cgra_done = true;
  if(mask==STREAM_WAIT) {
    check_cgra_done = false;
  }

  if(check_cgra_done) {
    if (done && !cgra_done(show, mask)) {
      done = false;
    }
  }

  if (SS_DEBUG::CHECK_SCR_ALIAS) {
    if (mask & WAIT_SCR_RD && done) {
      for (int i = 0; i < SCRATCH_SIZE; i += 1) {
        scratchpad_readers[i] = 0;
      }
    }
    if (mask & WAIT_SCR_WR && done) {
      for (int i = 0; i < SCRATCH_SIZE; i += 1) {
        scratchpad_writers[i] = 0;
      }
    }
  }

  return done;
}

// checks everything to see if it's done  (includes queues)
bool accel_t::done_internal(bool show, int mask) {
  if (!done_concurrent(show, mask)) {
    return false;
  }

  // TODO: FIX  -- this can be optimized!
  // if (_ssconfig->dispatch_inorder() || mask==0 || mask&WAIT_CMP) {
  if (_cmd_queue.size()) {
    if (show) {
      cout << "Main Queue Not Empty\n";
    }
    return false;
  }
  //}

  return true;
}

bool network_controller_t::remote_port_multicast_requests_active() {
  return _remote_port_multicast_streams.size();
}
bool network_controller_t::remote_scr_requests_active() {
  return _remote_scr_streams.size();
}
bool network_controller_t::direct_remote_scr_requests_active() {
  return _direct_remote_scr_streams.size();
}

bool dma_controller_t::dma_port_streams_active() {
  return _dma_port_streams.size();
}
bool dma_controller_t::port_dma_streams_active() {
  return _port_dma_streams.size();
}
bool dma_controller_t::indirect_streams_active() {
  return _indirect_streams.size();
}
bool dma_controller_t::indirect_wr_streams_active() {
  return _indirect_wr_streams.size();
}

bool dma_controller_t::done(bool show, int mask) {
  if (mask == 0 || mask & WAIT_CMP || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (dma_port_streams_active()) {
      if (show)
        cout << "DMA -> PORT Streams Not Empty\n";
      return false;
    }
    if (mem_reqs() != 0) {
      if (show)
        cout << "Memory requests are outstanding\n";
      return false;
    }
    if (indirect_streams_active()) {
      if (show)
        cout << "DMA -> Indirect Streams Not Empty\n";
      return false;
    }
  }

  if (mask == 0 || mask & WAIT_CMP || mask & WAIT_MEM_WR || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (port_dma_streams_active()) {
      if (show)
        cout << "PORT -> DMA Streams Not Empty\n";
      return false;
    }
    if (indirect_wr_streams_active()) {
      if (show)
        cout << "Indirect -> DMA Streams Not Empty\n";
      return false;
    }
  }

  if ((mask & WAIT_SCR_WR) && _fake_scratch_reqs) {
    if (show)
      cout << "Scratch reqs: " << _fake_scratch_reqs << " not zero \n";
    return false;
  }

  return true;
}

bool scratch_write_controller_t::port_scr_streams_active() {
  return _port_scr_streams.size();
}

bool scratch_write_controller_t::atomic_scr_streams_active() {
  return _atomic_scr_streams.size();
}

bool scratch_write_controller_t::atomic_scr_issued_requests_active() {
  for (int i = 0; i < NUM_SCRATCH_BANKS; ++i) {
    if (!_atomic_scr_issued_requests[i].empty()) {
      return true;
    }
  }
  return false;
}

bool scratch_read_controller_t::indirect_scr_read_requests_active() {
  for (int i = 0; i < NUM_SCRATCH_BANKS; ++i) {
    if (!_indirect_scr_read_requests[i].empty()) {
      return true;
    }
  }
  return false;
}
bool scratch_write_controller_t::const_scr_streams_active() {
  return _const_scr_streams.size();
}

bool scratch_write_controller_t::done(bool show, int mask) {
  if (mask & WAIT_SCR_WR_DF) { // wait_all is for local streams only
    if (!release_df_barrier()) {
      if (show)
        cout << "PORT -> REMOTE SCR port Not Empty\n";
      return false;
    } else {
      _remote_scr_writes = 0;
      _df_count = -1;
    }
  }
  if (mask == 0 || mask & WAIT_CMP || mask & WAIT_SCR_WR || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (port_scr_streams_active()) {
      if (show)
        cout << "PORT -> SCR Stream Not Empty\n";
      return false;
    }
    if (const_scr_streams_active()) {
      if (show)
        cout << "CONST -> SCR Stream Not Empty\n";
      return false;
    }
  }
  if (mask == 0 || mask & WAIT_CMP || mask & WAIT_SCR_WR ||
      mask & WAIT_SCR_RD || mask & WAIT_SCR_ATOMIC || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (atomic_scr_streams_active()) {
      if (show)
        cout << "ATOMIC SCR Stream Not Empty\n";
      return false;
    }
    if (atomic_scr_issued_requests_active()) {
      if (show)
        cout << "ATOMIC SCR bank buffers Not Empty\n";
      return false;
    }
  }
  return true;
}

bool scratch_read_controller_t::scr_port_streams_active() {
  return _scr_port_streams.size();
}

bool scratch_read_controller_t::done(bool show, int mask) {
  if (mask == 0 || mask & WAIT_CMP || mask & WAIT_SCR_RD || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (scr_port_streams_active()) {
      if (show)
        cout << "SCR -> PORT Streams Not Empty\n";
      return false;
    }
    if (indirect_scr_read_requests_active()) {
      if (show)
        cout << "Banked scratchpad queues Not Empty\n";
      return false;
    }
  }
  return true;
}

bool network_controller_t::done(bool show, int mask) {
  if (mask == 0 || mask & WAIT_CMP || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (remote_port_multicast_requests_active()) {
      if (show)
        cout << "PORT -> REMOTE PORT Stream Not Empty\n";
      return false;
    }
    if (remote_scr_requests_active()) {
      if (show)
        cout << "INDIRECT PORT -> REMOTE SCR Stream Not Empty\n";
      return false;
    }
    if (direct_remote_scr_requests_active()) {
      if (show)
        cout << "DIRECT PORT -> REMOTE SCR Stream Not Empty\n";
      return false;
    }
  }
  return true;
}

bool port_controller_t::port_port_streams_active() {
  for (auto &i : _port_port_streams)
    if (!i.empty())
      return true;
  return false;
}

bool port_controller_t::const_port_streams_active() {
  for (auto &i : _const_port_streams)
    if (!i.empty())
      return true;
  return false;
}

bool port_controller_t::done(bool show, int mask) {
  if (mask == 0 || mask & WAIT_CMP || mask == GLOBAL_WAIT || mask == STREAM_WAIT) {
    if (port_port_streams_active()) {
      if (show)
        cout << "PORT -> PORT Stream Not Empty\n";
      return false;
    }
    if (const_port_streams_active()) {
      if (show)
        cout << "CONST -> PORT Stream  Not Empty\n >>>";
      return false;
    }
  }
  return true;
}

bool accel_t::cgra_done(bool show, int mask) {

  if (mask == 0 || mask & WAIT_CMP || mask == GLOBAL_WAIT) {
    for (unsigned i = 0; i < _soft_config.in_ports_active_plus.size(); ++i) {
      int cur_port = _soft_config.in_ports_active_plus[i];
      auto &in_vp = _port_interf.in_port(cur_port);
      if (in_vp.in_use() || in_vp.num_ready() || in_vp.mem_size()) {
        // if (in_vp.in_use() || in_vp.num_ready() || in_vp.mem_size() ||
        // _dfg->is_busy()) {
        if (show) {
          cout << "In VP: " << cur_port << " Not Empty (";
          cout << "in_use: " << in_vp.in_use()
               << (in_vp.completed() ? "(completed)" : "");
          cout << " num_rdy: " << in_vp.num_ready();
          cout << " mem_size: " << in_vp.mem_size();
          cout << ") \n";
        }

        return false;
      }
    }
  }
  if (mask == 0 || mask == GLOBAL_WAIT) {
    for (unsigned i = 0; i < _soft_config.out_ports_active_plus.size(); ++i) {
      int cur_port = _soft_config.out_ports_active_plus[i];
      auto &out_vp = _port_interf.out_port(cur_port);
      if (out_vp.in_use() || out_vp.num_ready() || out_vp.mem_size() ||
          out_vp.num_in_flight()) { //  || _dfg->is_busy()) {
        if (show) {
          cout << "Out VP: " << cur_port << " Not Empty (";
          cout << "in_use: " << out_vp.in_use();
          cout << " num_rdy: " << out_vp.num_ready();
          cout << " mem_size: " << out_vp.mem_size();
          cout << " in_flight: " << out_vp.num_in_flight();
          cout << ") \n";
        }
        return false;
      }
    }
  }
  return true;
}

// Configure once you get all the bits
void accel_t::configure(addr_t addr, int size, uint64_t *bits) {
  // Slice 0: In Ports Activge
  // Slice 1: Out Ports Active
  // 2,3,4: Reserved for delay  (4 bits each)
  // 5+: switch/fu configuration
  // size - num of 64bit slices

  if (debug && (SS_DEBUG::COMMAND || SS_DEBUG::SCR_BARRIER)) {
    timestamp();
    cout << "SS_CONFIGURE(response): "
         << "0x" << std::hex << addr << " " << std::dec << size << "\n";
    // for(int i = 0; i < size/8; ++i) {
    //  cout << "0x" << std::hex << bits[i] << " ";
    //}
    // cout << "\n";
  }

  assert(_in_config);
  _in_config = false;

  _soft_config.reset(); // resets to no configuration
  _port_interf.reset();

  if (_sched) {
    _sched->clear_ssdfg();
    delete _sched;
  }
  _sched = new Schedule(_ssconfig);
  // assert(_sched);

  _soft_config.inst_histo = _sched->interpretConfigBits(size, bits);

  _dfg = _sched->ssdfg();                 // now we have the dfg!
  _dfg->set_dbg_stream(_cgra_dbg_stream); // change debug stream for dfg

  // Lets print it for debugging purposes
  std::ofstream ofs("viz/dfg-reconstructed.dot", std::ios::out);
  if (!ofs.good()) {
    cerr << "WARNING: viz/ folder not created\n";
  }
  _dfg->printGraphviz(ofs);

  _sched->printGraphviz("viz/sched-reconstructed.gv");

  _soft_config.out_ports_lat.resize(64); // make this bigger if we need

  _soft_config.cgra_in_ports_active.resize(128);

  _soft_config.in_ports_active_group.resize(NUM_GROUPS);
  _soft_config.out_ports_active_group.resize(NUM_GROUPS);
  _soft_config.input_dfg_node.resize(NUM_GROUPS);
  _soft_config.output_dfg_node.resize(NUM_GROUPS);
  _soft_config.group_thr.resize(NUM_GROUPS);

  _soft_config.in_ports_name.resize(64);
  _soft_config.out_ports_name.resize(64);

  for (int ind = 0; ind < _dfg->num_vec_input(); ++ind) {
    SSDfgVec *vec_in = _dfg->vec_in(ind);
    int i = _sched->vecPortOf(vec_in).second;
    _soft_config.in_ports_active.push_back(i); // activate input vector port

    SSDfgVecInput *vec_input = dynamic_cast<SSDfgVecInput *>(vec_in);
    assert(vec_input);

    _soft_config.in_ports_name[i] = vec_input->name();

    int group_ind = _dfg->find_group_for_vec(vec_input);
    _soft_config.in_ports_active_group[group_ind].push_back(i);


    // port mapping of 1 vector port - cgra_port_num: vector offset elements
    port_data_t &cur_in_port = _port_interf.in_port(i);
    cur_in_port.set_dfg_vec(vec_in);

    cur_in_port.set_port_width(vec_in->get_port_width());
 

    // find corresponding dfg nodes
    std::vector<SSDfgInput *> dfg_inputs;

    // for each mapped cgra port

    for (unsigned port_idx = 0; port_idx < cur_in_port.port_cgra_elem();
         ++port_idx) {
      int cgra_port_num = cur_in_port.cgra_port_for_index(port_idx);
      // SSDfgNode *dfg_node = vec_input->inputs()[port_idx];
      SSDfgNode *dfg_node = vec_input->inputs()[port_idx*vec_input->get_port_width()/64];

      if (dfg_node != nullptr) {
        SSDfgInput *dfg_in = static_cast<SSDfgInput *>(dfg_node);
        dfg_inputs.push_back(dfg_in); // get all the dfg inputs for ports
        if (cgra_port_num >= _soft_config.cgra_in_ports_active.size())
          _soft_config.cgra_in_ports_active.resize(cgra_port_num + 1);
        _soft_config.cgra_in_ports_active[cgra_port_num] = true;
      } else {
        // TODO: is there something I SHOULD DO INCASE input DOESN'T EXIST?
      }
    }
    _soft_config.input_dfg_node[group_ind].push_back(dfg_inputs);
  }

  for (int ind = 0; ind < _dfg->num_vec_output(); ++ind) {
    SSDfgVec *vec_out = _dfg->vec_out(ind);
    int i = _sched->vecPortOf(vec_out).second;

    _soft_config.out_ports_active.push_back(i);

    SSDfgVecOutput *vec_output = dynamic_cast<SSDfgVecOutput *>(vec_out);
    assert(vec_output);

    _soft_config.out_ports_name[i] = vec_output->name();

    int group_ind = _dfg->find_group_for_vec(vec_output);
    _soft_config.out_ports_active_group[group_ind].push_back(i);

    vector<bool> mask = _sched->maskOf(vec_out);

    // port mapping of 1 vector port - cgra_port_num: vector offset elements
    auto &cur_out_port = _port_interf.out_port(i);
    cur_out_port.set_port_width(vec_out->get_port_width());
    cur_out_port.set_dfg_vec(vec_out);

    // find corresponding dfg nodes
    std::vector<SSDfgOutput *> dfg_outputs;

    int max_lat = 0;
    for (unsigned port_idx = 0; port_idx < cur_out_port.port_cgra_elem();
         ++port_idx) {
      // SSDfgNode *dfg_node = vec_output->getOutput(port_idx);
      SSDfgNode *dfg_node = vec_output->getOutput(port_idx*vec_output->get_port_width()/64);
      assert(dfg_node);
      SSDfgOutput *dfg_out = static_cast<SSDfgOutput *>(dfg_node);
      max_lat = std::max(_sched->latOf(dfg_out), max_lat);
      dfg_outputs.push_back(dfg_out);
    }
    _soft_config.out_ports_lat[i] = max_lat;
    _soft_config.output_dfg_node[group_ind].push_back(dfg_outputs);
  }

  int lat, lat_mis;
  _sched->cheapCalcLatency(lat, lat_mis);
  int max_lat_mis = lat_mis; //_sched->decode_lat_mis();

  for (int g = 0; g < NUM_GROUPS; ++g) {
    auto &active_ports = _soft_config.in_ports_active_group[g];
    auto &active_out_ports = _soft_config.out_ports_active_group[g];

    if (active_ports.size() > 0) {

      int thr = _dfg->maxGroupThroughput(g);

      float thr_ratio = 1 / (float)thr;
      float mis_ratio = ((float)_fu_fifo_len) / (_fu_fifo_len + max_lat_mis);

      // setup the rate limiting structures
      if (thr_ratio < mis_ratio) { // group throughput is worse
        _soft_config.group_thr[g] = make_pair(1, thr);
        _cgra_prev_issued_group[g].resize(max(thr - 1, 0), false);
      } else { // group latency is worse
        _soft_config.group_thr[g] =
            make_pair(_fu_fifo_len, _fu_fifo_len + max_lat_mis);
        _cgra_prev_issued_group[g].resize(
            max(_fu_fifo_len + max_lat_mis - 1, 0), false);
        if (max_lat_mis == 0) {
          _cgra_prev_issued_group[g].resize(0, false);
        }
      }

      if (SS_DEBUG::SHOW_CONFIG) {
        for (int i = 0; i < active_ports.size(); ++i) {
          int p = active_ports[i];
          cout << "in vp" << p << " ";
        }
        cout << "\n";
        for (int i = 0; i < active_out_ports.size(); ++i) {
          int p = active_out_ports[i];
          cout << "out vp" << p
               << " has latency:" << _soft_config.out_ports_lat[p] << "\n";
        }
      }
    }
  }

  // compute active_plus
  _soft_config.in_ports_active_plus = _soft_config.in_ports_active;
  _soft_config.out_ports_active_plus = _soft_config.out_ports_active;
  // indirect
  for (unsigned i = START_IND_PORTS; i < STOP_IND_PORTS; ++i) {
    _soft_config.in_ports_active_plus.push_back(i);
    _soft_config.out_ports_active_plus.push_back(i);
  }

  // compute the in ports active for backcgra
  if (_back_cgra) {
    _soft_config.in_ports_active_backcgra = _soft_config.in_ports_active;
  } else {
    // Iterate through the groups and input vectors from temporal groups
    // into the overall group of inputs to check for backcgra
    for (int g = 0; g < NUM_GROUPS; ++g) {
      if (_dfg->group_prop(g).is_temporal) {
        auto &active_ports = _soft_config.in_ports_active_group[g];
        for (int i = 0; i < active_ports.size(); ++i) {
          _soft_config.in_ports_active_backcgra.push_back(active_ports[i]);
        }
      }
    }
  }

  ofs.close();
}
