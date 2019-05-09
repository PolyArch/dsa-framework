#ifndef __SS_SUB_MODEL_H__
#define __SS_SUB_MODEL_H__

#include "fu_model.h"
#include "direction.h"

#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <map>
#include <utility>
#include <algorithm>

namespace SS_CONFIG {

class ssnode;
class ssinput;
class ssoutput;
class ssvport;

class ssio_interface {
    public:
    //interf_vec_port_num -> vec<cgra_port_num>
    std::map<int, ssvport*> in_vports;
    std::map<int, ssvport*> out_vports;

    void sort_in_vports(std::vector<std::pair<int,int>>& portID2size) {
      sort(portID2size, in_vports);
    }
    
    void sort_out_vports(std::vector<std::pair<int,int>>& portID2size) {
      sort(portID2size, out_vports);
    }
    
    ssvport* getDesc_I(int id) {
        assert(in_vports.count(id) != 0);
        return in_vports[id];
    }  
    ssvport* getDesc_O(int id) {
        assert(out_vports.count(id) != 0);
        return out_vports[id];
    }  
    private:        
    void sort(std::vector<std::pair<int,int>>& portID2size, 
         std::map<int,ssvport*>& vports);
};

class sslink {
public:

  sslink() {}

  ssnode *orig() const { return _orig; }

  ssnode *dest() const { return _dest; }

  SwitchDir::DIR dir() const { return _dir; }

  void setdir(SwitchDir::DIR dir) { _dir = dir; }

  //Constructor
  sslink(ssnode *orig, ssnode *dest) {
    _orig = orig;
    _dest = dest;
    _ID = -1;
  }

  std::string name() const;

  std::string gams_name(int config) const;

  std::string gams_name(int, int) const;

  int id() { return _ID; }

  void set_id(int id) { _ID = id; }

  int max_util() { return _max_util; }

  int set_max_util(int m) { return _max_util = m; }

protected:
  int _ID = -1;

  int _max_util = 1; // by default, assume its a dedicated link

  ssnode *_orig;
  ssnode *_dest;
  SwitchDir::DIR _dir;

private:
  friend class SubModel;
};
    
    
class ssnode {
public:
  ssnode() {}

  sslink *add_link(ssnode *node) {
    sslink *link = new sslink(this, node);
    _out_links.push_back(link);
    node->add_back_link(link);
    return link;
  }

  void add_back_link(sslink *link) {
    _in_links.push_back(link);
  }

  virtual std::string name() const {
    return std::string("loadslice");
  }

  virtual std::string gams_name(int config = 0) const {
    return std::string("loadslice");
  }

  typedef std::vector<sslink *>::const_iterator const_iterator;

  const std::vector<sslink *> &in_links() { return _in_links; }

  const std::vector<sslink *> &out_links() { return _out_links; }

  sslink *getFirstOutLink() {
    return _out_links.empty() ? nullptr : _out_links[0];
  }

  sslink *getFirstInLink() {
    return _in_links.empty() ? nullptr : _in_links[0];
  }

  sslink *getInLink(SwitchDir::DIR dir) {
    for (auto &dlink: in_links()) {
      if (dlink->dir() == dir) return dlink;
    }
    return nullptr;
  }

  sslink *getOutLink(SwitchDir::DIR dir) {
    for (auto &dlink: out_links()) {
      if (dlink->dir() == dir) return dlink;
    }
    return nullptr;
  }

  sslink *get_cycle_link() {
    for (auto &dlink: out_links()) {
      if (dlink->dest() == this) {
        return dlink;
      }
    }
    return nullptr;
  }


  int id() { return _ID; }

  void set_id(std::vector<ssnode *> &node_list,
              std::vector<sslink *> &link_list) {
    _ID = (int) node_list.size();
    node_list.push_back(this);
    for (unsigned i = 0; i < _out_links.size(); ++i) {
      sslink *link = _out_links[i];
      assert(link->id() == -1);
      link->set_id((int) link_list.size());
      link_list.push_back(link);
    }
  }

  int node_dist(int slot) { return _node_dist[slot]; }


  std::pair<int, sslink *> came_from(int slot) { return _came_from[slot]; }

  void update_dist(int slot, int dist, int from_slot, sslink *from) {
    _node_dist[slot] = dist;
    _came_from[slot] = std::make_pair(from_slot, from);
  }

  void reset_runtime_vals() {
    memset(_node_dist, -1, sizeof _node_dist);
    memset(_came_from, 0, sizeof _came_from);
  }

  int max_util() { return _max_util; }

  int max_util(SS_CONFIG::ss_inst_t inst) { return _max_util * 64 / SS_CONFIG::bitwidth[inst]; }

  int set_max_util(int m) { return _max_util = m; }

protected:
  int _ID = -1;

  int _node_dist[8];
  std::pair<int, sslink*>_came_from[8];

  int _max_util = 1; // by default, assume its a dedicated link
  std::vector<sslink *> _in_links;
  std::vector<sslink *> _out_links;

private:
  friend class SubModel;
};
    
class ssswitch : public ssnode {
public:

  ssswitch() : ssnode() {}

  void setXY(int x, int y) {
    _x = x;
    _y = y;
  }

  int x() const { return _x; }

  int y() const { return _y; }

  virtual std::string name() const {
    std::stringstream ss;
    ss << "SW" << "_" << _x << "_" << _y;
    return ss.str();
  }

  virtual std::string gams_name(int config) const {
    std::stringstream ss;
    if (config != 0) {
      ss << "Sw" << _x << _y << "c" << config;
    } else {
      ss << "Sw" << _x << _y;
    }
    return ss.str();
  }

  ssinput *getInput(int i);

  ssoutput *getOutput(int i);

protected:
  int _x, _y;
};
    
class ssfu : public ssnode {
public:

  ssfu() : ssnode() {}

  void setFUDef(func_unit_def *fu_def) { _fu_def = fu_def; }

  void setXY(int x, int y) {
    _x = x;
    _y = y;
  }

  int x() const { return _x; }

  int y() const { return _y; }

  virtual std::string name() const {
    std::stringstream ss;
    ss << "FU" << "_" << _x << "_" << _y;
    return ss.str();
  }

  virtual std::string gams_name(int config) const {
    std::stringstream ss;
    if (config != 0) {
      ss << "Fu" << _x << _y << "c" << config;
    } else {
      ss << "Fu" << _x << _y;
    }
    return ss.str();
  }

  func_unit_def *fu_def() { return _fu_def; }

protected:
  int _x, _y;
  func_unit_def *_fu_def;

private:
  friend class SubModel;
};

class ssinput : public ssnode { 
    public:
    
    ssinput() : ssnode() {}
      
    void setPort(int port) {_port=port;}
    int port() const {return _port;}
    
    std::string name() const {
        std::stringstream ss;
        ss << "IP" << "_" << _port;
        return ss.str();
    }
    std::string gams_name(int config) const {
        std::stringstream ss;
        if(config!=0) {
          ss << "I" << _port << "c" << config;
        } else {
          ss << "I" << _port;
        }
        return ss.str();
    }
    
    protected:
    int _port;
};  

class ssoutput : public ssnode {
    public:
    ssoutput() : ssnode() {}
      
    void setPort(int port) {_port=port;}
    int port() const {return _port;}
    
    std::string name() const {
        std::stringstream ss;
        ss << "OP" << "_" << _port;
        return ss.str();
    }
    
    std::string gams_name(int config) const {
        std::stringstream ss;
        if(config!=0) {
          ss << "O" << _port << "i" << config;
        } else {
          ss << "O" << _port;
        }
        return ss.str();
    }
    
    protected:
    int _port;
};

//This should be improved later
class ssvport : public ssnode {
public:
  std::vector<int>& port_vec() {return _port_vec;}
  void set_port_vec(std::vector<int> p) {_port_vec=p;}
  size_t size() {return _port_vec.size();}

private:
  std::vector<int> _port_vec;
};

class SubModel {
public:

  //Port type of the substrate nodes
  //opensp -- dyser opensplyser N + N -1 ips
  //three ins -- Softbrain 3 x N
  //everywitch -- all switches has ops and ips
  enum class PortType {
    opensp, everysw, threein, threetwo
  };

  typedef std::vector<ssinput>::const_iterator const_input_iterator;
  typedef std::vector<ssoutput>::const_iterator const_output_iterator;

  SubModel() {}

  SubModel(std::istream &istream, FuModel *, bool multi_config = true);

  SubModel(int x, int y, PortType pt = PortType::opensp, int ips = 2, int ops = 2, bool multi_config = true);

  void PrintGraphviz(std::ostream &ofs);

  void PrintGamsModel(std::ostream &ofs,
                      std::unordered_map<std::string, std::pair<ssnode *, int> > &,
                      std::unordered_map<std::string, std::pair<sslink *, int> > &,
                      std::unordered_map<std::string, std::pair<ssswitch *, int> > &,
                      std::unordered_map<std::string, std::pair<bool, int>> &,  /*isInput, port*/
                      int n_configs = 1);

  int sizex() { return _sizex; }

  int sizey() { return _sizey; }

  ssswitch *switchAt(int x, int y) { return _switches[x][y]; }

  std::vector<ssinput*> &inputs() { return _inputs; }

  std::vector<ssoutput*> &outputs() { return _outputs; }

  std::vector<std::vector<ssfu*> > &fus() { return _fus; }

  std::vector<ssfu* > &fu_list() { return _fu_list; }

  std::vector<ssswitch* > &switch_list() { return _switch_list; }

  std::vector<std::vector<ssswitch*> > &switches() { return _switches; }

  bool multi_config() { return _multi_config; }

  ssswitch *cross_switch() { return &_cross_switch; }

  ssnode *load_slice() { return &_load_slice; }

  size_t num_fu() { return _fus.size(); }

  void parse_io(std::istream &istream);

  ssio_interface &io_interf() { return _ssio_interf; }

  void clear_all_runtime_vals();

  void clear_fu_runtime_vals();

  const std::vector<sslink *> &link_list() { return _link_list; }

  const std::vector<ssnode *> &node_list() { return _node_list; }

  void add_inputs(int n) {
    _inputs.resize(n+1);
    //Port num to each switch
    for(unsigned i = 0; i < _inputs.size(); ++i) {
      add_input(i);
    }
  }

  void add_outputs(int n) {
    _outputs.resize(n+1);
    //Port num to each switch
    for(unsigned i = 0; i < _outputs.size(); ++i) {
      add_output(i);
    }
  }


  ssinput* add_input(int i) {
    auto* in = new ssinput();
    if(i >= (int)_inputs.size()) _inputs.resize(i+1);
    assert(_inputs[i]==NULL);
    _inputs[i]=in;
    in->setPort(i);
    return in;
  }

  ssoutput* add_output(int i) {
    auto* out = new ssoutput();
    if(i >= (int)_outputs.size()) _outputs.resize(i+1);
    assert(_outputs[i]==NULL);
    _outputs[i]=out;
    out->setPort(i);
    return out;
  }

  ssfu* add_fu(int x, int y) {
    auto * fu = new ssfu();
    if(x >= (int)_fus.size()) _fus.resize(x+1);
    if(y >= (int)_fus[x].size()) _fus[x].resize(y+1);
    _fus[x][y]=fu;
    _fu_list.push_back(fu);
    fu->setXY(x,y);
    fu->setFUDef(nullptr);
    return fu;
  }

  ssswitch* add_switch(int x, int y) {
    auto* sw = new ssswitch();
    if(x >= (int)_switches.size()) _switches.resize(x+1);
    if(y >= (int)_switches[x].size()) _switches[x].resize(y+1);
    _switches[x][y]=sw;
    _switch_list.push_back(sw);
    sw->setXY(x,y);
    return sw;
  }

  void regroup_vecs(); //fills in the linear lists


private:


  //void CreateFUArray(int,int);

  //void SetTotalFUByRatio();
  //void RandDistributeFUs();
  void build_substrate(int x, int y);

  void connect_substrate(int x, int y, PortType pt, int ips, int ops, bool multi_config, int temp_x, int temp_y,
                         int temp_width, int temp_height);

  int _sizex, _sizey;  //size of SS cgra
  bool _multi_config;
  std::vector<ssinput*> _inputs;
  std::vector<ssoutput*> _outputs;
  std::vector<std::vector<ssfu*> > _fus;
  std::vector<std::vector<ssswitch*> > _switches;

  //These are only valid after regroup_vecs()
  std::vector<ssnode *> _io_list;
  std::vector<ssnode *> _node_list;
  std::vector<sslink *> _link_list;

  std::vector<ssfu *> _fu_list;
  std::vector<ssswitch *> _switch_list;

  ssswitch _cross_switch;
  ssnode _load_slice;
  ssio_interface _ssio_interf;
};

}

#endif
