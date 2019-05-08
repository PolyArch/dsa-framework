#include "accel.hh"
#include "stream.hh"
#include "sim-debug.hh"

int base_stream_t::ID_SOURCE=0;

void base_stream_t::set_empty(bool b) {
  assert(b && "only goes one way for now");
  assert(!_empty && "can only empty once");

  if(SS_DEBUG::COMMAND_O) {
    if(b == true) {
      // timestamp();
      std::cout << "COMPLETED: "; 
    } 
    print_status();
  }

  _empty=b;
}

void base_stream_t::print_in_ports() {
  for(int i = 0; i < _in_ports.size();++i) {
    std::cout << _in_ports[i] << " ";
  }
  if(_soft_config) {
    std::cout << "(";
    for(int i = 0; i < _in_ports.size();++i) {
      std::cout << _soft_config->in_ports_name[_in_ports[i]] << " ";
    }
    if(_in_ports.size()) std::cout <<'\b';
    std::cout << ")";
  }
}

