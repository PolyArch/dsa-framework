#ifndef __SS__SCHEDULE_H__
#define __SS__SCHEDULE_H__

#include <map>
#include <unordered_set>
#include "model.h"
#include "sub_model.h"
#include "ssdfg.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#include "bitslice.h"
#include "config_defs.h"
#include "color_mapper.h"
#include "limits.h"

//Experimental Boost Stuffs
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/functional/hash.hpp>


using namespace SS_CONFIG;

//How do you choose which switch in each row and FU to pass ??
//struct sw_config {
//  unsigned pred:2;
//  unsigned opcode:5;
//  unsigned fu_in1:2;    //cfg for 1st input of FU
//  unsigned fu_in2:2;    //cfg for 2nd input of FU
//  unsigned fu_out:2;    //cfg for 3rd input of FU
//  unsigned  sw_s:3;     //select line for output muxes
//  unsigned sw_se:3;
//  unsigned  sw_e:3;
//  unsigned sw_ne:3;
//  unsigned  sw_n:3;
//  unsigned sw_nw:3;
//  unsigned  sw_w:3;
//  unsigned sw_sw:3;
//  unsigned   row:2;  //can address only 4 rows -- need to update it
//};

#define MAX_SCHED_LAT 1000000

class Schedule {
public:
  Schedule() {}

  //Read in schedule (both ssmodel, ssdfg, and schedule from file)
  Schedule(SSModel *model, SSDfg *dfg) : _ssModel(model), _ssDFG(dfg) {
    allocate_space();
  }

  Schedule(SSModel *model) : _ssModel(model), _ssDFG(nullptr) {
    allocate_space();
  }

  constexpr static const float gvsf = 4.0f;

  void printGraphviz(const char *name);

  void printFUGraphviz(std::ofstream &ofs, ssfu *fu);

  void printInputGraphviz(std::ofstream &ofs, ssnode *fu);

  void printMvnGraphviz(std::ofstream &ofs, ssnode *node);

  void printMelGraphviz(std::ofstream &ofs, ssnode *node);

  void printOutputGraphviz(std::ofstream &ofs, ssnode *node);

  void printSwitchGraphviz(std::ofstream &ofs, ssswitch *sw);

  //Scheduling Interface:
  bool spilled(SSDfgNode *);

  //Old Interface:

  int getPortFor(SSDfgNode *);

  void printConfigHeader(std::ostream &, std::string cfg_name, bool cheat = true);

  void printConfigBits(std::ostream &os, std::string cfg_name);

  void printConfigCheat(std::ostream &os, std::string cfg_name);

  void printConfigVerif(std::ostream &os);

  //Rest of Stuff
  SSDfg *ssdfg() const { return _ssDFG; }


  std::map<SS_CONFIG::sslink *, SS_CONFIG::sslink *> &link_map_for_sw(ssswitch *sssw) {
    return _assignSwitch[sssw];
  }

  //check if inlink
  sslink *get_switch_in_link(ssswitch *sssw, sslink *out_link) {
    auto iter = _assignSwitch[sssw].find(out_link);
    if (iter == _assignSwitch[sssw].end()) {
      return nullptr;
    } else {
      return _assignSwitch[sssw][out_link];
    }
  }

  bool have_switch_links() {
    return !_assignSwitch.empty();
  }

  //For a switch, assign the outlink to inlink
  void assign_switch(ssswitch *sssw, sslink *slink, sslink *slink_out) {
    assert(sssw);
    assert(slink);
    assert(slink_out);
    _assignSwitch[sssw][slink_out] = slink;     //out to in for a sw
  }

  void assign_lat(SSDfgVec *vec, int lat) {
    _vecProp[vec].lat = lat;
  }

  void assign_lat(SSDfgNode *dfgnode, int lat) {
    _vertexProp[dfgnode->id()].lat = lat;
  }

  int latOf(SSDfgNode *dfgnode) {
    return _vertexProp[dfgnode->id()].lat;
  }

  void assign_lat_bounds(SSDfgVec *vec, int min, int max) {
    auto &vec_prop = _vecProp[vec];
    vec_prop.min_lat = min;
    vec_prop.max_lat = max;
  }

  void assign_lat_bounds(SSDfgNode *dfgnode, int min, int max) {
    auto &vertex_prop = _vertexProp[dfgnode->id()];
    vertex_prop.min_lat = min;
    vertex_prop.max_lat = max;
  }

  std::pair<int, int> lat_bounds(SSDfgNode *dfgnode) {
    auto &vertex_prop = _vertexProp[dfgnode->id()];
    return std::make_pair(vertex_prop.min_lat, vertex_prop.max_lat);
  }

  void assign_edge_pt(SSDfgEdge *edge, std::pair<int, ssnode*> pt) {
    if ((int) _edgeProp.size() <= edge->id()) {
      _edgeProp.resize(edge->id() + 1);
    }
    add_passthrough_node(pt.second);
    _edgeProp[edge->id()].passthroughs.insert(pt);
  }

  int groupMismatch(int g) {
    return _groupMismatch[g];
  }

  int violation() { return _totalViolation; }

  void add_violation(int violation) {
    _totalViolation += violation;
    _max_lat_mis = std::max(_max_lat_mis, violation);
  }

  int vioOf(SSDfgNode *n) { return _vertexProp[n->id()].vio; }

  void record_violation(SSDfgNode *n, int violation) {
    _vertexProp[n->id()].vio = violation;
  }


  //Assign the ssnode to dfgnode and vice verse
  void assign_node(SSDfgNode *dfgnode, std::pair<int, ssnode *> assigned) {
    int vid = dfgnode->id();
    if (vid >= (int) _vertexProp.size()) {
      _vertexProp.resize(vid + 1);
    }
    auto snode = assigned.second;

    assert(_vertexProp[vid].node == nullptr || _vertexProp[vid].node == snode);
    if (_vertexProp[vid].node == snode) return;

    _vertexProp[vid].node = snode;
    _vertexProp[vid].idx = assigned.first;
    _vertexProp[vid].width = dfgnode->bitwidth();

    assert(dfgnode->type() < SSDfgNode::V_NUM_TYPES);
    _num_mapped[dfgnode->type()]++;
    assert(_num_mapped[SSDfgNode::V_INPUT] <= _ssDFG->inputs().size());
    assert(_num_mapped[SSDfgNode::V_OUTPUT] <= _ssDFG->outputs().size());
    assert(_num_mapped[SSDfgNode::V_INST] <= _ssDFG->inst_vec().size());

    //std::cout << "node " << dfgnode->name() << " assigned to "
    //          << snode->name() << "\n";
    assert(dfgnode);

    assert(snode->id() < (int) _nodeProp.size());
    _nodeProp[snode->id()].vertices.insert(std::make_pair(assigned.first, dfgnode));
  }

  void calc_out_lat() {
    for (int i = 0; i < _ssDFG->num_vec_output(); ++i) {
      calc_out_vport_lat(_ssDFG->vec_out(i));
    }
  }

  void calc_out_vport_lat(SSDfgVecOutput *dfgvec_out) {
    int max_lat = 0;
    for (auto dfgout : dfgvec_out->outputs()) {
      //ssnode* out_ssnode = locationOf(dfgout).first;
      int lat_of_out_ssnode = _vertexProp[dfgout->id()].lat;
      std::cout << dfgvec_out->gamsName() << " lat:" << lat_of_out_ssnode << "\n";
      max_lat = std::max(max_lat, lat_of_out_ssnode);
    }
    //std::cout << "max lat: " << max_lat<< "\n";
    _vecProp[dfgvec_out].lat = max_lat;
  }

  //Get the mask for a software vector
  std::vector<bool> maskOf(SSDfgVec *dfgvec) {
    auto &vp = _vecProp[dfgvec];
    return vp.mask;
  }

  //Get the software vector for a hardware vector port identifier
  SSDfgVec *vportOf(std::pair<bool, int> pn) {
    if (_assignVPort.count(pn)) {
      return _assignVPort[pn];
    } else {
      return nullptr;
    }
  }

  //vector to port num
  //true for input
  void assign_vport(SSDfgVec *dfgvec, std::pair<bool, int> pn,
                    std::vector<bool> mask) {
    /*std::cout << (pn.first ? "input" : "output" )
              << " vector port" << pn.second
              << " assigned to " << dfgvec->gamsName() << "\n";*/
    _assignVPort[pn] = dfgvec;
    auto &vp = _vecProp[dfgvec];
    vp.vport = pn;
    vp.mask = mask;

    //for assert
    auto &io_interf = _ssModel->subModel()->io_interf();
    if (pn.first) {
      assert(mask.size() == io_interf.in_vports[pn.second]->size());
    } else {
      assert(mask.size() == io_interf.out_vports[pn.second]->size());
    }
  }

  bool vecMapped(SSDfgVec *p) {
    return _vecProp[p].vport.second != -1;
  }

  std::pair<bool, int> vecPortOf(SSDfgVec *p) {
    return _vecProp[p].vport;
  }

  //unassign all the input nodes and vector
  void unassign_input_vec(SSDfgVecInput *dfgvec) {
    for (auto in : dfgvec->inputs()) {
      unassign_dfgnode(in);
    }

    auto &vp = _vecProp[dfgvec];
    std::pair<bool, int> pn = vp.vport;
    _assignVPort.erase(pn);
    _vecProp.erase(dfgvec);
  }

  //unassign all the input nodes and vector
  void unassign_output_vec(SSDfgVecOutput *dfgvec) {
    for (auto out : dfgvec->outputs()) {
      unassign_dfgnode(out);
    }

    auto &vp = _vecProp[dfgvec];
    std::pair<bool, int> pn = vp.vport;
    _assignVPort.erase(pn);
    _vecProp.erase(dfgvec);
  }

  void unassign_edge(SSDfgEdge *edge) {
    auto &ep = _edgeProp[edge->id()];

    //std::cout << "unassign: " <<edge->name() << "\n";

    _edge_links_mapped -= ep.links.size();

    //Remove all the edges for each of links
    for (auto &link : ep.links) {
      auto &lp = _linkProp[link.second->id()];
      lp.slots[link.first].edges.erase(edge);
      if (lp.slots[link.first].edges.empty()) {
        _links_mapped--;
        assert(_links_mapped >= 0);
      }
    }

    //Remove all passthroughs associated with this edge
    for (auto &pt : ep.passthroughs) {
      auto &np = _nodeProp[pt.second->id()];
      np.num_passthroughs -= 1; //take one passthrough edge away
      assert(np.num_passthroughs >= 0);
    }

    _edgeProp[edge->id()].reset();
  }

  //Delete all scheduling data associated with dfgnode, including its
  //mapped locations, and mapping information and metadata for edges
  void unassign_dfgnode(SSDfgNode *dfgnode) {
    for (auto edge : dfgnode->in_edges()) {
      unassign_edge(edge);
    }
    for (auto edge : dfgnode->uses()) {
      unassign_edge(edge);
    }

    auto &vp = _vertexProp[dfgnode->id()];
    ssnode *node = vp.node;
    if (node) {
      _num_mapped[dfgnode->type()]--;
      vp.node = nullptr;
      auto &np_vs = _nodeProp[node->id()].vertices;
      for (auto iter = np_vs.begin(), end = np_vs.end(); iter != end; ++iter)
        if (iter->second == dfgnode) {
          _nodeProp[node->id()].vertices.erase(iter);
          break;
        }
    }
  }

  std::unordered_set<SSDfgEdge *> &edge_list(int slot, sslink *link) {
    return _linkProp[link->id()].slots[slot].edges;
  }

  void print_all_mapped() {
    std::cout << "Vertices: ";
    //TODO: can't get vertex/edge from id, need to modify dfg to maintain
    for (unsigned i = 0; i < _vertexProp.size(); ++i) {
      auto &v = _vertexProp[i];
      if (v.node) {
        std::cout << i << "->" << v.node->name() << " ";
      }
    }
    std::cout << "\nEdges: ";
    for (unsigned i = 0; i < _edgeProp.size(); ++i) {
      auto &e = _edgeProp[i];
      if (e.links.size()) {
        std::cout << i << " ";
      }
    }
    std::cout << "\nVec: ";
    for (auto &v : _vecProp) {
      if (v.first && vecMapped(v.first)) {
        std::cout << v.first->name() << " ";
      }
    }
    std::cout << "\n";
  }

  //pdf edge to sslink
  void assign_edgelink(SSDfgEdge *dfgedge, int slot, sslink * slink) {
    assert(slink);
    assert(dfgedge);
    //assert(_assignLink[slink]==nullptr || _assignLink[slink] == dfgedge->def());
    //std::cout << dfgedge->name() << " assigned to "
    //          << slink->name() << "\n";

    _edge_links_mapped++;
    auto &lp = _linkProp[slink->id()].slots[slot];
    if (lp.edges.empty())
      _links_mapped++;
    lp.edges.insert(dfgedge);


    if ((int) _edgeProp.size() <= dfgedge->id()) {
      _edgeProp.resize(dfgedge->id() + 1);
    }
    _edgeProp[dfgedge->id()].links.insert(std::make_pair(slot, slink));
  }

  //void print_links(SSDfgEdge* dfgedge) {
  //  for(auto& i : _assignLinkEdge[dfgedge]) {
  //    cout << i->name() << " ";
  //  }
  //  cout << "\n";
  //}

  int link_count(SSDfgEdge *dfgedge) {
    return _edgeProp[dfgedge->id()].links.size();
  }

  std::unordered_set<std::pair<int, sslink*>, boost::hash<std::pair<int, sslink*>>> &links_of(SSDfgEdge *edge) {
    auto &ep = _edgeProp[edge->id()];
    return ep.links;
  }

  void setLatOfLink(std::pair<int, sslink*> link, int l) { _linkProp[link.second->id()].slots[link.first].lat = l; }

  int latOfLink(std::pair<int, sslink*> link) { return _linkProp[link.second->id()].slots[link.first].lat; }

  bool linkAssigned(int slot, sslink* link) {
    return !_linkProp[link->id()].slots[slot].edges.empty();
  }

  //return cost_to_route
  //0: free
  //1: empty
  //>2: already there
  int temporal_cost(std::pair<int, sslink *> link, SSDfgNode *node) {
    assert(link.second);
    //Check all slots will be occupied empty.
    bool empty = true;
    if (!_linkProp[link.second->id()].slots[link.first].edges.empty())
      empty = false;
    if (empty)
      return 1;
    for (auto elem : _linkProp[link.second->id()].slots[link.first].edges)
      if (elem->def() == node)
        return 0;
    return _linkProp[link.second->id()].slots[link.first].edges.size();
  }

  bool input_matching_vector(SSDfgNode *node, SSDfgVecInput *in_v) {
    auto *input = dynamic_cast<SSDfgInput*>(node);
    if (input) {
      if (input->input_vec() == in_v) {
        return true;
      }
    }
    return false;
  }

  bool output_matching_vector(SSDfgNode *node, SSDfgVecOutput *out_v) {
    for (auto elem : node->uses()) {
      auto *output = dynamic_cast<SSDfgOutput *>(elem->use());
      if (output && output->output_vec() == out_v) {
        return true;
      }
    }
    return false;
  }

  int temporal_cost_in(sslink *link, SSDfgVecInput *in_v) {
    assert(link);
    auto &vec = _linkProp[link->id()].slots[0].edges;
    if (vec.empty()) return 1;
    for (auto *elem : vec) {
      if (input_matching_vector(elem->def(), in_v))
        return 0;
    }
    return 2;
  }

  //what a confusing function
  int temporal_cost_out(std::pair<int, sslink*> link, SSDfgNode *node, SSDfgVecOutput *out_v) {
    assert(link.second);
    auto &vec = _linkProp[link.second->id()].slots[link.first].edges;
    if (vec.empty()) return 1;
    //It's free if the node is the same, or one of the use vectors is the same.
    for (auto elem : vec) {
      if (elem->def() == node)
        return 0;
      if (output_matching_vector(elem->def(), out_v))
        return 0;
    }
    return 2;
  }

  //find first node for
  SSDfgNode *dfgNodeOf(int slot, sslink* link) {
    assert(link);
    auto &vec = _linkProp[link->id()].slots[slot].edges;
    return vec.empty() ? nullptr : (*vec.begin())->def();
  }

  size_t thingsAssigned(ssnode *node) {
    auto &np = _nodeProp[node->id()];
    return np.vertices.size() + np.num_passthroughs;
  }

  bool nodeAssigned(ssnode *node) {
    return !_nodeProp[node->id()].vertices.empty();
  }

  //find first node for
  SSDfgNode *dfgNodeOf(ssnode *node) {
    auto &vec = _nodeProp[node->id()].vertices;
    return vec.size() == 0 ? nullptr : vec.begin()->second;
  }

  //Find all the nodes for the given ssnode
  std::unordered_set<std::pair<int, SSDfgNode *>,
          boost::hash<std::pair<int, SSDfgNode *>>> dfg_nodes_of(ssnode *node) {
    auto &vec = _nodeProp[node->id()].vertices;
    return vec;
  }

  ssnode *locationOf(SSDfgNode *dfgnode) {
    return _vertexProp[dfgnode->id()].node;
  }

  std::pair<int, ssnode *> location_of(SSDfgNode *dfgnode) {
    return std::make_pair(_vertexProp[dfgnode->id()].idx, _vertexProp[dfgnode->id()].node);
  }

  bool is_scheduled(SSDfgNode *dfgnode) {
    return _vertexProp[dfgnode->id()].node != nullptr;
  }

  void stat_printOutputLatency();

  //typedef std::vector<sslink*>::const_iterator link_iterator;
  //link_iterator links_begin(SSDfgNode* n) {return _linksOf[n].begin();}
  //link_iterator links_end(SSDfgNode* n)   {return _linksOf[n].end();}


  //wide vector ports
  int numWidePorts() { return _wide_ports.size(); }

  std::vector<int> &widePort(int i) { return _wide_ports[i]; }

  void addWidePort(std::vector<int> &port) { _wide_ports.push_back(port); }


  SSModel *ssModel() { return _ssModel; }

  void iterativeFixLatency();

  std::vector<SSDfgInst *> ordered_non_temporal();

  void calcLatency(int &lat, int &latmis, bool warnMismatch = false);

  void cheapCalcLatency(int &lat, int &latmis, bool set_delay = false);

  void calcNodeLatency(SSDfgInst *, int &lat, int &latmis, bool set_delay = false);


  bool fixLatency(int &lat, int &latmis);

  bool fixLatency_fwd(int &lat, int &latmis);

  bool fixLatency_bwd();

  bool fixDelay(SSDfgOutput *dfgout, int ed, std::unordered_set<SSDfgNode *> &visited);

  void checkOutputMatch(int &latmis);

  void calcAssignEdgeLink_single(SSDfgNode *dfgnode);

  void calcAssignEdgeLink();

  void clearAll() {
    _totalViolation = 0;
    _assignSwitch.clear();
    _assignVPort.clear();
    _vecProp.clear();
    _vertexProp.clear();
    _edgeProp.clear();
    //
    _nodeProp.clear();
    _linkProp.clear();

    allocate_space();
  }

  //assign outlink to inlink for a switch
  //if the dfgnode is associated with the switch
  //whats is the outlink of the inlink from that node
  void xfer_link_to_switch() {
    using namespace SS_CONFIG;
    using namespace std;

    if (_assignSwitch.size() != 0) { //switches already assigned!
      return;
    }

    vector<vector<ssswitch*> > &switches = _ssModel->subModel()->switches();  //2d switches
    for (int i = 0; i < _ssModel->subModel()->sizex() + 1; ++i) {
      for (int j = 0; j < _ssModel->subModel()->sizey() + 1; ++j) {

        ssswitch *sssw = switches[i][j];

        for (auto &inlink: sssw->in_links()) {
          for (int slot_in = 0; slot_in < 8; ++slot_in) {
            if (linkAssigned(slot_in, inlink)) {
              //inlink to sw associated with a dfg node
              SSDfgNode *innode = dfgNodeOf(slot_in, inlink);

              for (auto outlink: sssw->out_links()) {
                for (int slot_out = 0; slot_out < 8; ++slot_out) {

                  //check if the dfgnode has same outlink and inlink
                  if (linkAssigned(slot_out, outlink) != 0 && innode == dfgNodeOf(slot_out, outlink)) {
                    _assignSwitch[sssw][outlink] = inlink;
                  }
                }
              }//end for out links

            }
          }
        }//end for sin links

      }//end for sizex
    }//end for sizey
  }

  //NOTE/WARN: interpretConfigBits creates a dfg object that should
  //be cleaned up later by the schedule object
  std::map<SS_CONFIG::ss_inst_t, int> interpretConfigBits(int size, uint64_t *bits);

  std::map<SS_CONFIG::ss_inst_t, int> interpretConfigBitsCheat(char *s);

  //TODO: Implement it to support MGRA
  //std::map<SS_CONFIG::ss_inst_t,int> interpretConfigBitsDedicated();

  void clear_ssdfg();

  void reset_simulation_state();

  bitslices<uint64_t> &slices() { return _bitslices; }

  void add_passthrough_node(ssnode *n) {
    assert(dfgNodeOf(n) == nullptr);

    _nodeProp[n->id()].num_passthroughs += 1; //add one to edges passing through
  }

  bool isPassthrough(ssnode *n) {
    return _nodeProp[n->id()].num_passthroughs > 0;
  }

  void print_bit_loc() {
    std::cout << "Primary Config\n";
    std::cout << "Row: " << ROW_LOC
              << ":" << ROW_LOC + ROW_BITS - 1 << "\n";
    std::cout << "Switches: " << SWITCH_LOC << ":"
              << SWITCH_LOC + SWITCH_BITS - 1 << "\n";
    std::cout << "FU Dir: " << FU_DIR_LOC << ":"
              << FU_DIR_LOC + FU_DIR_BITS - 1 << "\n";
    std::cout << "FU Pred Inv: " << FU_PRED_INV_LOC << ":"
              << FU_PRED_INV_LOC + FU_PRED_INV_BITS - 1 << "\n";
    std::cout << "Opcode: " << OPCODE_LOC << ":"
              << OPCODE_LOC + OPCODE_BITS - 1 << "\n";
    std::cout << "In Del.: " << IN_DELAY_LOC << ":"
              << IN_DELAY_LOC + IN_DELAY_BITS - 1 << "\n";
  }

  void set_edge_delay(int i, SSDfgEdge *e) { _edgeProp[e->id()].extra_lat = i; }

  int edge_delay(SSDfgEdge *e) { return _edgeProp[e->id()].extra_lat; }

  void set_link_order(int slot, sslink *l, int i) { _linkProp[l->id()].slots[slot].order = i; }

  int link_order(std::pair<int, sslink *> l) { return _linkProp[l.second->id()].slots[l.first].order; }

  struct LinkProp;

  std::vector<LinkProp> &link_prop() { return _linkProp; }

  size_t num_passthroughs(SSDfgEdge *e) {
    return _edgeProp[e->id()].passthroughs.size();
  }

  int max_lat() {
    assert(_max_lat != -1);
    return _max_lat;
  }

  int max_lat_mis() { return _max_lat_mis; }

  int decode_lat_mis() { return _decode_lat_mis; }

  void set_decode_lat_mis(int i) { _decode_lat_mis = i; }

  void reset_lat_bounds() {
    for (auto elem : _ssDFG->inputs()) {
      auto &vp = _vertexProp[elem->id()];
      vp.min_lat = 0;
      vp.max_lat = 0;
    }
    for (auto elem : _ssDFG->inst_vec()) {
      auto &vp = _vertexProp[elem->id()];
      vp.min_lat = 0;
      vp.max_lat = INT_MAX - 1000;
    }
    for (int i = 0; i < _ssDFG->num_vec_output(); i++) {
      SSDfgVecOutput *vec_out = _ssDFG->vec_out(i);
      auto &vecp = _vecProp[vec_out];
      vecp.min_lat = 0;
      vecp.max_lat = INT_MAX - 1000;
      for (auto dfgout: vec_out->outputs()) {
        auto &vp = _vertexProp[dfgout->id()];
        vp.min_lat = 0;
        vp.max_lat = INT_MAX - 1000;
      }
    }
  }

  void set_name(std::string s) { _name = s; }

  std::string name() { return _name; }

private:


  //called by reconstructSchedule to trace link assignment
  void tracePath(ssnode *, SSDfgNode *,
                 std::map<ssnode *, std::map<SwitchDir::DIR, SwitchDir::DIR>> &,
                 std::map<ssnode *, SSDfgNode *> &,
                 std::map<SSDfgNode *, std::vector<SwitchDir::DIR> > &);

  //helper for reconstructing Schedule
  //routemap -- for each ssnode - inlink and outlinks
  //dfgnode_for -- ssnode to dfgnode mapping
  //posMap -- for each dfgnode, vector of incoming dirs
  void reconstructSchedule(
          std::map<ssnode *, std::map<SwitchDir::DIR, SwitchDir::DIR>> &routemap,
          std::map<ssnode *, SSDfgNode *> &dfgnode_for,
          std::map<SSDfgNode *, std::vector<SwitchDir::DIR> > &posMap);


public:
  unsigned num_insts_mapped() { return _num_mapped[SSDfgNode::V_INST]; }

  unsigned num_inputs_mapped() { return _num_mapped[SSDfgNode::V_INPUT]; }

  unsigned num_outputs_mapped() { return _num_mapped[SSDfgNode::V_OUTPUT]; }

  unsigned num_mapped() {
    return _num_mapped[SSDfgNode::V_INST] +
           _num_mapped[SSDfgNode::V_INPUT] +
           _num_mapped[SSDfgNode::V_OUTPUT];
  }

  unsigned inputs_complete() { return num_inputs_mapped() == _ssDFG->inputs().size(); }

  unsigned outputs_complete() { return num_outputs_mapped() == _ssDFG->outputs().size(); }

  unsigned insts_complete() { return num_insts_mapped() == _ssDFG->inst_vec().size(); }

  bool isComplete() { return num_mapped() == _ssDFG->nodes().size(); }

  unsigned num_links_mapped() { return _links_mapped; }

  unsigned num_edge_links_mapped() { return _edge_links_mapped; }

  unsigned num_left() {
    int num = _ssDFG->nodes().size() - num_mapped();
    assert(num >= 0);
    return num;
  }

  void allocate_space() {
    if (_ssDFG) {
      _vertexProp.resize((size_t) _ssDFG->num_node_ids());
      _edgeProp.resize((size_t) _ssDFG->num_edge_ids());
    }
    if (_ssModel) {
      _nodeProp.resize((size_t) _ssModel->subModel()->node_list().size());
      _linkProp.resize((size_t) _ssModel->subModel()->link_list().size());
    }
  }

  int colorOf(SSDfgNode *n) { return _cm.colorOf(n); }

  void get_overprov(int& ovr, int& agg_ovr, int& max_util);

  struct VertexProp {
    int min_lat = 0, max_lat = 0, lat = 0, vio = 0;
    ssnode *node = nullptr;
    int width = -1, idx = -1;

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned version);
  };

  struct EdgeProp {
    int num_links = 0;
    int extra_lat = 0;
    std::unordered_set<std::pair<int, sslink*>, boost::hash<std::pair<int, sslink*>>> links;
    std::unordered_set<std::pair<int, ssnode*>, boost::hash<std::pair<int, ssnode*>>> passthroughs;

    void reset() {
      num_links = 0;
      extra_lat = 0;
      links.clear();
      passthroughs.clear();
    }

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned version);
  };

  struct NodeProp {
    int num_passthroughs = 0;
    std::unordered_set<std::pair<int, SSDfgNode *>, boost::hash<std::pair<int, SSDfgNode*>>> vertices;

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned version);
  };

  struct LinkProp {
    struct LinkSlot {
      int lat = 0, order = -1;
      std::unordered_set<SSDfgEdge *> edges;
      std::unordered_set<SSDfgNode *> nodes;
    };

    LinkSlot slots[8];

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned version);
  };

  struct VecProp {
    int min_lat = 0, max_lat = INT_MAX, lat = 0;
    std::pair<bool, int> vport = {false, -1};
    std::vector<bool> mask;

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned version);
  };

private:

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &ar, const unsigned version);

  std::string _name;

  //Private Data
  SSModel *_ssModel;
  SSDfg *_ssDFG;

  int _totalViolation = 0;
  int _max_lat = -1, _max_lat_mis = -1; //filled from interpretConfigBits + calcLatency

  unsigned _num_mapped[SSDfgNode::V_NUM_TYPES] = {0}; //init all to zero
  int _links_mapped = 0, _edge_links_mapped = 0;

  std::map<int, int> _groupMismatch;

  std::vector<std::vector<int> > _wide_ports;

  std::map<std::pair<bool, int>, SSDfgVec *> _assignVPort;

  std::unordered_map<SSDfgVec *, VecProp> _vecProp;
  std::vector<VertexProp> _vertexProp;
  std::vector<EdgeProp> _edgeProp;

  //vport prop missing datastructure, imiplicit pair for now
  std::vector<NodeProp> _nodeProp;
  std::vector<LinkProp> _linkProp;


  std::map<ssswitch *, std::map<sslink *, sslink *>> _assignSwitch; //out to in

  int _decode_lat_mis = 0;

  //CGRA-parsable Config Data
  bitslices<uint64_t> _bitslices;

  SwitchDir ssdir;
  ColorMapper _cm;
};

#endif
