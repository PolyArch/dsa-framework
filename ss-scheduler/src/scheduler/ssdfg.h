#ifndef __SSDFG_H__
#define __SSDFG_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include "ssinst.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <queue>
#include <list>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include "model.h"
#include <bitset>
#include <unordered_set>
#include <math.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

void checked_system(const char* command);

class Schedule;
class SSDfgNode;
class SSDfg;

class SSDfgEdge {
public:
  SSDfgEdge() {}

  enum EdgeType {
    data, ctrl, ctrl_true, ctrl_false
  };

  EdgeType etype() { return _etype; }

  SSDfgEdge(SSDfgNode *def, SSDfgNode *use,
             EdgeType etype, SSDfg *ssdfg, int l = 0, int r = 63);

  int id() { return _ID; }

  SSDfgNode *def() const {
    return _def;
  }

  SSDfgNode *use() const {
    return _use;
  }

  std::string gamsName();

  std::string name();

  void set_delay(int d) { _delay = d; }

  int delay() { return _delay; }

  // void compute_next();
  void compute_after_push(bool print, bool verif);

  void compute_after_pop(bool print, bool verif);

  void push_in_buffer(uint64_t v, bool valid, bool print, bool verif) {
    assert(_data_buffer.size() < buf_len && "Trying to push in full buffer\n");
    // std::cout << this->name() << " The value I am trying to push in buffer (Associated with an operand): " << v << "\n";
    _data_buffer.push(std::make_pair(v, valid));
    compute_after_push(print, verif);
  }

  bool is_buffer_full() {
    return (_data_buffer.size() == buf_len);
  }

  bool is_buffer_empty() {
    return _data_buffer.empty();
    // return (_data_buffer.size()!=0);
  }

  uint64_t extract_value(uint64_t v_in);

  uint64_t get_buffer_val() {
    assert(!_data_buffer.empty());
    // std::cout << "value at top of buffer: " << _data_buffer.front().first << "\n";
    // std::cout << "value extracted: " << extract_value(_data_buffer.front().first) << "\n";
    return extract_value(_data_buffer.front().first);
  }

  bool get_buffer_valid() {
    assert(!_data_buffer.empty());
    return _data_buffer.front().second;
  }

  void pop_buffer_val(bool print, bool verif) {
    assert(!_data_buffer.empty() && "Trying to pop from empty queue\n");
    // std::cout << "came here to pop buffer val\n";
    _data_buffer.pop();
    compute_after_pop(print, verif);
  }

  int bitwidth() { return _r - _l + 1; }
  int l() { return _l; }
  int r() { return _r; }

  uint64_t get_value();

  void reset_associated_buffer() {
    while(!_data_buffer.empty()) _data_buffer.pop();
  }

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned int version);

private:
  int _ID;
  SSDfg *_ssdfg;
  SSDfgNode *_def, *_use;
  EdgeType _etype;
  int _l, _r;

  //Runtime Types
  std::queue<std::pair<uint64_t, bool>> _data_buffer;
  // unsigned int buf_len = 1;
  // using 2 since 1st entry is used for bp
  unsigned int buf_len = 9;
  // unsigned int buf_len = FU_BUF_LEN;

  int _delay = 0;
};

class SSDfgInst;

// Datastructure describing the operand
struct SSDfgOperand {
  SSDfgOperand() {}

  SSDfgOperand(SSDfgEdge *e) {
    edges.push_back(e);
  }

  SSDfgOperand(std::vector<SSDfgEdge *> es) {
    edges = es;
  }

  SSDfgOperand(uint64_t i) {
    imm = i;
  }

  //Helper functions
  SSDfgEdge *get_first_edge() const {
    if (edges.empty()) {
      return nullptr;
    } else {
      return edges[0];
    }
  }

  void clear() {
    imm = 0;
    edges.clear();
  }

  bool is_ctrl() {
    for (SSDfgEdge *e : edges) {
      if (e->etype() == SSDfgEdge::ctrl) {
        return true;
      }
    }
    return false;
  }

  bool is_imm() { return edges.empty(); }

  bool is_composed() { return edges.size() > 1; }


  //Functions which manipulate dynamic state
  uint64_t get_value() { //used by simple simulator
    uint64_t base = imm;
    int cur_bit_pos = 0;
    for (SSDfgEdge *e : edges) {
      base |= e->get_value() << cur_bit_pos;
      cur_bit_pos += e->bitwidth();
    }
    assert(cur_bit_pos <= 64); // max bitwidth is 64
    return base;
  }

  uint64_t get_buffer_val() { //used by backcgra simulator
    uint64_t base = imm;
    int cur_bit_pos = 0;
    for (SSDfgEdge *e : edges) {
      base |= e->get_buffer_val() << cur_bit_pos;
      cur_bit_pos += e->bitwidth();
    }
    assert(cur_bit_pos <= 64); // max bitwidth is 64
    return base;
  }

  uint64_t get_buffer_valid() { //used by backcgra simulator
    for (SSDfgEdge *e : edges) {
      if (!e->get_buffer_valid()) {
        return false;
      }
    }
    return true;
  }

  uint64_t is_buffer_empty() { //used by backcgra simulator
    for (SSDfgEdge *e : edges) {
      if (e->is_buffer_empty()) {
        return true;
      }
    }
    return false;
  }

  void pop_buffer_val(bool print, bool verif) {
    for (SSDfgEdge *e : edges) {
      e->pop_buffer_val(print, verif);
    }
  }

  // An Operand is valid as long as ALL of its edges are valid
  bool valid();

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned int version);

  //Edges concatenated in bit order from least to most significant
  std::vector<SSDfgEdge *> edges;
  uint64_t imm = 0;
};

//DFG Node -- abstract base class
class SSDfgNode {
public:
  SSDfgNode() {}

  enum V_TYPE {
    V_INVALID, V_INPUT, V_OUTPUT, V_INST, V_NUM_TYPES
  };

  virtual void printGraphviz(std::ostream &os, Schedule *sched = nullptr);
  //virtual void printEmuDFG(std::ostream& os, std::string dfg_name);

  // some issue with this function
  virtual uint64_t invalid() { return _invalid; } //execution-related

  SSDfgNode(SSDfg *ssdfg, V_TYPE v);
  SSDfgNode(SSDfg *ssdfg, V_TYPE v, const std::string &name);

  typedef std::vector<SSDfgEdge *>::const_iterator const_edge_iterator;
  typedef std::vector<SSDfgOperand>::const_iterator const_op_iterator;

  //Add edge to operand in least to most significant bit order
  void addOperand(unsigned pos, SSDfgEdge *e) {
    assert(pos <= 4);
    if (_ops.size() <= pos) {
      _ops.resize(pos + 1);
    }

    _ops[pos].edges.push_back(e);
    //if(_ops[pos].edges.size()) {
    //  std::cerr << "ERROR: overwriting op at pos" << pos << "\n";
    //  assert(0);
    //}

    //for(SSDfgEdge* e : op.edges) {
    //  _inc_edge_list.push_back(e);
    //}
    _inc_edge_list.push_back(e);
  }

  void addOutEdge(SSDfgEdge *edge) {
    _uses.push_back(edge);
  }

  void reset_node();

  void validate() {
    for (unsigned i = 0; i < _ops.size(); ++i) {
      SSDfgEdge *edge = _inc_edge_list[i];
      assert(edge == nullptr || edge->use() == this);
    }
    for (unsigned i = 0; i < _uses.size(); ++i) {
      SSDfgEdge *edge = _uses[i];
      assert(edge->def() == this);
    }
  }

  //TODO:FIXME this won't work for decomp-CGRA
  void removeIncEdge(SSDfgNode *orig) {
    for (unsigned i = 0; i < _ops.size(); ++i) {
      SSDfgEdge *edge = _ops[i].get_first_edge();;
      if (edge->def() == orig) {
        _ops[i].clear();
        return;
      }
    }
    assert(false && "edge was not found");
  }

  void removeOutEdge(SSDfgNode *dest) {
    for (auto it = _uses.begin(); it != _uses.end(); it++) {
      if ((*it)->use() == dest) {
        _uses.erase(it);
        return;
      }
    }
    assert(false && "edge was not found");
  }

  virtual int compute(bool print, bool verif) {
    return 0;
  }

  //-----------------------------------------
  virtual int update_next_nodes(bool print, bool verif) {
    return 0;
  }

  /*
  virtual int update_next_nodes(bool print, bool verif) {
      // called from push_vector: i/p node
      int num_computed = 0;
      SSDfgNode* n = this->first_use();
      num_computed = n->inc_inputs_ready_backcgra(print, verif);

      return num_computed;
  }
      auto it = this->uses_begin();
      if(!(*it)->is_buffer_full()){
        num_computed = n->inc_inputs_ready_backcgra(print, verif);
      }
 */

  virtual int compute_backcgra(bool print, bool verif) {
    //for an output node
    _inputs_ready = 0; // hopefully if it comes from inc_inputs_ready
    return 0;
  }

  //-------------------------------------------------------

  SSDfgEdge *getLinkTowards(SSDfgNode *to) {
    for (auto elem : _uses) {
      if (elem && elem->use() == to) {
        return elem;
      }
    }
    return nullptr;
  }

  virtual int maxThroughput() {
    if (_max_thr == 0) {
      for (auto elem : _uses) {
        _max_thr = std::max(_max_thr, elem->use()->maxThroughput());
      }
    }
    return _max_thr;
  }

  virtual int lat_of_inst() {
    return 0;
  }

  virtual void depInsts(std::vector<SSDfgInst *> &insts) {
    for (auto it : _uses) {
      SSDfgNode *use = it->use();
      if (std::find(insts.begin(), insts.end(), use) != insts.end()) {
        use->depInsts(insts);
      }
    }
  }

  int num_inputs_ready() { return _inputs_ready; }

  SSDfgEdge *first_inc_edge() { return _ops[0].edges[0]; }

  SSDfgOperand &first_operand() { return _ops[0]; }

  SSDfgNode *first_op_node() { return (_ops[0].edges[0]->def()); }

  SSDfgNode *first_use() { return (_uses[0]->use()); }

  size_t num_inc() const { return _ops.size(); }

  size_t num_out() const { return _uses.size(); }

  virtual std::string name() = 0;     //pure func
  void setName(std::string name) { _name = name; }

  virtual std::string gamsName() = 0;

  const std::vector<SSDfgOperand> &ops() {return _ops; }

  const std::vector<SSDfgEdge*> &in_edges() { return _inc_edge_list; }

  const std::vector<SSDfgEdge*> &uses() { return _uses; }

  int id() { return _ID; }

  void set_value(uint64_t v, bool valid) {
    _val = v;
    _invalid = !valid;
  }

  bool get_bp() {
    bool bp = false;
    for (auto elem : _uses)
      if (elem->is_buffer_full()) {
        bp = true;
      }
    return bp;
  }

  void set_outputnode(uint64_t v, bool valid, bool avail) {
    _val = v;
    _invalid = !valid;
    _avail = avail;
  }

  // Check's all consumers for backpressure-freedom,
  // If backpressure,
  void set_node(uint64_t v, bool valid, bool avail, bool print, bool verif) {
    _val = v;
    _invalid = !valid;
    _avail = avail;

    // std::cout << "came here to set the node: " << name() <<" to value: "<<v<<" and avail: "<<avail<<"\n";
    // no need to do anything for output node
    if (this->num_out() == 0) { return; }
    if (avail) {
      if (!get_bp()) {
        for (auto iter = _uses.begin(); iter != _uses.end(); iter++) {
          (*iter)->push_in_buffer(v, valid, print, verif);
        }
        _avail = false;
      } else {
        this->set_value(v, valid, avail, 1); // after 1 cycle
      }
    }
  }

  // sets value at this cycle
  void set_value(uint64_t v, bool valid, bool avail, int cycle);
  //--------------------------------------------

  uint64_t get_value() { return _val; }

  bool get_avail() { return _avail; }

  bool input = false;
  bool output = false;
  int _iter;

  int min_lat() { return _min_lat; }

  void set_min_lat(int i) { _min_lat = i; }

  int sched_lat() { return _sched_lat; }

  void set_sched_lat(int i) { _sched_lat = i; }

  int inc_inputs_ready(bool print, bool verif) {
    _inputs_ready += 1;
    if (_inputs_ready == num_inc_edges()) {
      int num_computed = compute(print, verif);
      _inputs_ready = 0;
      return num_computed;
    }
    return 0;
  }


  int inc_inputs_ready_backcgra(bool print, bool verif);

  int get_inputs_ready() {
    return _inputs_ready;
  }

  void push_buf_dummy_node();

  void set_node_id(int i) { _node_id = i; }

  int node_id() { return _node_id; }

  bool is_temporal();

  virtual int bitwidth() { return 64; }
  //---------------------------------------------------------------------------

  V_TYPE type() { return _vtype; }

  int group_id() { return _group_id; }

  int num_inc_edges() { return _inc_edge_list.size(); }

private:

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned int version);

protected:
  SSDfg *_ssdfg;  //sometimes this is just nice to have : )

  int _node_id = -1; //hack for temporal simulator to remember _node_id

  //Dynamic stuff
  uint64_t _val = 0; //dynamic var (setting the default value)
  bool _avail = false; // if their is data in the output buffer
  // bool _backPressure=false;
  uint64_t _invalid = false;
  //bool _is_new_val = 0; // new variable
  int _inputs_ready = 0; //dynamic inputs ready
  std::vector<bool> _back_array;     //in edges

  //Static Stuff
  int _ID;
  std::string _name;
  std::vector<SSDfgOperand> _ops;     //in edges
  std::vector<SSDfgEdge *> _inc_edge_list; //in edges
  std::vector<SSDfgEdge *> _uses;          //out edges
  int _min_lat = 0;
  int _sched_lat = 0;
  int _max_thr = 0;
  int _group_id = 0; //which group do I belong to

  V_TYPE _vtype;
};


//Control map defines the set of control signals which will be passed into
//the configuration if any of these bits are set.
class CtrlMap {
public:
  enum ctrl_flag {BACKP1, BACKP2, DISCARD, RESET, ABSTAIN, NUM_CTRL};

  CtrlMap() {
    add_ctrl("b1", BACKP1);
    add_ctrl("b2", BACKP2);
    add_ctrl("d" , DISCARD);
    add_ctrl("r" , RESET);
    add_ctrl("a" , ABSTAIN);
  }

  std::string decode_control(ctrl_flag c) {
    if(_decode_map.count(c)) {
      return _decode_map[c];
    } else {
      assert(0);
    }
  }

  ctrl_flag encode_control(std::string s) {
    if(_encode_map.count(s)) {
      return _encode_map[s];
    } else {
      std::cout << "Bad Control Symobol: " << s <<"\n";
      assert("Bad Control Symbol" && 0);
    }
  }

private:
  std::map<std::string,ctrl_flag> _encode_map;
  std::map<ctrl_flag,std::string> _decode_map; //this could be an array i suppose

  void add_ctrl(const char* s, ctrl_flag c) {
    _encode_map[std::string(s)]=c;
    _decode_map[c]=std::string(s);
  }
};

typedef std::vector<std::string> string_vec_t;

//post-parsing control signal definitions (mapping of string of flag to it's value?)
typedef std::map<int,string_vec_t> ctrl_def_t;


class CtrlBits {
public:
  static int bit_loc(int c_val, CtrlMap::ctrl_flag flag) {
    return c_val * CtrlMap::NUM_CTRL + flag; // should have returned NUM_CTRL should be 5 and flag should be 0?
  }
  CtrlBits(ctrl_def_t d) {
    for(auto i : d) {
      int key = i.first;
      auto vec = i.second;
      // for debug
      // std::cout << "key: " << key << "\n";
      for(auto s : vec) {
        CtrlMap::ctrl_flag  local_bit_pos = ctrl_map.encode_control(s);
        int final_pos = bit_loc(key,local_bit_pos);
        _bits.set(final_pos);
      }
    }
  }
  CtrlBits(uint64_t b) {
    _bits = b;
  }
  CtrlBits(){}

  void print_rep() {
    for(int i = 0; i < 4; ++i) {
      printf("ctrl input %d:", i);
      for(int c = 0; c<CtrlMap::NUM_CTRL;++c) {
        if(isSet(i,(CtrlMap::ctrl_flag)c)) {
          printf(" %s",ctrl_map.decode_control((CtrlMap::ctrl_flag)c).c_str());
        }
      }
      printf("\n");
    }
  }

  uint64_t bits() {return _bits.to_ullong();}

  // What is c_val? bits is the control lost, c_val is the value you read here
  bool isSet(int c_val, CtrlMap::ctrl_flag flag) {
    // std::cout << "In in set, c_val: " << c_val << "\n";
    // std::cout << "num_ctrl: " << (int)CtrlMap::NUM_CTRL << " and flag: " << (int)flag << "\n";
    return _bits.test(bit_loc(c_val,flag));
  }

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned version);

  static CtrlMap ctrl_map;
  std::bitset<32> _bits;
};

struct SymEntry;
struct EdgeEntry {
  int l, r;
  SSDfgNode* node;
};


struct SymEntry {
  SymEntry() {
    type = SYM_INV;
    flag = FLAG_INV;
  }

  SymEntry(uint64_t i) {
    type = SYM_INT;
    data.i = i;
    width = 1;
    bitwidth = 64;
  }

  SymEntry(uint64_t i1, uint64_t i2) {
    type = SYM_INT;
    data.i = ((i2 & 0xFFFFFFFF) << 32) | ((i1 & 0xFFFFFFFF) << 0);
    width = 2;
    bitwidth = 64;
  }

  SymEntry(uint64_t i1, uint64_t i2, uint64_t i3, uint64_t i4) {
    type = SYM_INT;
    data.i = ((i4 & 0xFFFF) << 48) | ((i3 & 0xFFFF) << 32) |
             ((i2 & 0xFFFF) << 16) | ((i1 & 0xFFFF) << 0);
    width = 4;
    bitwidth = 64;
  }

  SymEntry(double d) {
    type = SYM_DUB;
    data.d = d;
    width = 1;
    bitwidth = 64;
  }

  SymEntry(double d1, double d2) {
    float f1 = d1, f2 = d2;
    type = SYM_DUB;
    data.f.f1 = f1;
    data.f.f2 = f2;
    width = 2;
    bitwidth = 64;
  }

  SymEntry(SSDfgNode *node) {
    type = SYM_NODE;
    edge_entries = new std::vector<EdgeEntry>();
    EdgeEntry e;
    e.node = node;
    e.l = 0;
    e.r = 63;
    edge_entries->push_back(e);
    width = 1;
  }

  SymEntry(SSDfgNode *node, int l, int r) {
    type = SYM_NODE;
    edge_entries = new std::vector<EdgeEntry>();
    EdgeEntry e;
    e.node = node;
    e.l = l;
    e.r = r;
    edge_entries->push_back(e);
    width = 1;
  }

  void set_flag(std::string &s) {
    if (s == std::string("pred")) {
      flag = FLAG_PRED;
    } else if (s == std::string("inv_pred")) {
      flag = FLAG_INV_PRED;
    } else if (s == std::string("control")) {
      flag = FLAG_CONTROL;
    } else {
      printf("qualifier: %s unknown", s.c_str());
      assert(0 && "Invalid argument qualifier");
    }
  }

  void take_union(SymEntry entry) {
    // edge_entries->push_back(entry.firstEdge());
    // TODO: change edge_entries to deque
    edge_entries->insert(edge_entries->begin(), entry.firstEdge());
  }

  EdgeEntry firstEdge() {
    assert(type == SymEntry::SYM_NODE && "trying to get node from wrong type");
    if (edge_entries->size() != 1) {
      assert(0 && "Node must have exactly one entry to extract");
    }
    return edge_entries->at(0);
  }

  SSDfgNode *node() {
    assert(type == SymEntry::SYM_NODE && "trying to get node from wrong type");
    if (edge_entries->size() != 1) {
      assert(false && "Node must have exactly one entry to extract");
    }
    return edge_entries->at(0).node;
  }

  void set_control_list(ctrl_def_t &d) {
    ctrl_bits = CtrlBits(d);
  }

  /*!
   * \brief Set the width of bits of the edge.
   * \param l, r: The bit slice range [l, r]
   */
  void set_bitslice_params(int l, int r) {
      // std::cout << "IT CAME IN SET BITSLICE PARAMS FROM WHERE\n";
    assert(type == SymEntry::SYM_NODE && "trying to get node from wrong type");
    if (edge_entries->size() != 1) {
      assert(false && "Node must have exactly one entry to extract");
    }

    EdgeEntry &edge_entry = edge_entries->at(0);
    //SSDfgNode *node = edge_entry.node;
    int bitwidth = r - l + 1;
    assert(l <= r && r <= 64 && (bitwidth & -bitwidth) == bitwidth && "improper bitwidth");
    edge_entry.l = l;
    edge_entry.r = r;
  }

  SymEntry(const SymEntry &s) {
    this->type = s.type;
    this->flag = s.flag;
    this->ctrl_bits = s.ctrl_bits;
    this->width = s.width;
    this->bitwidth = s.bitwidth;
    this->ctrl_bits = s.ctrl_bits;
    this->data = s.data;

    if (s.edge_entries) {
      this->edge_entries = new std::vector<EdgeEntry>();
      for (auto &i : *s.edge_entries) {
        this->edge_entries->push_back(i);
      }
    }
  }

  //TODO:FIXME:Known-Issue
  //The DFG parser, as-is, LEAKs the edge_entries vector.
  //This is because if I uncomment the destructor, bad bad things happen,
  //and i'm not sure why, and i'm not sure I care enough, yet.

  //~SymEntry() {
  //  if(edge_entries) {
  //    delete edge_entries;
  //  }
  //}

  //data
  enum enum_type {
    SYM_INV, SYM_INT, SYM_DUB, SYM_NODE
  } type;
  enum enum_flag {
    FLAG_NONE, FLAG_INV, FLAG_PRED, FLAG_INV_PRED,
    FLAG_BGATE, FLAG_CONTROL
  } flag = FLAG_NONE;
  CtrlBits ctrl_bits;
  int width;
  int bitwidth;

  //Union data is just used for immediates
  union union_data {
    uint64_t i;
    double d;
    struct struct_data {
      float f1, f2;
    } f;
  } data;

  std::vector<EdgeEntry> *edge_entries = nullptr;

};

//Instruction
class SSDfgInst : public SSDfgNode {
public:
  SSDfgInst() {}

  SSDfgInst(SSDfg *ssdfg, SS_CONFIG::ss_inst_t inst, bool is_dummy = false) : SSDfgNode(ssdfg, V_INST),
                                                        _predInv(false), _isDummy(is_dummy),
                                                        _imm_slot(-1), _subFunc(0), _ssinst(inst) {
    _reg.resize(8, 0);
    _reg_32.resize(16, 0);
    _reg_16.resize(32, 0);
    _reg_8.resize(64, 0);
  }


  SSDfgInst(SSDfg *ssdfg) : SSDfgNode(ssdfg, V_INST),
                             _predInv(false), _isDummy(false),
                             _imm_slot(-1), _subFunc(0) {
    _reg.resize(8, 0);
    _reg_32.resize(16, 0);
    _reg_16.resize(32, 0);
    _reg_8.resize(64, 0);

  }

  void printGraphviz(std::ostream &os, Schedule *sched = nullptr);
  //virtual void printEmuDFG(std::ostream& os, std::string dfg_name);

  virtual int lat_of_inst() {
    return inst_lat(inst());
  }

  void setImm(uint64_t val) { _imm = val; }

  uint64_t imm() { return _imm; }

  void setPredInv(bool predInv) { _predInv = predInv; }

  bool predInv() { return _predInv; }

  bool isDummy() { return _isDummy; }

  SS_CONFIG::ss_inst_t inst() { return _ssinst; }

  //Adding new function in the header file
  int update_next_nodes(bool print, bool verif);

  virtual int maxThroughput() {
    if (_max_thr == 0) {
      _max_thr = inst_thr(inst());
      for (auto it = _uses.begin(); it != _uses.end(); it++) {
        _max_thr = std::max(_max_thr, (*it)->use()->maxThroughput());
      }
    }
    return _max_thr;
  }

  virtual void depInsts(std::vector<SSDfgInst *> &insts) {
    insts.push_back(this);
    for (auto it = _uses.begin(); it != _uses.end(); it++) {
      SSDfgNode *use = (*it)->use();
      if (std::find(insts.begin(), insts.end(), use) != insts.end()) {
        use->depInsts(insts);
      }
    }
  }

  std::string name() {
    std::stringstream ss;
    ss << _name << ":";
    ss << SS_CONFIG::name_of_inst(_ssinst);
    if (_imm_slot != -1) {
      ss << " Imm:" << _imm;
    }
    return ss.str();
  }

  std::string gamsName();

  void setImmSlot(int i);

  int immSlot() const { return _imm_slot; }

  void setSubFunc(int i) { _subFunc = i; }

  int subFunc() const { return _subFunc; }

  uint64_t do_compute(uint64_t &discard);

  virtual int compute(bool print, bool verif);

  // new line added
  virtual int compute_backcgra(bool print, bool verif);

  void set_verif_id(std::string s) { _verif_id = s; }

  virtual uint64_t invalid() {
    return _invalid;
  }

  void set_ctrl_bits(CtrlBits c) {
    // std::cout << "Should come here in set_ctrl_bits to set bits to: " << c << "\n";
    _ctrl_bits = c;
    // if(_ctrl_bits.bits() != 0) {
    //   _ctrl_bits.print_rep();
    // }
  }

  uint64_t ctrl_bits() { return _ctrl_bits.bits(); }

  virtual int bitwidth() {
    return SS_CONFIG::bitwidth[_ssinst];
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);

  std::ofstream _verif_stream;
  std::string _verif_id;
  std::vector<uint64_t> _input_vals;
  std::vector<uint32_t> _input_vals_32;
  std::vector<uint16_t> _input_vals_16;
  std::vector<uint8_t> _input_vals_8;

  bool _predInv;
  bool _isDummy;
  int _imm_slot;
  int _subFunc;
  CtrlBits _ctrl_bits;

  std::vector<uint64_t> _reg;
  std::vector<uint32_t> _reg_32;
  std::vector<uint16_t> _reg_16;
  std::vector<uint8_t> _reg_8;

  uint64_t _imm;
  SS_CONFIG::ss_inst_t _ssinst;
};

class SSDfgVec;
class SSDfgIO : public SSDfgNode {
public:
  SSDfgIO() {}

  SSDfgIO(SSDfg *ssdfg, const std::string& name, SSDfgVec *vec_, V_TYPE v);

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);
  SSDfgVec *vector() { return vec_; }

  std::string name() {
    std::stringstream ss;
    ss << _name << ":";
    ss << i_or_o() << id();
    ss << _name;
    return ss.str();
  }

protected:
  SSDfgVec *vec_;

  char i_or_o() {
    if (_vtype == V_INPUT || _vtype == V_OUTPUT)
      return "IO"[_vtype == V_OUTPUT];
    return 'X';
  }
};

class SSDfgVecInput;

class SSDfgInput : public SSDfgIO {       //inturn inherits ssnode
public:
  SSDfgInput() {}

  void printGraphviz(std::ostream &os, Schedule *sched = nullptr);

  SSDfgInput(SSDfg *ssdfg, const std::string &name, SSDfgVec *vec_) : SSDfgIO(ssdfg, name, vec_, V_INPUT) {}

  SSDfgVecInput *input_vec();

  std::string gamsName();

  // after inc inputs ready?
  virtual int compute_backcgra(bool print, bool verif) {
    int num_computed = 0;
    for (auto iter = _uses.begin(); iter != _uses.end(); iter++) {
      // std::cout << "starts computation for this vector at an input\n";
      SSDfgNode *use = (*iter)->use();
      num_computed += use->inc_inputs_ready_backcgra(print, verif);
    }
    return num_computed;
  }
  //---------------------------------------
  virtual int compute(bool print, bool verif) {
    int num_computed = 0;
    for (auto iter = _uses.begin(); iter != _uses.end(); iter++) {
      SSDfgNode *use = (*iter)->use();

      num_computed += use->inc_inputs_ready(print, verif);
    }
    return num_computed;
  }


private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned int version);

};

class SSDfgVecOutput;
class SSDfgOutput : public SSDfgIO {
public:
  SSDfgOutput() {}

  void printGraphviz(std::ostream &os, Schedule *sched = nullptr);

  void printDirectAssignments(std::ostream &os, std::string dfg_name);
  //virtual void printEmuDFG(std::ostream& os, std::string dfg_name, std::string* realName, int* iter, std::vector<int>* output_sizes);

  SSDfgOutput(SSDfg *ssdfg, const std::string &name, SSDfgVec *vec_) : SSDfgIO(ssdfg, name, vec_, V_OUTPUT) {}


  SSDfgVecOutput *output_vec();

  std::string gamsName();

  //returns the instruction producing the
  //value to this output node
  //Returns nullptr if the producing instruction is an input!
  //TODO:FIXME: This might not be safe for decomp-CGRA
  SSDfgInst *out_inst() {
    return dynamic_cast<SSDfgInst *>(_ops[0].edges[0]->def());
  }

  //retrieve the value of the def
  uint64_t retrieve() {
    assert(_ops.size() == 1);
    return _ops[0].get_value();
  }

  uint64_t parent_invalid() {
    return _ops[0].edges[0]->def()->invalid();
  }

  virtual uint64_t invalid() {
    return _invalid;
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned int version);

};

//vector class
class SSDfgVec {
public:
  SSDfgVec() {}

  SSDfgVec(int len, const std::string &name, int id, SSDfg *ssdfg);

  int id() { return _ID; }

  void set_group_id(int id) { _group_id = id; }

  int group_id() { return _group_id; }

  bool is_temporal();

  virtual std::string gamsName() = 0;

  virtual std::string name() { return _name; }

  void set_port_width(int n){
    _port_width=n;
  }

  int get_port_width(){
    return _port_width;
  }


  void set_vp_len(int n){
    _vp_len=n;
  }

  int get_vp_len(){
    return _vp_len;
  }

  int logical_len(){
    return _vp_len;
  }

  virtual unsigned length() = 0;

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);

protected:
  std::string _name;
  int _ID;
  SSDfg *_ssdfg;
  int _group_id = 0; //which group do I belong to
  int _port_width;
  int _vp_len;
};

class SSDfgVecInput : public SSDfgVec {
public:
  SSDfgVecInput() {}

  SSDfgVecInput(int len, const std::string &name, int id, SSDfg *ssdfg) : SSDfgVec(len, name, id, ssdfg) {}

  virtual std::string gamsName() override {
    std::stringstream ss;
    ss << "IPV_" << _name;
    return ss.str();
  }

  virtual unsigned length() override {return _inputs.size();}

  bool backPressureOn();

  void addInput(SSDfgInput *in) { _inputs.push_back(in); }

  const std::vector<SSDfgInput*>& inputs() { return _inputs; }

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);

private:
  std::vector<SSDfgInput *> _inputs;
};


class SSDfgVecOutput : public SSDfgVec {
public:
  SSDfgVecOutput() {}

  SSDfgVecOutput(int len, const std::string &name, int id, SSDfg *ssdfg) : SSDfgVec(len, name, id, ssdfg) {}

  virtual std::string gamsName() override {
    std::stringstream ss;
    ss << "OPV_" << _name;
    return ss.str();
  }

  void addOutput(SSDfgOutput *out) { _outputs.push_back(out); }

  const std::vector<SSDfgOutput*>& outputs() { return _outputs; }

  virtual unsigned length() override {return _outputs.size();}

  SSDfgOutput *getOutput(int i) { return _outputs[i]; }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);

private:
  std::vector<SSDfgOutput *> _outputs;
};

class SymTab {
  void assert_exists(const std::string &s) {
    if (_sym_tab.count(s) == 0) {
      std::cerr << "Could not find" + s + "\n";
      assert(false);
    }
  }

public:
  void set(const std::string &s, const SymEntry &n) {
    //std::cout << "setting symbol " << s << " #ent:" << n.edge_entries->size() << "\n";
    _sym_tab[s] = n;
  }

  void set(const std::string &s, SSDfgNode *n) {
    //std::cout << "setting symbol " << s << " to node " << n->name() << "\n";
    _sym_tab[s] = n;

    _sym_tab[s] = SymEntry(n);
  }

  void set(std::string &s, uint64_t n) { _sym_tab[s] = SymEntry(n); }

  void set(std::string &s, double n) { _sym_tab[s] = SymEntry(n); }

  bool has_sym(std::string &s) {
    return _sym_tab.count(s);
  }

  SymEntry get_sym(const std::string &s) {
    assert_exists(s);
    return _sym_tab[s];
  }

  int get_int(std::string &s) {
    assert_exists(s);
    if (_sym_tab[s].type != SymEntry::SYM_INT) {
      std::cerr << "symbol \"" + s + "\" is not an int\"";
      assert(0);
    }
    return _sym_tab[s].data.i;
  }

  int get_float(std::string &s) {
    assert_exists(s);
    if (_sym_tab[s].type != SymEntry::SYM_DUB) {
      std::cerr << "symbol \"" + s + "\" is not a double\"";
      assert(0);
    }
    return _sym_tab[s].data.d;
  }

private:
  std::map<std::string, SymEntry> _sym_tab;
};

struct GroupProp {
  bool is_temporal = false;

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);
};

class SSDfg {
public:
  SSDfg();

  SSDfg(std::string filename);

  ~SSDfg() {
  }

  void remap(int num_HW_FU);

  bool remappingNeeded();

  void rememberDummies(std::set<SSDfgOutput *> d);

  void removeDummies();

  void reset_simulation_state();

  static void order_insts(SSDfgInst *inst,
                          std::set<SSDfgInst *> &done_nodes,         //done insts
                          std::vector<SSDfgInst *> &ordered_insts);

  std::vector<SSDfgInst *> &ordered_insts() {
    if (_orderedInsts.size() == 0) {
      std::set<SSDfgInst *> done_nodes;
      for (SSDfgOutput *out : _outputs) {
        if (SSDfgInst *producing_node = out->out_inst()) {
          order_insts(producing_node, done_nodes, _orderedInsts);
        }
      }
    }
    return _orderedInsts;
  }

  void printGraphviz(std::ostream &os, Schedule *sched = nullptr);

  //void printEmuDFG(std::ostream& os, std::string dfg_name);
  void printGraphviz(const char *fname, Schedule *sched = nullptr) {
    std::ofstream os(fname);
    assert(os.good());
    printGraphviz(os, sched);
    os.flush();
  }

  void start_new_dfg_group();

  void set_pragma(std::string &c, std::string &s);

  void printGams(std::ostream &os, std::unordered_map<std::string, SSDfgNode *> &,
                 std::unordered_map<std::string, SSDfgEdge *> &,
                 std::unordered_map<std::string, SSDfgVec *> &);

  void printPortCompatibilityWith(std::ostream &os, SS_CONFIG::SSModel *ssModel);


  void addInst(SSDfgInst *inst) {
    _insts.push_back(inst);
    _nodes.push_back(inst);
  }

  // remove instruction from nodes and insts
  void removeInst(SSDfgInst *inst) {
    _insts.erase(std::remove(_insts.begin(), _insts.end(), inst), _insts.end());
    _nodes.erase(std::remove(_nodes.begin(), _nodes.end(), inst), _nodes.end());
  }

  //Just for adding single input without keeping track of name/sym-table
  void addInput(SSDfgInput *input) {
    _inputs.push_back(input);
    _nodes.push_back(input);
  }

  void addOutput(SSDfgOutput *output) {
    _outputs.push_back(output);
    _nodes.push_back(output);
  }

void addVecOutput(const std::string &name, int len, SymTab &syms, int width) {
   int n = std::max(1, len);
   int slice = 64 / width;
   int t = ceil(n / float(slice));
   SSDfgVecOutput *vec_output = new SSDfgVecOutput(t, name, (int) _vecOutputs.size(), this);
   insert_vec_out(vec_output);
   vec_output->set_port_width(width);
   vec_output->set_vp_len(n);
   int left_len = 0;
   for (int i = 0, cnt = 0; i < n; i += slice) {
     SSDfgOutput *dfg_out = new SSDfgOutput(this, name + "_out", vec_output);

     left_len = slice;
     if(n-i>0) {
       left_len = std::min(n-i,slice);
     }

     addOutput(dfg_out);
     vec_output->addOutput(dfg_out);

     for (int j = 0; j < left_len*width; j += width) {
       std::stringstream ss;
       ss << name;
       if (len)
         ss << cnt++;
       auto sym = syms.get_sym(ss.str());
       // std::cout << ss.str() << " ";
       int num_entries = (int) sym.edge_entries->size();
       assert(num_entries > 0 && num_entries <= 16);

       for (auto edge_entry : *sym.edge_entries) {
         SSDfgNode *out_node = edge_entry.node;
         connect(out_node, dfg_out, 0, SSDfgEdge::data, edge_entry.l, edge_entry.r);
       }
     }
   }
 }

 void addVecInput(const std::string &name, int len, SymTab &syms, int width) {
   int n = std::max(1, len);
   int slice = 64 / width;
   int t = ceil(n / float(slice));
   // std::cout << "t: " << t << "\n";
   // std::cout << "port_width: " << width << "\n";
   auto *vec_input = new SSDfgVecInput(t, name, (int) _vecInputs.size(), this);
   insert_vec_in(vec_input);
   vec_input->set_port_width(width);
   vec_input->set_vp_len(n);
   int left_len=0;


   for (int i = 0, cnt = 0; i < n; i += slice) {
     auto *dfg_in = new SSDfgInput(this, name, vec_input);

     left_len = slice;
     if(n-i>0) {
       left_len = std::min(n-i,slice);
     }

     addInput(dfg_in);
     vec_input->addInput(dfg_in);
     for (int j = 0; j < left_len*width; j += width) {
       std::stringstream ss;
       ss << name;
       if (len)
         ss << cnt++;
       SymEntry entry(dfg_in, j, j + width - 1);
       syms.set(ss.str(), entry);
     }
   }
 }


  SSDfgEdge *connect(SSDfgNode *orig, SSDfgNode *dest, int slot,
                      SSDfgEdge::EdgeType etype, int l = 0, int r = 63);


  void disconnect(SSDfgNode *orig, SSDfgNode *dest);

  SymEntry create_inst(std::string opcode, std::vector<SymEntry> &args);

  typedef std::vector<SSDfgOutput *>::const_iterator const_output_iterator;

  const std::vector<SSDfgNode*> &nodes() { return _nodes; }

  const std::vector<SSDfgInst *> &inst_vec() { return _insts; }

  const std::vector<SSDfgInput*> &inputs() { return _inputs; }

  const std::vector<SSDfgOutput*> &outputs() { return _outputs; }

  int num_vec_input() { return _vecInputs.size(); }

  int num_vec_output() { return _vecOutputs.size(); }

  void insert_vec_in(SSDfgVecInput *in) {
    _vecInputs.push_back(in);
    // add to the group we are creating right now
    // std::cout << "It does come in inserting vec_in" << "\n";
    _vecInputGroups.back().push_back(in);
  }

  void insert_vec_out(SSDfgVecOutput *out) {
    _vecOutputs.push_back(out);
    _vecOutputGroups.back().push_back(out);
  }

  void insert_vec_in_group(SSDfgVecInput *in, unsigned group) {
    _vecInputs.push_back(in);
    if (_vecInputGroups.size() <= group) {
      _vecInputGroups.resize(group + 1);
    }
    _vecInputGroups[group].push_back(in);
  }

  void insert_vec_out_group(SSDfgVecOutput *out, unsigned group) {
    _vecOutputs.push_back(out);
    if (_vecOutputGroups.size() <= group) {
      _vecOutputGroups.resize(group + 1);
    }
    _vecOutputGroups[group].push_back(out);
  }

  int find_group_for_vec(SSDfgVecInput *in) {
    for (unsigned i = 0; i < _vecInputGroups.size(); ++i) {
      for (SSDfgVecInput *v : _vecInputGroups[i]) {
        if (v == in) {
          return i;
        }
      }
    }
    assert(0 && "Vec Input not found");
  }

  int find_group_for_vec(SSDfgVecOutput *out) {
    for (unsigned i = 0; i < _vecOutputGroups.size(); ++i) {
      for (SSDfgVecOutput *v : _vecOutputGroups[i]) {
        if (v == out) {
          return i;
        }
      }
    }
    assert(0 && "Vec Output not found");
  }

//------------------------------
/*
 int find_vec_for_scalar(SSDfgNode* in, int &index) {
      for(unsigned int i =0; i < _vecInputs.size(); ++i) {
          SSDfgVecInput* vec_in = _vecInputs[i];
          for(unsigned int j=0; j<vec_in->num_inputs(); ++j) {
              SSDfgNode* v = dynamic_cast<SSDfgNode*>(_vecInputs[i]->getInput(j));
              // std::cout << "Vector_id: " << i << " and v: " << v << " and in: " << in << "\n";
              if(v == in) {
                 index=j;
                 return i;
              }
          }
    }
      assert(0 && "Scalar input not found\n");
}

SSDfgVecInput* get_vector_input(int i){
    return _vecInputs[i];
}
*/
  //------------------


  SSDfgVecInput *vec_in(int i) { return _vecInputs[i]; }

  SSDfgVecOutput *vec_out(int i) { return _vecOutputs[i]; }

  std::vector<SSDfgVecInput *> &vec_in_group(int i) { return _vecInputGroups[i]; }

  std::vector<SSDfgVecOutput *> &vec_out_group(int i) { return _vecOutputGroups[i]; }

  GroupProp &group_prop(int i) { return _groupProps[i]; }

  int num_groups() { return _vecInputGroups.size(); }

  void sort_vec_in() {
    sort(_vecInputs.begin(), _vecInputs.end(), [](SSDfgVecInput *&left, SSDfgVecInput *&right) {
      return left->inputs().size() > right->inputs().size();
    });
  }

  void sort_vec_out() {
    sort(_vecOutputs.begin(), _vecOutputs.end(), [](SSDfgVecOutput *&left, SSDfgVecOutput *&right) {
      return left->outputs().size() > right->outputs().size();
    });
  }

  int compute(bool print, bool verif, int group);  //atomically compute
  int maxGroupThroughput(int group);

  void instsForGroup(int g, std::vector<SSDfgInst *> &insts);


  // --- New Cycle-by-cycle interface for more advanced CGRA -----------------

  double count_starving_nodes() {
    double count = 0;
    double num_unique_dfg_nodes = 0;
    for(auto it : _vecInputs) {
      SSDfgVecInput vec_in = *it;
      for(auto elem : vec_in.inputs()) { // each scalar node
        // for(auto node : elem->uses()) {
        SSDfgNode *node = elem->first_use(); // FIXME: how does it work for dgra
        num_unique_dfg_nodes += 1/node->num_inc();
        if(node->num_inputs_ready()) { count += 1/node->num_inc(); }
        // }
      }
    }
    assert(num_unique_dfg_nodes>=0);
    if(num_unique_dfg_nodes==0) return 0; // What is this case? none nodes
    return count/num_unique_dfg_nodes;
  }



  //Simulator pushes data to vector given by vector_id
  bool push_vector(SSDfgVecInput *vec_in, std::vector<uint64_t> data, std::vector<bool> valid, bool print, bool verif) {
    if(data.size() != vec_in->get_vp_len()) {
      std::cout << "DATA FROM GEM5: " << data.size() << " VEC VP SIZE: " << vec_in->get_vp_len() << "\n";
    }
    assert(data.size() == vec_in->get_vp_len() && "insufficient data available");
    // assert(data.size() == vec_in->length() && "insufficient data available");
    int npart = 64/vec_in->get_port_width();
    int x = static_cast<int>(vec_in->get_vp_len());
    // int x = static_cast<int>(vec_in->length());
    uint64_t val=0;

    for (int i = 0; i < (int)vec_in->inputs().size(); ++i) {
      int n_times = std::min(npart, x-i*npart); 
      for(int j = n_times-1+i*npart; j >= i*npart; --j) { 
        val = data[j] | (val << vec_in->get_port_width());
      }
      SSDfgInput *ss_node = vec_in->inputs()[i];
      ss_node->set_node(val, valid[i], true, print, verif);
      val = 0;
    }
    return true;
  }

  // check if some value present at input node or if
  // there is some backpressure or invalid value
  bool can_push_input(SSDfgVecInput *vec_in) {
    for (auto elem : vec_in->inputs())
      if (elem->get_avail())
        return false;
    return true;
  }

  //Simulator would like to pop size elements from vector port (vector_id)
  bool can_pop_output(SSDfgVecOutput *vec_out, unsigned int len) {

    assert(len > 0 && "Cannot pop 0 length output\n");
    if(vec_out->outputs().size() != len) {
      std::cout << "DATA FROM GEM5: " << len << " VEC VP SIZE: " << vec_out->outputs().size() << "\n";
    }
    assert(vec_out->outputs().size() == len
           && "asked for different number of outputs than the supposed length\n");

    size_t ready_outputs = 0;
    for (auto elem: vec_out->outputs()) {
      SSDfgOperand &operand = elem->first_operand();
      if (!operand.is_buffer_empty()) {
        ready_outputs++;
      }
    }
    // std::cout << "ready outputs: " << ready_outputs << " len: " << len << "\n";
    if (ready_outputs == len) {
      return true;
    } else {
      return false;
    }
  }

  //Simulator grabs size elements from vector port (vector_id)
  //assertion failure on insufficient size
  void pop_vector_output(SSDfgVecOutput *vec_out, std::vector<uint64_t> &data,
                         std::vector<bool> &data_valid, unsigned int len, bool print, bool verif) {
    assert(vec_out->outputs().size() == len && "insufficient output available\n");

    // we don't need discard now!
    for (auto elem: vec_out->outputs()) {
      SSDfgOperand &operand = elem->first_operand();
      data.push_back(operand.get_buffer_val());
      data_valid.push_back(operand.get_buffer_valid()); // I can read different validity here
      operand.pop_buffer_val(print, verif);
    }
    // Insufficient output size
    // assert(data.size()==len);
  }

  void push_transient(SSDfgNode *n, uint64_t v, bool valid, bool avail, int cycle) {
    struct cycle_result *temp = new cycle_result(n, v, valid, avail);
    transient_values[(cycle + cur_node_ptr) % get_max_lat()].push_back(temp);
  }

  void push_buf_transient(SSDfgEdge *e, bool is_dummy, int cycle) {
    struct buffer_pop_info *temp = new buffer_pop_info(e, is_dummy);
    buf_transient_values[(cycle + cur_buf_ptr) % get_max_lat()].push_back(temp);
  }

  int cycle(bool print, bool verif) {
    // int num_computed=0;
    for (auto it = buf_transient_values[cur_buf_ptr].begin(); it != buf_transient_values[cur_buf_ptr].end();) {
      // set the values
      buffer_pop_info *temp = *it;
      SSDfgEdge *e = temp->e;
      e->pop_buffer_val(print, verif);

      it = buf_transient_values[cur_buf_ptr].erase(it);
      // buf_transient_values[cur_buf_ptr].erase(it);
      // it--;
    }

    for (auto it = transient_values[cur_node_ptr].begin(); it != transient_values[cur_node_ptr].end();) {
      struct cycle_result *temp = *it;
      SSDfgNode *ss_node = temp->n;
      ss_node->set_node(temp->val, temp->valid, temp->avail, print, verif);
      it = transient_values[cur_buf_ptr].erase(it);
    }

    std::unordered_set<int> nodes_complete;

    for (auto I = _ready_nodes.begin(); I != _ready_nodes.end();) {
      SSDfgNode *n = *I;

      int node_id = n->node_id();
      bool should_fire = (node_id == -1) ||
                         (nodes_complete.count(node_id) == 0);

      unsigned inst_throughput = 1;

      //If inst_throughput cycles is great than 1, lets mark the throughput
      //make sure nobody else can also schedule a complex instruction during
      //that period
      if (should_fire && node_id != -1) {
        if (SSDfgInst *inst = dynamic_cast<SSDfgInst *>(n)) {
          inst_throughput = inst_thr(inst->inst());
          if (inst_throughput > 1) {
            if (_complex_fu_free_cycle[node_id] > _cur_cycle) {
              should_fire = false;
            }
          }
        }
      }

      if (should_fire && n->get_avail() == 0) {
        n->compute_backcgra(print, verif);
        I = _ready_nodes.erase(I);
        nodes_complete.insert(node_id);

        if (node_id != -1 && inst_throughput > 1) {
          _complex_fu_free_cycle[node_id] = _cur_cycle + inst_throughput;
        }

      } else {
        ++I;
      }
    }

    cur_buf_ptr = (cur_buf_ptr + 1) % get_max_lat();
    cur_node_ptr = (cur_node_ptr + 1) % get_max_lat();
    _cur_cycle = _cur_cycle + 1;
    int temp = _total_dyn_insts;
    _total_dyn_insts = 0;
    return temp;
  }

// ---------------------------------------------------------------------------

  std::set<SSDfgOutput *> getDummiesOutputs() { return dummiesOutputs; }

  void calc_minLats();

  void set_dbg_stream(std::ostream *dbg_stream) { _dbg_stream = dbg_stream; }

  std::ostream &dbg_stream() { return *_dbg_stream; }

  void check_for_errors();

  void inc_total_dyn_insts() { _total_dyn_insts++; }

  int total_dyn_insts() { return _total_dyn_insts; }

  int get_max_lat() { return MAX_LAT; }

  int num_node_ids() { return _num_node_ids; }

  std::vector<SSDfgEdge*>& edges() {return _edges;}

  int num_edge_ids() { return _num_edge_ids; }

  int inc_node_id() { return _num_node_ids++; }

  int inc_edge_id() { return _num_edge_ids++; }

  void push_ready_node(SSDfgNode *node) { _ready_nodes.push_back(node); }

private:
  // to keep track of number of cycles---------------------
  struct cycle_result {
    SSDfgNode *n;
    uint64_t val;
    bool valid;
    bool avail;

    cycle_result(SSDfgNode *node, uint64_t value, bool valid_in, bool a) {
      n = node;
      val = value;
      valid = valid_in;
      avail = a;
    }
  };

  struct buffer_pop_info {
    SSDfgEdge *e;
    bool is_dummy;

    buffer_pop_info(SSDfgEdge *edge, bool dummy) {
      e = edge;
      is_dummy = dummy;
    }
  };

  std::vector<SSDfgNode *> _ready_nodes;

  int MAX_LAT = 1000;
  std::list<struct cycle_result *> transient_values[1000];
  int cur_node_ptr = 0;
  int cur_buf_ptr = 0;
  uint64_t _cur_cycle = 0;

  std::list<struct buffer_pop_info *> buf_transient_values[1000];

  std::unordered_map<int, uint64_t> _complex_fu_free_cycle;

  //stats
  int _total_dyn_insts = 0;

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);

  std::vector<SSDfgNode *> _nodes;

  //redundant storage:
  std::vector<SSDfgInst *> _insts;
  std::vector<SSDfgInput *> _inputs;
  std::vector<SSDfgOutput *> _outputs;

  std::vector<SSDfgInst *> _orderedInsts;
  std::vector<std::vector<SSDfgInst *>> _orderedInstsGroup;

  std::vector<SSDfgVecInput *> _vecInputs;
  std::vector<SSDfgVecOutput *> _vecOutputs;

  std::vector<SSDfgEdge *> _edges;

  std::map<std::pair<SSDfgNode *, SSDfgNode *>, SSDfgEdge *> removed_edges;

  std::vector<std::vector<SSDfgVecInput *>> _vecInputGroups;
  std::vector<std::vector<SSDfgVecOutput *>> _vecOutputGroups;
  std::vector<GroupProp> _groupProps;

  int _num_node_ids = 0;
  int _num_edge_ids = 0;

  //Dummy Stuffs:
  std::map<SSDfgOutput *, SSDfgInst *> dummy_map;
  std::map<SSDfgNode *, int> dummys_per_port;
  std::set<SSDfgInst *> dummies;
  std::set<SSDfgOutput *> dummiesOutputs;

  std::ostream *_dbg_stream;
};


#endif
