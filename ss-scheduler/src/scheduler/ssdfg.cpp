#include "model_parsing.h"
#include "ssdfg.h"
#include <vector>
#include <set>
#include <iomanip>
#include <string>
#include <list>
#include "schedule.h"
#include "dfg-parser.tab.h"

using namespace std;
using namespace SS_CONFIG;

CtrlMap CtrlBits::ctrl_map;

bool SSDfgVecInput::backPressureOn() {
    // return getBackBit();
    return false;
}

void checked_system(const char* command) {
  int ret = system(command);
  if(ret) {
    std::cout << "Command: \"" << command 
              << "\" failed with return value: " << ret << "\n";
  }
}


SSDfgEdge::SSDfgEdge(SSDfgNode* def, SSDfgNode* use, EdgeType etype, SSDfg* ssdfg, int l, int r) :
   _ID(ssdfg->inc_edge_id()), _ssdfg(ssdfg), _def(def), _use(use), _etype(etype),  _l(l), _r(r) {
}

bool SSDfgNode::is_temporal() {
  return _ssdfg->group_prop(_group_id).is_temporal;
}

/*
void SSDfgEdge::compute_next(){
  if(_data_buffer.size()==1){
        _use->inc_inputs_ready_backcgra(false, false);
  }
}*/

void SSDfgEdge::compute_after_push(bool print, bool verif){
  // std::cout << this->name() << " is checking compute after push, buf size: " << _data_buffer.size() << "\n";
  if(_data_buffer.size()==1){
    _use->inc_inputs_ready_backcgra(print, verif);
  }
}
void SSDfgEdge::compute_after_pop(bool print, bool verif){
  // std::cout << this->name() << " is checking compute after pop, buf size: " << _data_buffer.size() << "\n";

  if(_data_buffer.size()>0){
    _use->inc_inputs_ready_backcgra(print, verif);
  }
}

//Calculate the value based on the origin's value, and the edge's
//index and bitwidth fields
uint64_t SSDfgEdge::extract_value(uint64_t val) {
  if (_r - _l + 1 == 64) { //this is special cased because << 64 is weird in c
    return val;
  } else {
    // std::cout << "value to extract: " << val << " " << _l << " " << _r << "\n";
    uint64_t mask = (((uint64_t) 1 << bitwidth()) - 1);
    return (val >> _l) & mask; // little endian machine
    // return (val >> (63-_r)) & mask;
  }
}

uint64_t SSDfgEdge::get_value() {
  assert(_def);
  return extract_value(_def->get_value()); //the whole value
}

bool SSDfgOperand::valid() {
    for(SSDfgEdge* e : edges) {
      SSDfgNode* n = e->def();
      if(n->invalid()) return false;
    }
    return true;
}

void SSDfgNode::push_buf_dummy_node(){
    _ssdfg->push_buf_transient(this->first_inc_edge(), true, 1); // can it be immediate?
}


void SSDfg::order_insts(SSDfgInst* inst,
                 std::set<SSDfgInst*>& done_nodes,         //done insts
                 std::vector<SSDfgInst*>& ordered_insts) {

  if(done_nodes.count(inst)) {
    return;
  }

  //insert the new inst
  done_nodes.insert(inst);

  //incoming edges to a node
  int index=0;
  for(auto edge : inst->in_edges()) {
    if(!edge) {
      assert(inst->immSlot()==index);
      continue;
    }

    //if there is a defintion node
    if(SSDfgInst* op_inst = dynamic_cast<SSDfgInst*>(edge->def()) ) {
      order_insts(op_inst, done_nodes, ordered_insts);
      //recursive call until the top inst with the last incoming edge
    }
    ++index;
  }

  ordered_insts.push_back(inst);
}

void SSDfg::check_for_errors() {
  printGraphviz("viz/error_check_dfg.dot");

  bool error = false;
  for (auto elem : _inputs) {
    if (elem->num_out() == 0) {
      cerr << "Error: No uses on input " << elem->name() << "\n";
      error = true;
    }
  }

  for (auto elem : _insts) {
    if (elem->num_out() == 0) {
      cerr << "Error: No uses on inst " << elem->name() << "\n";
      error = true;
    }
    if (elem->num_inc() == 0) {
      cerr << "Error: No operands on inst " << elem->name() << "\n";
      error = true;
    }
  }

  for (auto elem : _outputs) {
    if (elem->num_inc() == 0) {
      cerr << "Error: No operands on output " << elem->name() << "\n";
      error = true;
    }
  }

  assert(!error && "ERROR: BAD DFG");
}

// This function is called from the simulator to
int SSDfg::compute(bool print, bool verif, int g) {
  //if(_orderedInstsGroup.size()<=(unsigned)g) {
  //  _orderedInstsGroup.resize(g+1);
  //}

  //if(_orderedInsts.size()==0) {
  //  std::set<SSDfgInst*> done_nodes;

  //  //for each output node traverse
  //  //the incoming node
  //  for(SSDfgOutput* out : _outputs) {
  //    if(SSDfgInst* producing_node = out->out_inst()) {
  //      order_insts(producing_node, done_nodes, _orderedInsts);
  //    }
  //  }
  //}

  //for(SSDfgInst* inst : _orderedInsts) {
  //  inst->compute(print,verif);
  //}

  //printf("came inside compute function at line:74\n");


  int num_computed=0;

  assert(g < (int)_vecInputGroups.size());
  for(unsigned i = 0; i < _vecInputGroups[g].size(); ++i) {
    SSDfgVecInput* vec = _vecInputGroups[g][i];
    for (auto elem : vec->inputs()) {
      num_computed += elem->compute(print,verif); //calling some other compute
    }
  }
  return num_computed;
}

//Calculates max group throughput based on functional unit type
int SSDfg::maxGroupThroughput(int g) {
  int maxgt=0;

  assert(g < (int)_vecInputGroups.size());
  for(unsigned i = 0; i < _vecInputGroups[g].size(); ++i) {
    SSDfgVecInput* vec = _vecInputGroups[g][i];
    for (auto elem : vec->inputs())
      maxgt=std::max(maxgt, elem->maxThroughput());
  }
  return maxgt;
}

void SSDfg::instsForGroup(int g, std::vector<SSDfgInst*>& insts) {
  assert(g < (int)_vecInputGroups.size());
  for(unsigned i = 0; i < _vecInputGroups[g].size(); ++i) {
    SSDfgVecInput* vec = _vecInputGroups[g][i];
    for (auto elem : vec->inputs())
      elem->depInsts(insts);
  }
}

//Necessary for BOOST::SERIALIZATION
SSDfg::SSDfg() {

}


//COMMA IF NOT FIRST
void CINF(std::ostream& os, bool& first) {
  if (first) {
    first = false;
  } else {
    os << ", ";
  }
}

bool conv_to_int(std::string s, uint64_t& ival) {
  try {
    ival = (uint64_t) stol(s, 0, 0);
    return true;
  } catch (...) {}
  return false;
}

bool conv_to_double(std::string s, double& dval) {
  try {
    dval = stod(s);
    return true;
  } catch (...) {}
  return false;
}

SymEntry SSDfg::create_inst(std::string opcode, std::vector<SymEntry> &args) {

  SS_CONFIG::ss_inst_t inst = inst_from_string(opcode.c_str());
  auto *dfg_inst = new SSDfgInst(this, inst);

  int imm_offset = 0;
  for (unsigned i = 0; i < args.size(); ++i) {
    SymEntry &sym = args[i];
    if (sym.type == SymEntry::SYM_INT || sym.type == SymEntry::SYM_DUB) {
      assert(imm_offset == 0 && "only one immediate per instr. is allowed");
      dfg_inst->setImm(sym.data.i);
      dfg_inst->setImmSlot(i);
      imm_offset = 1;
      assert(sym.flag == SymEntry::FLAG_NONE);
    } else if (sym.type == SymEntry::SYM_NODE) {
      int num_entries = (int) sym.edge_entries->size();
      assert(num_entries > 0 && num_entries <= 16);
      for (int e = 0; e < num_entries; ++e) {
        auto &edge_entry = sym.edge_entries->at(e); // l,r set here
        SSDfgNode *inc_node = edge_entry.node;
        if (inc_node == nullptr) {
          cerr << "Could not find argument " << i << " for \""
               << opcode + "\" inst \n";
          assert(false);
        }
        SSDfgEdge::EdgeType etype = SSDfgEdge::EdgeType::data;

        switch (sym.flag) {
          case SymEntry::FLAG_CONTROL:
            etype = SSDfgEdge::EdgeType::ctrl;
            break;
          case SymEntry::FLAG_PRED:
            etype = SSDfgEdge::EdgeType::ctrl_true;
            break;
          case SymEntry::FLAG_INV_PRED:
            etype = SSDfgEdge::EdgeType::ctrl_false;
            break;
          default:
            etype = SSDfgEdge::data;
        }

        // flag should not work if it is control
        connect(inc_node, dfg_inst, i, etype, edge_entry.l, edge_entry.r);
        dfg_inst->set_ctrl_bits(sym.ctrl_bits);
      }
    } else {
      assert(false && "Invalide Node type");
    }
  }


  addInst(dfg_inst);
  SymEntry e(dfg_inst);
  // e.set_bitslice_params(64-SS_CONFIG::bitwidth[inst],63);
  // little endian machine: this is output i guess
  // std::cout << "SETS BITWIDTH HERE\n";
  e.set_bitslice_params(0, SS_CONFIG::bitwidth[inst] - 1);
  return e;
}

void SSDfg::set_pragma(std::string& c, std::string& s) {
  if (c == string("dfg")) {
    cout << "No pragmas yet for dfg\n";
  } else if (c == string("group")) {
    if (s == "temporal") {
      assert(!_groupProps.empty());
      _groupProps[_groupProps.size() - 1].is_temporal = true;
    }
  } else {
    cout << "Context \"" << c << "\" not recognized.";
  }
}

void SSDfg::start_new_dfg_group() {
  _vecInputGroups.emplace_back(std::vector<SSDfgVecInput*>());
  _vecOutputGroups.emplace_back(std::vector<SSDfgVecOutput*>());
  _groupProps.emplace_back(GroupProp());
}

SSDfg::SSDfg(string filename) : SSDfg() {
  string line;
  start_new_dfg_group();
  parse_dfg(filename.c_str(),this);
  calc_minLats();
  check_for_errors();
}

std::string SSDfgEdge::name() {
  std::stringstream ss;
  ss << _def->name() << "->" << _use->name();
  return ss.str();
}

// -- Gams names --
std::string SSDfgEdge::gamsName() {
  std::stringstream ss;
  ss << _def->gamsName() << "_" << _use->gamsName() << "i" << _ID ;
  return ss.str();
}

std::string SSDfgInput::gamsName() {
  std::stringstream ss;
  ss << "IV" << _ID;
  return ss.str();
}

std::string SSDfgOutput::gamsName() {
  std::stringstream ss;
  ss << "OV" << _ID;
  return ss.str();
}

std::string SSDfgInst::gamsName() {
  std::stringstream ss;
  ss << "FV" << _ID;
  return ss.str();
}
void SSDfgInst::setImmSlot(int i) {
  assert(i < 4);

  if ((int) _ops.size() <= i) {
    _ops.resize(i + 1);
  }

  _imm_slot = i;
}

uint64_t SSDfgInst::do_compute(uint64_t &discard) {
  //This is a really cheezy way to do this, but i'm a little tired
  uint64_t output;
  switch(bitwidth()) {
    case 64:
     output=SS_CONFIG::execute64(_ssinst,_input_vals,_reg,discard,_back_array);
      break;
    case 32:
      _input_vals_32.resize(_input_vals.size());
      for(int i = 0; i < (int)_input_vals.size(); ++i) {
        _input_vals_32[i] = _input_vals[i];
      }
      output=SS_CONFIG::execute32(_ssinst,_input_vals_32,_reg_32,discard,_back_array);
      break;
    case 16:
      _input_vals_16.resize(_input_vals.size());
      for(int i = 0; i < (int)_input_vals.size(); ++i) {
        _input_vals_16[i] = _input_vals[i];
      }
      output=SS_CONFIG::execute16(_ssinst,_input_vals_16,_reg_16,discard,_back_array);
      break;
    case 8:
      _input_vals_8.resize(_input_vals.size());
      for(int i = 0; i < (int)_input_vals.size(); ++i) {
        _input_vals_8[i] = _input_vals[i];
      }
      output=SS_CONFIG::execute8(_ssinst,_input_vals_8,_reg_8,discard,_back_array);
      break;
    default:
      cout << "Weird bitwidth: " << bitwidth() << "\n";
      assert(0 && "weird bitwidth");
  }
  return output;
}

//compute:actual compute called from SSDfg class (slightly modify this)
int SSDfgInst::compute(bool print, bool verif) {
  assert(_ops.size() <=3);

  if(_input_vals.size()==0) {
    _input_vals.resize(_ops.size());
  }
  assert(_input_vals.size() <= _ops.size());

  if(print) {
    _ssdfg->dbg_stream() << name() << " (" << _ID << "): ";
  }

  _invalid=false;

  for(unsigned i = 0; i < _ops.size(); ++i) {
    if(immSlot() == (int)i) {
      _input_vals[i]=imm();
    } else {
      _input_vals[i] = _ops[i].get_value();
      if(!_ops[i].valid()) {
        _invalid=true;
      }
    }
    if(print) {
      _ssdfg->dbg_stream() << std::hex << _input_vals[i] << " ";
    }
  }

  _val = do_compute(_invalid);

  if(print) {
    _ssdfg->dbg_stream() << " = " << _val << "\n";
  }

  if(verif) {
    if (_verif_stream.is_open()) {
      _verif_stream << hex << setw(16) << setfill('0') << _val << "\n";
      _verif_stream.flush();
    } else {
      _verif_stream.open(("verif/fu" + _verif_id + ".txt").c_str(), ofstream::trunc | ofstream::out);
      assert(_verif_stream.is_open());
    }
  }

  int num_computed=!_invalid;

  for(auto iter = _uses.begin(); iter != _uses.end(); iter++) {
      SSDfgNode* use = (*iter)->use();
      num_computed += use->inc_inputs_ready(print, verif); //recursively call compute
  }

  return num_computed;

}

// new compute for back cgra-----------------------------
int SSDfgInst::compute_backcgra(bool print, bool verif) {

  assert(_ops.size() <=3);

  if(_input_vals.size()==0) {
    _input_vals.resize(_ops.size());
  }

  uint64_t pred = 1; // valid by default

  // initializing back pressure
  if(_back_array.size() == 0){
     _back_array.resize(_ops.size());
  }

  for(unsigned int i=0; i<_ops.size(); ++i) {
    _back_array[i] = 0;
  }
  /*
  if(!predInv()) {
    _input_vals.resize(_ops.size()-1);
      // slot 2 in this instruction is predicate
  }
  */
  if(print) {
    _ssdfg->dbg_stream() << name() << " (" << _ID << "): ";
  }

  uint64_t discard=0;
  uint64_t output = 0;
  bool reset = false;

  _invalid=false;

  for(unsigned i = 0; i < _ops.size(); ++i) {
    if(_ops[i].is_imm()) {
      _input_vals[i]=imm();
    } else if (_ops[i].is_ctrl()) {
      int c_val = _ops[i].get_buffer_val();
      _back_array[0] = _ctrl_bits.isSet(c_val,CtrlMap::BACKP1); // 2*5+0 = 10th pos in bitmap?
      _back_array[1] = _ctrl_bits.isSet(c_val,CtrlMap::BACKP2);
      discard = _ctrl_bits.isSet(c_val,CtrlMap::DISCARD);
      pred = !(_ctrl_bits.isSet(c_val,CtrlMap::ABSTAIN)); // because it is abstain
      // FIXME: confirm that this is correct
      _input_vals[i] = c_val;
      reset = _ctrl_bits.isSet(c_val,CtrlMap::RESET);
      //TODO: use reset somehow
    } else {
      _input_vals[i] = _ops[i].get_buffer_val();
      if(!_ops[i].get_buffer_valid()) {
        _invalid=true;
      }
    }

    if(print) {
      _ssdfg->dbg_stream() << std::hex << _input_vals[i] << " ";
    }
  }

  // we set this instruction to invalid
  // pred = 1; // for now--check why is it here?
  if(pred==0) {
    _invalid=true;
  }

  if(!_invalid) { //IF VALID
     _ssdfg->inc_total_dyn_insts();

    // Read in some temp value and set _val after inst_lat cycles
    output=do_compute(discard);

    if(print) {
      _ssdfg->dbg_stream() << " = " << output << "\n";
    }
  }
  //TODO/FIXME: change to all registers
  if(reset) {
     _reg[0] = 0;
     _reg_32[0] = 0;
     _reg_16[0] = 0;
    _reg_8[0] = 0;
  }

  if(print) {
     std::cout << (_back_array[0] ? "backpressure on 1st input\n" : "");
     std::cout << (_back_array[1] ? "backpressure on 2nd input\n" : "");
  }
  if(this->name() == ":Phi") {
    _ssdfg->inc_total_dyn_insts();
    assert(_input_vals.size()==3 && "Not enough input in phi node");
    for(unsigned i = 0; i < _ops.size(); ++i) {
      if(_ops[i].get_buffer_valid()) {
         output = _ops[i].get_buffer_val();
      }
    }

    if(print) {
      _ssdfg->dbg_stream() << " = " << output << "\n";
    }
    discard=false;
    _invalid=false;
  }

  // std::cout << "final values of discard: " << discard << " pred: " << pred << "\n";
  _inputs_ready=0;

  if(print) {
    std::cout << (_invalid ? "instruction invalid\n" : "instruction valid\n");
    if(discard) {
      cout << " and output discard!\n";
    }
    // cout << "\n";
  }

  if(!discard) {
    this->set_value(output, !_invalid, true, inst_lat(inst()));
  }
  // _invalid = true;


  // pop the inputs after inst_thr+back_press here
  int inst_throughput = inst_thr(inst());


  for(unsigned i = 0; i < _ops.size(); ++i) {
    if(_ops[i].is_imm()) {
      continue;
    }
    if(_back_array[i]==1){
      _inputs_ready += _ops[i].edges.size();
    }
    else{
      //TODO:FIXME:CHECK:IMPORTANT
      //Iterate over edges in an operand and push transients for all of them?
      for(SSDfgEdge* e : _ops[i].edges) {
        _ssdfg->push_buf_transient(e, false, inst_throughput);
      }
    }
  }


  if(verif) {
    if (_verif_stream.is_open()) {
      _verif_stream << hex << setw(16) << setfill('0') << _val << "\n";
      _verif_stream.flush();
    } else {
      checked_system("mkdir -p verif");
      _verif_stream.open(("verif/fu" + _verif_id + ".txt").c_str(), ofstream::trunc | ofstream::out);
      assert(_verif_stream.is_open());
    }
  }

  // int num_computed=!discard;
  // return num_computed;

  return 1;
}

// Virtual function-------------------------------------------------

int SSDfgInst::update_next_nodes(bool print, bool verif){
    return 0;
}

bool SSDfgVec::is_temporal() {
  return _ssdfg->group_prop(_group_id).is_temporal;
}

SSDfgIO::SSDfgIO(SSDfg *ssdfg, const std::string &name, SSDfgVec *vec_, SSDfgNode::V_TYPE v) : SSDfgNode(ssdfg, v, name), vec_(vec_) {

}

SSDfgVec::SSDfgVec(int len, const std::string &name, int id, SSDfg* ssdfg) : _name(name), _ID(id), _ssdfg(ssdfg) {
  _group_id = ssdfg->num_groups() - 1;
}

SSDfgNode::SSDfgNode(SSDfg* ssdfg, V_TYPE v) :
      _ssdfg(ssdfg), _ID(ssdfg->inc_node_id()),  _vtype(v) {
  //TODO:FIXME: This will likely be incorrect while rebuilding the
  //DFG from a config file, but might work for now
  _group_id = _ssdfg->num_groups() - 1;

}

SSDfgNode::SSDfgNode(SSDfg* ssdfg, V_TYPE v, const std::string &name) :
        _ssdfg(ssdfg), _ID(ssdfg->inc_node_id()),  _name(name), _vtype(v)  {
  //TODO:FIXME: This will likely be incorrect while rebuilding the
  //DFG from a config file, but might work for now
  _group_id = _ssdfg->num_groups() - 1;

}

int SSDfgNode::inc_inputs_ready_backcgra(bool print, bool verif) {
  _inputs_ready+=1;
   // std::cout<<"Node: " << this->name() << " Came to inc the inputs avail are: "<<_inputs_ready << " and required: "<<num_inc_edges()<<"\n";
  if(_inputs_ready == num_inc_edges()) {
    // std::cout << "(and its being pushed to ready)\n";
    _ssdfg->push_ready_node(this);
  }
  return 0;
}

SSDfgVecOutput *SSDfgOutput::output_vec() {
  auto res = dynamic_cast<SSDfgVecOutput*>(vec_);
  assert(res);
  return res;
}

SSDfgVecInput *SSDfgInput::input_vec() {
  auto res = dynamic_cast<SSDfgVecInput*>(vec_);
  assert(res);
  return res;
}


void SSDfgNode::set_value(uint64_t v, bool valid, bool avail, int cycle) {
    _ssdfg->push_transient(this, v,valid, avail, cycle);
}

//------------------------------------------------------------------


void SSDfgNode::printGraphviz(ostream& os, Schedule* sched) {

  string ncolor = "black";
  os << "N" << _ID << " [ label = \"" << name();

  if(sched) {
    os << "\\n lat=" << sched->latOf(this)  << " ";
  }
  os << "min:" << _min_lat;

  if(sched) {
    auto p = sched->lat_bounds(this);
    os << "\\n bounds=" << p.first << " to " << p.second;
    os << "\\n vio=" << sched->vioOf(this);
  }

  os  << "\", color= \"" << ncolor << "\"]; ";

  os << "\n";

  //print edges
  SSDfgNode::const_edge_iterator I,E;
  for (auto e : _uses) {

    if(e->etype()==SSDfgEdge::data) {
       ncolor="black";
    } else if(e->etype()==SSDfgEdge::ctrl_true) {
       ncolor="blue";
    } else if(e->etype()==SSDfgEdge::ctrl_false) {
       ncolor="red";
    }

    SSDfgNode* n = e->use();
    os << "N" << _ID << " -> N" << n->_ID << "[ color=";
    os << ncolor;
    os << " label = \"";
    if(sched) {
      os << "l:" << sched->link_count(e)
         << "\\nex:" << sched->edge_delay(e)
         << "\\npt:" << sched->num_passthroughs(e);
    }
    os << e->l() << ":" << e->r();
    os << "\"];\n";
  }

  os << "\n";

}

void SSDfgInst::printGraphviz(ostream& os, Schedule* sched) {
  SSDfgNode::printGraphviz(os,sched);
}

void SSDfgOutput::printGraphviz(ostream& os,Schedule* sched) {
  SSDfgNode::printGraphviz(os,sched);
}

void SSDfgInput::printGraphviz(ostream& os,Schedule* sched) {
  SSDfgNode::printGraphviz(os,sched);
}

//TODO:FIXME: This doesn't work any more
// ---------------------------------------------------------------------------
// DFG Emulation -- not currently used ---------------------------------------
#if 0
void SSDfgNode::printEmuDFG(ostream& os, string dfg_name) {
  os << "The ID for this node is " << _ID << endl;
  os << "The name for this node is " << _name << endl;
  for(auto iter = _ops.begin(); iter != _ops.end(); iter++) {
  //  os << "Name for the ops " << (*iter)->name() << endl;
  }
  for(auto iter = _uses.begin(); iter != _uses.end(); iter++) {
    os << "Name for the uses " << (*iter)->name() << endl;
  }
  os << "The gams name is " << gamsName() << endl;
  os << "\n";
}

int SSDfgNode::findDepth(ostream& os, string dfg_name, int level) {
  int returned_level = level;
  for(auto name_iter = _uses.begin(); name_iter != _uses.end(); name_iter++) {
    if((*name_iter)->use()->output) {
      //Don't do anything
    } else {
      int candidate_level = (*name_iter)->use()->findDepth(os, dfg_name, level+1);
      if(candidate_level > returned_level) {
    returned_level = candidate_level;
      }
    }
  }
  return returned_level;
}

void SSDfgInst::printEmuDFG(ostream& os, string dfg_name) {
  //os << "INSTRUCTION " << dfg_name << "_" << _name << endl;
  auto name_iter = _uses.begin();
  if((*name_iter)->use()->output) {
    os << "   outputs[" << (*name_iter)->use()->_iter << "]";
    string outputArray = (*name_iter)->use()->name();
    if(!(*name_iter)->use()->getScalar()) {
      outputArray = outputArray.substr(0, outputArray.find_first_of(":"));
      if(outputArray.find_first_of("0123456789") < outputArray.length()) {
    outputArray = outputArray.substr(outputArray.find_first_of("0123456789"), outputArray.length());
    //this absolutely, should NOT have an _
    if(outputArray.find_first_of("_") < outputArray.length()) {
      outputArray = outputArray.substr(0, outputArray.find_first_of("_"));
    }
    //Get subIter
    os << "[" << outputArray << "]  = ";
      } else {
    os << "[0] = ";
      }
    } else {
      os << "[0] = ";
    }
  } else {
    string outputArray = (*name_iter)->def()->name();
    outputArray = outputArray.substr(0, outputArray.find_first_of(":"));
    os << "   uint64_t " << outputArray << " = ";
  }
  string instName = (*name_iter)->def()->name();
  instName = instName.substr(instName.find_first_of(":")+1, instName.length());
  os << instName << "(std::array<uint64_t,2>{";
  uint ops_amt = 1;
  for(auto ops_iter = _ops.begin(); ops_iter != _ops.end(); ops_iter++) {
    if((*ops_iter)->def()->input) {
      os << "inputs[" << (*ops_iter)->def()->_iter << "]";
      string inputArray = (*ops_iter)->def()->name();
      if(!(*ops_iter)->def()->getScalar()) {
    inputArray = inputArray.substr(0, inputArray.find_first_of(":"));
    if(inputArray.find_first_of("0123456789") < inputArray.length()) {
      inputArray = inputArray.substr(inputArray.find_first_of("0123456789"), inputArray.length());
      //Get subIter
      os << "[" << inputArray << "]";
    } else {
      os << "[0]";
    }
      } else {
    os << "[0]";
      }
    } else {
      string inputArray = (*ops_iter)->def()->name();
      inputArray = inputArray.substr(0, inputArray.find_first_of(":"));
      os << inputArray;
    }
    if(ops_amt != _ops.size()) {
      os << ",";
    } else {
      os << "});" << endl;
    }
    ops_amt++;
  }
  //SSDfgNode::printEmuDFG(os, dfg_name);
}

void SSDfgOutput::printDirectAssignments(ostream& os, string dfg_name) {
  for(auto ops_iter = _ops.begin(); ops_iter != _ops.end(); ops_iter++) {
    if((*ops_iter)->def()->input) {
      //Print our formatted name and such
      os << "   outputs[" << _iter << "]";
      string outputArray = name();
      if(!getScalar()) {
    outputArray = outputArray.substr(0, outputArray.find_first_of(":"));
    if(outputArray.find_first_of("0123456789") < outputArray.length()) {
      outputArray = outputArray.substr(outputArray.find_first_of("0123456789"), outputArray.length());
      //this absolutely, should NOT have an _
      if(outputArray.find_first_of("_") < outputArray.length()) {
        outputArray = outputArray.substr(0, outputArray.find_first_of("_"));
      }
      //Get subIter
      os << "[" << outputArray << "]  = ";
    } else {
      os << "[0] = ";
    }
      } else {
    os << "[0] = ";
      }
      os << "inputs[" << (*ops_iter)->def()->_iter << "]";
      string inputArray = (*ops_iter)->def()->name();
      if(!(*ops_iter)->def()->getScalar()) {
    inputArray = inputArray.substr(0, inputArray.find_first_of(":"));
    if(inputArray.find_first_of("0123456789") < inputArray.length()) {
      inputArray = inputArray.substr(inputArray.find_first_of("0123456789"), inputArray.length());
      //Get subIter
      os << "[" << inputArray << "]";
    } else {
      os << "[0]";
    }
      } else {
    os << "[0]";
      }
      os << ";\n";
    }
  }
}

void SSDfgOutput::printEmuDFG(ostream& os, string dfg_name, string* realName, int* iter, vector<int>* output_sizes) {
  output = true;
  //First, split name into realName and subIter
  if((_name.find_first_of("0123456789") < _name.length()) && !_scalar) {
    _realName = _name.substr(0, _name.find_first_of("0123456789"));
    _realName = _realName.substr(0, _realName.find_last_of("_"));
    //Get subIter
    _subIter = atoi(_name.substr(_name.find_first_of("0123456789"), _name.length()).c_str());
  } else {
    _realName = _name;
    _realName = _realName.substr(0, _realName.find_last_of("_"));
    _subIter = 0;
  }
  if((realName->compare(_realName) == 0) && !_scalar) {
    //same input or output as last time, don't do anything
    _iter = *iter - 1;
    output_sizes->back() = output_sizes->back()+1;
  } else {
    _iter = *iter;
    _size = 1;
    *iter = (*iter) + 1;
    os << "#define P_" << dfg_name << "_"  << _realName << " " << _iter << endl;
    output_sizes->push_back(_size);
  }
  //SSDfgNode::printEmuDFG(os, dfg_name);
  *realName = _realName;
}

void SSDfgInput::printEmuDFG(ostream& os, string dfg_name, string* realName, int* iter, vector<int>* input_sizes) {
  //First, split name into realName and subIter
  input = true;
  if((_name.find_first_of("0123456789") < _name.length()) && !_scalar) {
    _realName = _name.substr(0, _name.find_first_of("0123456789"));
    //Get subIter
    _subIter = atoi(_name.substr(_name.find_first_of("0123456789"), _name.length()).c_str());
  } else {
    _realName = _name;
    _subIter = 0;
  }
  if(realName->compare(_realName) == 0) {
    //same input or output as last time, don't do anything
    _iter = *iter - 1;
    input_sizes->back() = input_sizes->back()+1;
  } else {
    _iter = *iter;
    _size = 1;
    *iter = (*iter) + 1;
    os << "#define P_" << dfg_name << "_"  << _realName << " " << _iter << endl;
    input_sizes->push_back(_size);
  }
  //SSDfgNode::printEmuDFG(os, dfg_name);
  *realName = _realName;
}
// End DFG Emulation -- not currently used -----------------------------------
// ---------------------------------------------------------------------------
#endif

//Connect two nodes in DFG
//assumption is that each operand's edges are
//added to in least to most significant order!
SSDfgEdge* SSDfg::connect(SSDfgNode* orig, SSDfgNode* dest, int slot,
                           SSDfgEdge::EdgeType etype, int l, int r) {
  assert(orig != dest && "we only allow acyclic dfgs");

  SSDfgEdge* new_edge = 0; //check if it's a removed edge first
  auto edge_it = removed_edges.find(make_pair(orig,dest));
  if(edge_it != removed_edges.end()) {
    new_edge = edge_it->second;
  } else {
    new_edge = new SSDfgEdge(orig, dest, etype, this, l, r);
  }

  dest->addOperand(slot,new_edge);
  orig->addOutEdge(new_edge);
  _edges.push_back(new_edge);

  return new_edge;
}

//Disconnect two nodes in DFG
void SSDfg::disconnect(SSDfgNode* orig, SSDfgNode* dest) {
  assert(orig != dest && "we only allow acyclic dfgs");

  dest->removeIncEdge(orig);
  orig->removeOutEdge(dest);
  for (auto it=_edges.begin(); it!=_edges.end(); it++) {
    if ((*it)->def() == orig && (*it)->use() == dest) {
        removed_edges[make_pair(orig,dest)] = *it;
        _edges.erase(it);
        return;
    }
  }
  assert(false && "edge was not found");
}

bool SSDfg::remappingNeeded() {
  if(dummy_map.empty()) {
    //Count the number of dummy nodes needed
    for (auto dfg_out : _outputs) {
      SSDfgInst* inst = dfg_out->out_inst();
      SSDfgNode* node = dfg_out->first_op_node();
      //if producing instruction is an input or
      // if producing instruction has more than one uses
      if (!inst || inst->num_out() > 1) {
        SSDfgInst* newNode = new SSDfgInst(this, SS_CONFIG::ss_inst_t::SS_Copy, true);
        //TODO: insert information about this dummy node
        disconnect(node, dfg_out);
        connect(node, newNode, 0, SSDfgEdge::data);
        connect(newNode, dfg_out, 0, SSDfgEdge::data);
        addInst(newNode);
        dummy_map[dfg_out] = newNode;
      }
    }
  }
  return !dummy_map.empty();
}

void SSDfg::removeDummies() {
  _orderedInsts.clear(); //invalidate dummies

  for (auto Ii=_insts.begin(),Ei=_insts.end();Ii!=Ei;++Ii)  {
    SSDfgInst* inst = *Ii;
    if(inst->isDummy()) {
       SSDfgNode* input = inst->first_op_node();
       SSDfgNode* output = inst->first_use();
       disconnect(input,inst);
       disconnect(inst,output);
       connect(input,output,0,SSDfgEdge::data);
    }
  }

  for(auto i : dummy_map) {
    removeInst(i.second); //remove from list of nodes
  }

  dummies.clear();
  dummiesOutputs.clear();
  dummys_per_port.clear();
}


void SSDfg::remap(int num_HW_FU) {
  //First Disconnect any Dummy Nodes  (this is n^2 because of remove, but w/e)
  removeDummies();

  for (auto Iout=_outputs.begin(),Eout=_outputs.end();Iout!=Eout;++Iout)  {
    SSDfgOutput* dfg_out = (*Iout);
    SSDfgInst* inst = dfg_out->out_inst();
    SSDfgNode* node = dfg_out->first_op_node();
    bool not_composed = !dfg_out->first_operand().is_composed();

    if (not_composed && (!inst || inst->num_out() > 1) && ((rand()&3)==0) ) {
      //25% chance
      disconnect(node, dfg_out);
      SSDfgInst* newNode = dummy_map[dfg_out]; //get the one we saved earlier
      connect(node, newNode, 0, SSDfgEdge::data);
      connect(newNode, dfg_out, 0, SSDfgEdge::data);
      addInst(newNode); //add to list of nodes
      dummies.insert(newNode);
      dummiesOutputs.insert(dfg_out);

      if ((int)_insts.size() + (int)dummies.size() > num_HW_FU) {
        //cerr <<"No more FUs left, so we can't put more dummy nodes,\n"
        //     <<"  so probabily we will face problems when it comes to fix timing!\n";
        break;
      }

    }
  }
}

//We may have forgotten which dummies we included in the DFG, if we went on to
//some other solution.  This function recalls dummies and reconnects things
//appropriately
void SSDfg::rememberDummies(std::set<SSDfgOutput*> d) {
  removeDummies();

  for (auto Iout=_outputs.begin(),Eout=_outputs.end();Iout!=Eout;++Iout)  {
    SSDfgOutput* dfg_out = (*Iout);
    SSDfgNode* node = dfg_out->first_op_node();

    if (d.count(dfg_out)) {
      disconnect(node, dfg_out);
      SSDfgInst* newNode = dummy_map[dfg_out]; //get the one we saved earlier
      connect(node, newNode, 0, SSDfgEdge::data);
      connect(newNode, dfg_out, 0, SSDfgEdge::data);
      addInst(newNode); //add to list of nodes
      dummies.insert(newNode);
      dummiesOutputs.insert(dfg_out);
    }
  }
}

//TODO: @vidushi, does this clear all the transient state, in case we need to do a reset?
void SSDfg::reset_simulation_state() {
  for(auto& list : transient_values) {
    list.clear();
  }
  for(auto& list : buf_transient_values) {
    list.clear();
  }
  _complex_fu_free_cycle.clear();
  _ready_nodes.clear();
  for (auto in : _nodes) {
    (*in).reset_node();
  }
}

// TODO: free all buffers and clear inputs ready
void SSDfgNode::reset_node() {
  _inputs_ready = 0;
  for (auto in_edges: _inc_edge_list) {
    (*in_edges).reset_associated_buffer();
  }
  for (auto out_edges: _uses) {
    (*out_edges).reset_associated_buffer();
  }

}

void SSDfg::printGraphviz(ostream& os, Schedule* sched)
{
  os << "Digraph G { \nnewrank=true;\n " ;

  //Insts
  for (auto insts : _insts) {
    insts->printGraphviz(os,sched);
  }

  //Inputs
  for (auto in : _inputs) {
    in->printGraphviz(os,sched);
  }

  //Outputs
  for (auto out : _outputs) {
    out->printGraphviz(os,sched);
  }

  int cluster_num=0;

  os << "\n";
  for(auto& i : _vecInputs) {
    os << "subgraph cluster_" << cluster_num++ << " {" ;
    for (auto ssin : i->inputs()) {
      os << "N" << ssin->id() << " ";
    }
    os << "}\n";
  }

  for(auto& i : _vecOutputs) {
    os << "subgraph cluster_" << cluster_num++ << " {" ;
    for (auto ssout : i->outputs()) {
      os << "N" << ssout->id() << " ";
    }
    os << "}\n";
  }
  os << "\n";


  os << "\t{ rank = same; ";
  for (auto in : _inputs)   { os << "N" << in->id() << " ";  }
  os << "}\n";

  os << "\t{ rank = same; ";
  for (auto out:_outputs)   { os << "N" << out->id() << " "; }
  os << "}\n";

  os << "}\n";
}

#if 0
void SSDfg::printEmuDFG(ostream& os, string dfg_name)
{
  string realName = "";
  vector<int> input_sizes;
  int input_iter = 0;
  const_input_iterator Iin,Ein;
  os << "#include \"ss_emu.h\"" << endl;
  os << "#include \"ss_c_insts.h\" "<< endl << endl;

  os << "#define " << dfg_name << "_size 64" << endl;

  //Inputs
  int level = 1;
  for (Iin=_inputs.begin(),Ein=_inputs.end();Iin!=Ein;++Iin)  {
    (*Iin)->printEmuDFG(os, dfg_name, &realName, &input_iter, &input_sizes);
  }
  int output_iter = 0;
  const_output_iterator Iout,Eout;

  //Outputs
  vector<int> output_sizes;
  for (Iout=_outputs.begin(),Eout=_outputs.end();Iout!=Eout;++Iout)  { (*Iout)->printEmuDFG(os, dfg_name, &realName, &output_iter, &output_sizes); }

  for (Iin=_inputs.begin(),Ein=_inputs.end();Iin!=Ein;++Iin)  {
    int new_level = (*Iin)->findDepth(os, dfg_name, 0);
    if(new_level > level) {
      level = new_level;
    }
  }

  const_inst_iterator Ii,Ei;

  int iter = 0;
  int ionodes = 0;
  //Insts
  os << endl << "inline void dfg_func_" << dfg_name << "(uint64_t** inputs, uint64_t** outputs) {" << endl;
  for (Ii=_insts.begin(),Ei=_insts.end();Ii!=Ei;++Ii)  {
    (*Ii)->printEmuDFG(os, dfg_name);
  }
  //Next, we need to assure no outputs are just direct assignments of an input;
   for (Iout=_outputs.begin(),Eout=_outputs.end();Iout!=Eout;++Iout)  {
     (*Iout)->printDirectAssignments(os, dfg_name);
   }
   os << "}" << endl << endl << endl;
  os << "static ss_config " << dfg_name << "_config = {&dfg_func_" << dfg_name << ", " << input_iter << ", new int["  << input_iter << "]{";
  iter = 1;
  for(auto viter = input_sizes.begin(); viter != input_sizes.end(); viter++) {
    os << *viter;
    ionodes += *viter;
    if(iter != input_iter) {
      os << ",";
    }
    iter++;
  }
  os << "}, " << output_iter << ", new int[" << output_iter << "]{";
  iter = 1;
  for(auto viter = output_sizes.begin(); viter != output_sizes.end(); viter++) {
    os << *viter;
    ionodes += *viter;
    if(iter != output_iter) {
      os << ",";
    }
    iter++;
  }
  os << "}, " << num_nodes()-ionodes << ", "  << level << " };" << endl;
}
#endif



//IO-Model has the hardware vector io interface mapping
void SSDfg::printPortCompatibilityWith(std::ostream& os, SS_CONFIG::SSModel* ssModel) {
  os << "set cp(pv,pn) \"Port Compatibility\" \n /";   // Print the port compatibility
  bool first=true;

  for(auto& vec_in : _vecInputs) {
    std::vector<int> matching_ports;

    for(auto& port_interf : ssModel->subModel()->io_interf().in_vports) {
      const std::vector<int>& port_m = port_interf.second->port_vec();

      if(port_m.size() >= vec_in->inputs().size()) {
        matching_ports.push_back(port_interf.first);
        CINF(os,first);
        os << vec_in->gamsName() << ".ip" << port_interf.first << " ";
      }
    }

    if(matching_ports.size()==0) {
      cout << "IN PORT \"" << vec_in->gamsName() << "\" DID NOT MATCH ANY HARDWARE PORT INTERFACE\n";
      assert(0);
    }
  }

  for(auto& vec_out : _vecOutputs) {
    std::vector<int> matching_ports;

    for(auto& port_interf : ssModel->subModel()->io_interf().out_vports) {

      const std::vector<int>& port_m = port_interf.second->port_vec();

      if(port_m.size() >= vec_out->outputs().size()) {
        matching_ports.push_back(port_interf.first);
        CINF(os,first);
        os << vec_out->gamsName() << ".op" << port_interf.first << " ";
      }
    }

    if(matching_ports.size()==0) {
      cout << "OUT PORT \"" << vec_out->gamsName() << "\" DID NOT MATCH ANY HARDWARE PORT INTERFACE\n";
      assert(0);
    }

  }

  os << "/;\n";
}




void SSDfg::calc_minLats() {
  list<SSDfgNode* > openset;
  set<bool> seen;
  for (auto elem : _inputs) {
    openset.push_back(elem);
    seen.insert(elem);
  }

  //populate the schedule object
  while(!openset.empty()) {
    SSDfgNode* n = openset.front();
    openset.pop_front();

    int cur_lat = 0;

    for(auto elem : n->in_edges()) {
      SSDfgNode* dn = elem->def();
      if(dn->min_lat() > cur_lat) {
        cur_lat = dn->min_lat();
      }
    }

    if(SSDfgInst* inst_n = dynamic_cast<SSDfgInst*>(n)) {
      cur_lat += inst_lat(inst_n->inst()) + 1;
    } else if(dynamic_cast<SSDfgInput*>(n)) {
      cur_lat=0;
    } else if(dynamic_cast<SSDfgOutput*>(n)) {
      cur_lat+=1;
    }

    n->set_min_lat(cur_lat);

    for(auto elem : n->uses()) {
      SSDfgNode* un = elem->use();

      bool ready = true;
      for(auto elem : un->in_edges()) {
        SSDfgNode* dn = elem->def();
        if(!seen.count(dn)) {
          ready = false;
          break;
        }
      }
      if(ready) {
        seen.insert(un);
        openset.push_back(un);
      }
    }
  }
}

//Gams related
void SSDfg::printGams(std::ostream& os,
                      std::unordered_map<string,SSDfgNode*>& node_map,
                      std::unordered_map<std::string,SSDfgEdge*>& edge_map,
                      std::unordered_map<std::string, SSDfgVec*>& port_map) {

  os << "$onempty\n";

  {
    bool is_first = true;
    os << "set v \"verticies\" \n /";   // Print the set of Nodes:
    for (auto elem : _nodes) {
      if (!is_first) os << ", ";
      os << elem->gamsName();
      assert(elem);
      node_map[elem->gamsName()] = elem;
      is_first = false;
    }
    os << "/;\n";
  }

  {
    bool is_first = true;
    os << "set inV(v) \"input verticies\" /";   // Print the set of Nodes:
    for (auto elem : _inputs) {
      if (!is_first)
        os << ", ";
      assert(elem);
      os << elem->gamsName();
      is_first = false;
    }
    os << "/;\n";
  }

  {
    bool is_first = true;
    os << "set outV(v) \"output verticies\" /";   // Print the set of Nodes:
    for (auto elem : _outputs) {
      if (!is_first)
        os << ", ";
      os << elem->gamsName();
      assert(elem);
      is_first = false;
    }
    os << "/;\n";
  }

  {
    os << "parameter minT(v) \"Minimum Vertex Times\" \n /";
    for (auto Ii = _nodes.begin(), Ei = _nodes.end(); Ii != Ei; ++Ii) {
      if (Ii != _nodes.begin()) os << ", ";
      SSDfgNode *n = *Ii;
      int l = n->min_lat();
      if (SSDfgInst *inst = dynamic_cast<SSDfgInst *>(n)) {
        l -= inst_lat(inst->inst());
      }
      os << n->gamsName() << " " << l;
    }
    os << "/;\n";
  }


  os << "set iv(v) \"instruction verticies\";\n";
  os << "iv(v) = (not inV(v)) and (not outV(v));\n";

  for(int i = 2; i < SS_NUM_TYPES; ++i) {
    ss_inst_t ss_inst = (ss_inst_t)i;

    os << "set " << name_of_inst(ss_inst) << "V(v) /";
    bool first=true;

    for (auto dfg_inst : _insts) {

      if(ss_inst == dfg_inst->inst()) {
        CINF(os,first);
        os << dfg_inst->gamsName();
      }
    }
    os << "/;\n";
  }

  bool first=true;
  os << "set pv(*) \"Port Vectors\" \n /";   // Print the set of port vertices:
  for(auto& i : _vecInputs) {
    CINF(os,first);
    os << i->gamsName() << " ";
    port_map[i->gamsName()]=i;
  }
  for(auto& i : _vecOutputs) {
    CINF(os,first);
    os << i->gamsName() << " ";
    port_map[i->gamsName()]=i;
  }
  os << "/;\n";

  first=true;
  os << "parameter VI(pv,v) \"Port Vector Definitions\" \n /";   // Print the set of port vertices mappings:
  for(auto& i : _vecInputs) {
    int ind=0;
    for (auto ssin : i->inputs()) {
      CINF(os,first);
      os << i->gamsName() << "." << ssin->gamsName() << " " << ind+1;
    }
  }
  for(auto& i : _vecOutputs) {
    int ind=0;
    for (auto ssout : i->outputs()) {
      CINF(os,first);
      os << i->gamsName() << "." << ssout->gamsName() << " " << ind+1;
    }
  }
  os << "/;\n";

  // -------------------edges ----------------------------
  os << "set e \"edges\" \n /";   // Print the set of edges:

  for (auto Ie=_edges.begin(),Ee=_edges.end();Ie!=Ee;++Ie)  {
    if(Ie!=_edges.begin()) os << ", ";
    os << (*Ie)->gamsName();
    edge_map[(*Ie)->gamsName()]=*Ie;
  }
  os << "/;\n";

  //create the kindC Set
  os << "set kindV(K,v) \"Vertex Type\"; \n";

  // --------------------------- Enable the Sets ------------------------
  os << "kindV('Input', inV(v))=YES;\n";
  os << "kindV('Output', outV(v))=YES;\n";

  for(int i = 2; i < SS_NUM_TYPES; ++i) {
    ss_inst_t ss_inst = (ss_inst_t)i;
    os << "kindV(\'" << name_of_inst(ss_inst) << "\', " << name_of_inst(ss_inst) << "V(v))=YES;\n";
  }

  // --------------------------- Print the linkage ------------------------
  os << "parameter Gve(v,e) \"vetex to edge\" \n /";   // def edges
  for (auto Ie=_edges.begin(),Ee=_edges.end();Ie!=Ee;++Ie)  {
    if(Ie!=_edges.begin()) os << ", ";

    SSDfgEdge* edge = *Ie;
    os << edge->def()->gamsName() << "." << edge->gamsName() << " 1";
  }
  os << "/;\n";

  os << "parameter Gev(e,v) \"edge to vertex\" \n /";   // use edges
  for (auto Ie=_edges.begin(),Ee=_edges.end();Ie!=Ee;++Ie)  {
    if(Ie!=_edges.begin()) os << ", ";

    SSDfgEdge* edge = *Ie;
    os << edge->gamsName() << "." << edge->use()->gamsName() << " 1";
  }
  os << "/;\n";

  os << "set intedges(e) \"edges\" \n /";   // Internal Edges
  first =true;
  for (auto Ie=_edges.begin(),Ee=_edges.end();Ie!=Ee;++Ie)  {
    SSDfgEdge* edge = *Ie;

    if(!dynamic_cast<SSDfgInput*>(edge->def()) && !dynamic_cast<SSDfgOutput*>(edge->use()) ) {
      if (first) first = false;
      else os << ", ";
      os << edge->gamsName();
    }
  }
  os << "/;\n";

  os << "parameter delta(e) \"delay of edge\" \n /";   // Print the set of edges:
  for (auto Ie=_edges.begin(),Ee=_edges.end();Ie!=Ee;++Ie)  {
    if(Ie!=_edges.begin()) os << ", ";
    SSDfgEdge* edge = *Ie;

    if(SSDfgInst* dfginst = dynamic_cast<SSDfgInst*>(edge->def())) {
       os << (*Ie)->gamsName() << " " << inst_lat(dfginst->inst());
    } else {
       os << (*Ie)->gamsName() << " " << "0";  //TODO: WHAT LATENCY SHOULD I USE??
    }
  }
  os << "/;\n";
}
