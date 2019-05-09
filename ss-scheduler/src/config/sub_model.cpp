#include "sub_model.h"
#include "model_parsing.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <map>
#include <utility>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;


using namespace SS_CONFIG;
using namespace std;

//COMMA IF NOT FIRST
static void CINF(std::ostream& os, bool& first) {
  if(first) {
    first=false;
  } else {
    os << ", " ;
  }
}

void ssio_interface::sort(std::vector<std::pair<int,int>>& portID2size, 
     std::map<int,ssvport*>& vports) {
  int index = 0;
  portID2size.resize(vports.size());
  for(auto i : vports) {
    int id = i.first;
    int size = i.second->size();
    portID2size[index++] = std::make_pair(id,size);
  }
  std::sort(portID2size.begin(), portID2size.end(), 
            [](std::pair<int,int>& left, std::pair<int,int>& right){
    return left.second < right.second;
  });
}

// ----------------------- sslink ---------------------------------------------

std::string sslink::name() const {
    std::stringstream ss;
    ss << "L" << "_" << _orig->name() << "_" << _dest->name();
    return ss.str();
}

std::string sslink::gams_name(int config) const {
    std::stringstream ss;
    ss << _orig->gams_name(config) << "_" << _dest->gams_name(config);
    return ss.str();
}

std::string sslink::gams_name(int st, int end) const {
    std::stringstream ss;
    ss << _orig->gams_name(st) << "_" << _dest->gams_name(end);
    return ss.str();
}

// ---------------------- ssswitch --------------------------------------------

ssinput* ssswitch::getInput(int i) {
  ssnode::const_iterator I,E;   //Typedef for const_iterator for links
  
  int count = 0;
  for(I=_in_links.begin(); I!=E; ++I) {
      ssnode* node = (*I)->orig();
      if(ssinput* input_node = dynamic_cast<ssinput*>(node)) {
        if(count==i) {
            return input_node;
        }
        ++count;
      }
  }
  return nullptr; //failed to find input
}

//Get output based on the number in each sslink
ssoutput* ssswitch::getOutput(int i) {
  ssnode::const_iterator I,E;
  
  int count = 0;
  for(I=_out_links.begin(); I!=E; ++I) {
      ssnode* node = (*I)->dest();
      if(ssoutput* output_node = dynamic_cast<ssoutput*>(node)) {
        if(count==i) {
            return output_node;
        }
        ++count;
      }  
  }
  return nullptr; //failed to find output
}

void parse_list_of_ints(std::istream& istream, std::vector<int>& int_vec) {
  string cur_cap;
  while (getline(istream, cur_cap, ' ')) {
    if(cur_cap.empty()) {
      continue;
    } 
    int val;
    istringstream(cur_cap)>>val;
    int_vec.push_back(val);
  }
}

void parse_port_list(std::istream& istream,std::vector<std::pair<int,std::vector<int> > >& int_map) {
  string cur_cap;

  while (getline(istream, cur_cap, ',')) {
    int port_val;

    ModelParsing::trim(cur_cap);

    if(cur_cap.empty()) { 
      continue;
    }

    if(cur_cap.find(':')==std::string::npos) {
      std::cout << "Incorrect Port Map Specification! (no colon)\n";
      std::cout << "in \"" << cur_cap << "\"\n";
      assert(0);
    }

    istringstream ss(cur_cap);

    getline(ss, cur_cap, ':');
    istringstream(cur_cap) >> port_val;         //cgra port num

    std::vector<int> map_list;
    while (getline(ss, cur_cap, ' ')) {
      if(cur_cap.empty()) {
        continue;
      } 
      int val;
      istringstream(cur_cap)>>val;
      map_list.push_back(val);
    }
    int_map.push_back(make_pair(port_val,map_list));
  }
}


void SubModel::parse_io(std::istream& istream) {
    string param,value,portstring;

    while(istream.good()) {
        if(istream.peek()=='[') break;  //break out if done

        ModelParsing::ReadPair(istream,param,value);
        std::stringstream ss(param);          
        getline(ss, param, ' ');
        
        getline(ss, portstring);                        //port num
        int port_num;
        istringstream(portstring) >> port_num;

        std::vector<std::pair<int,std::vector<int> > > int_map;
        std::vector<int> int_vec;

        std::stringstream ssv(value);
             
        static int message=0;
        if(ModelParsing::StartsWith(param, "VPORT_IN")) {
          if(message++==0) 
            cout << "VPORT_IN depricated, ignoring colons, switch to PORT_IN\n";
          parse_port_list(ssv,int_map);
          for(auto& p : int_map) int_vec.push_back(p.first);
          _ssio_interf.in_vports[port_num]=new ssvport();
          _ssio_interf.in_vports[port_num]->set_port_vec(int_vec);
        } else if(ModelParsing::StartsWith(param, "VPORT_OUT")) {
          if(message++==0) 
             cout << "VPORT_OUT depricated, ignoring colons, switch to PORT_IN\n";
          parse_port_list(ssv,int_map);
          for(auto& p : int_map) int_vec.push_back(p.first);
          _ssio_interf.out_vports[port_num]=new ssvport();
          _ssio_interf.out_vports[port_num]->set_port_vec(int_vec);
        } else if(ModelParsing::StartsWith(param, "PORT_IN")) {
          parse_list_of_ints(ssv,int_vec);
          _ssio_interf.in_vports[port_num]=new ssvport();
          _ssio_interf.in_vports[port_num]->set_port_vec(int_vec);
        } else if(ModelParsing::StartsWith(param, "PORT_OUT")) {
          parse_list_of_ints(ssv,int_vec);
          _ssio_interf.out_vports[port_num]=new ssvport();
          _ssio_interf.out_vports[port_num]->set_port_vec(int_vec);
        }
    }
}

bool parseInt(std::string param, string value, const char* param_name, int& i) {
  if(ModelParsing::StartsWith(param, param_name)) {
    istringstream(value) >> i;
    return true;
  }
  return false;
}

// ------------------------ submodel impl -------------------------------------

SubModel::SubModel(std::istream& istream, FuModel* fuModel, bool multi_config) {
  
    string param,value;
    
    bool should_read=true;
    
    //parameters used here for initialization:
    int switch_outs=2, switch_ins=2, bwm=1;
    double bwmfrac=0.0;
    
    int temp_width, temp_height;  //size of temporal region
    int temp_x,     temp_y;       //location of temporal region

    PortType portType = PortType::opensp;
    
    while(istream.good()) {
        if(istream.peek()=='[') break;  //break out if done

        if(should_read) ModelParsing::ReadPair(istream,param,value);
        should_read=true;
       
        parseInt(param,value, "width",_sizex);
        parseInt(param,value, "height",_sizey);
        parseInt(param,value, "outs_per_switch", switch_outs);
        parseInt(param,value, "ins_per_switch", switch_ins);
        parseInt(param,value, "bwm", bwm);
        parseInt(param,value, "temporal_x", temp_x);
        parseInt(param,value, "temporal_y", temp_y);
        parseInt(param,value, "temporal_width",  temp_width);
        parseInt(param,value, "temporal_height", temp_height);

        if(ModelParsing::StartsWith(param, "io_layout")) {
            ModelParsing::trim(value);
            if(ModelParsing::StartsWith(value,"open_splyser")) {
                portType = PortType::opensp;
            } else if(ModelParsing::StartsWith(value,"every_switch")) {
                portType = PortType::everysw;
            } else if(ModelParsing::StartsWith(value,"three_sides_in")) {
                portType = PortType::threein;
            } else if(ModelParsing::StartsWith(value,"three_in_two_out")) {
                portType = PortType::threetwo;
            } else {
                cerr << "io_layout parameter: \"" << value << "\" not recognized\n"; 
                assert(0);
            }
        } else if (ModelParsing::StartsWith(param, "bw_extra")) {
            istringstream(value) >> bwmfrac;
        } else if (ModelParsing::StartsWith(param, "SS_LAYOUT")) {
          //defining switch capability
          
          ModelParsing::trim(value);
          
          // std::cout << "CGRA SIZE: " << _sizex << ", " << _sizey << "\n";
          build_substrate(_sizex,_sizey);
          
          if(value.compare("FULL")==0)
          {
              for(int j = 0; j < _sizey; j++)
              {
                string line, fustring;
                getline(istream,line);
                
                stringstream ss(line);
                
                for(int i = 0; i < _sizex; i++)
                {
                    getline(ss,fustring,' ');
                    
                    if(fustring.length()==0) {
                        --i;
                        continue;
                    }

                    _fus[i][j]->setFUDef(fuModel->GetFUDef(fustring));   //Setting the def of each FU
                }
              }
              
          } else {
              cerr << "Unsupported FU Initialization Type\n";   
          }
          
          /*else if(value.compare("RATIO_RAND")==0)
          {
              ReadPair(istream,param,value);
              func_unit* fu = GetFU(trim(param));
              while(istream.good() && fu)
              {
                  stringstream ss(value);
                  ss >> fu->ratio;
              }
              
              should_read=false;
          }*/
          
          
        } 
    }

   connect_substrate(_sizex, _sizey, portType, switch_ins, switch_outs, multi_config,
                     temp_x, temp_y, temp_width, temp_height);
    
}

//Graph of the configuration or substrate
void SubModel::PrintGraphviz(ostream& ofs) {
  ofs << "Digraph G { \n";

  //switchesnew_sched
  for (int i = 0; i < _sizex+1; ++i) {
    for (int j = 0; j < _sizey+1; ++j) {
      //ofs << switches[i][j]->name() <<"[ label = \"Switch[" << i << "][" << j << "]\" ];\n";
      
     //output links  
      for(auto &elem : _switches[i][j]->out_links()) {
        const ssnode* dest_node = elem->dest();         //FUs and output nodes
        ofs << _switches[i][j]->name() << " -> " << dest_node->name() << ";\n";
      }
      
    }
  }
  
  //fus
  for (int i = 0; i < _sizex; ++i) {
    for (int j = 0; j < _sizey; ++j) {
      //ofs << fus[i][j]->name() <<"[ label = \"FU[" << i << "][" << j << "]\" ];\n";
      
      for(auto &elem : _fus[i][j]->out_links()) {
        const ssnode* dest_node = elem->dest();             //Output link of each FU
        ofs << _fus[i][j]->name() << " -> " << dest_node->name() << ";\n";
      }
    }
  }

  //Input nodes
  for (unsigned i = 0; i < _inputs.size(); ++i) {
    //ofs << _inputs[i]->name() <<"[ label = \"IPort[" << i << "]\" ];\n";
    for(auto &elem : _inputs[i]->out_links()) {
      const ssnode* dest_node = elem->dest();       //Dest nodes for input ndoes are switches
      ofs << _inputs[i]->name() << " -> " << dest_node->name() << ";\n";
    }
    
  }
 
  /*
  for (unsigned i = 0; i < outputs.size(); ++i) {
    ofs << outputs[i]->name() <<"[ label = \"OPort[" << i << "]\" ];\n";
    ssnode::const_iterator I = outputs[i]->ibegin(), E = outputs[i]->iend();
    for(;I!=E;++I) {
      const ssnode* orig_node = (*I)->orig();
      ofs << orig_node->name() << " -> " << outputs[i]->name() << ";\n";
    }
  }*/

   

  ofs << "}\n";
}

void SubModel::clear_fu_runtime_vals() {
  for(auto& i : _fu_list) {
    i->reset_runtime_vals();
  }
}

void SubModel::clear_all_runtime_vals() {
  for (auto &i : _inputs) {
    i->reset_runtime_vals();
  }
  for (auto &i : _outputs) {
    i->reset_runtime_vals();
  }
  clear_fu_runtime_vals();
  for (auto &i : _switch_list) {
    i->reset_runtime_vals();
  }
}

#if 0

//GAMS specific
void SubModel::PrintGamsModel(ostream& ofs, unordered_map<string,pair<ssnode*,int> >& node_map, 
                              unordered_map<string,pair<sslink*,int> >& link_map, int n_configs) {
  
  // --------------------------- First, print the node sets ------------------------
  ofs << "$onempty\n";
  ofs << "Sets\n";
  ofs << "n \"Hardware Nodes\"\n /";

    
  bool first = true;
  for(int config=0; config < n_configs; ++config) {
    
    //fus    
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
          if(first) {
              first = false;
          } else {
              ofs << ", ";   
          }
          ofs << _fus[i][j]->gams_name(config);
          node_map[_fus[i][j]->gams_name(config)]=make_pair(&_fus[i][j],config);
      }
    }
    //inputs
    ofs << "\n";
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        ofs << ", " << _inputs[i]->gams_name(config);
        node_map[_inputs[i]->gams_name(config)]=make_pair(&_inputs[i],config);
    }
    //outputs
    ofs << "\n";
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        ofs << ", " << _outputs[i]->gams_name(config);
        node_map[_outputs[i]->gams_name(config)]=make_pair(&_outputs[i],config);
    }

  }
  ofs << "/\n";
  
  // --------------------------- next, print the capabilility sets  ------------------------
  
  //input nodes
  first = true;
  ofs << "inN(n) \"Input Nodes\"\n /";
  
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        if(first) {
            first = false;
        } else {
            ofs << ", ";   
        }
        ofs << _inputs[i]->gams_name(config);
    }
  }
  ofs << "/\n";
  
  //output nodes
  first = true;
  ofs << "outN(n) \"Output Nodes\"\n /";
  
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        if(first) {
            first = false;
        } else {
            ofs << ", ";   
        }
        ofs << _outputs[i]->gams_name(config);
    }
  }
  ofs << "/\n";
  
  for(int i = 2; i < SS_NUM_TYPES; ++i) {
    ss_inst_t ss_inst = (ss_inst_t)i;
    
    ofs << name_of_inst(ss_inst) << "N(n) /";
    
    first=true;
    for(int config=0; config < n_configs; ++config) {
      
      for (int i = 0; i < _sizex; ++i) {
        for (int j = 0; j < _sizey; ++j) {
          
            if(_fus[i][j]->fu_def()==nullptr || _fus[i][j]->fu_def()->is_cap(ss_inst)){
              if(first) {
                  first = false;
              } else {
                  ofs << ", ";   
              }
              ofs << _fus[i][j]->gams_name(config);
            }
        }
      }
    }

    ofs << "/\n";
  }
  
  //create the kindN Set
  ofs << "kindN(K,n) \"Capabilities of a Node\" \n";
  
  // --------------------------- print the switches  ------------------------
  ofs << "r \"Routers (switches)\"\n /";
  first = true;
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
          if(first) {
              first = false;
          } else {
              ofs << ", ";   
          }
          ofs << _switches[i][j]->gams_name(config);
      }
    }
  }
  

  ofs << "/\n";
  
  
  
  ofs << "l \"Links\"\n /";
  first = true;
  //_switches
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) { 
        ssnode::const_iterator I = _switches[i][j]->obegin(), E = _switches[i][j]->oend();
        for(;I!=E;++I) {
          if(first) {
              first = false;
          } else {
              ofs << ", ";   
          }
          ofs << (*I)->gams_name(config);
          link_map[(*I)->gams_name(config)]=make_pair(*I,config);
        }
      }
    }
  
    //fus
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        ssnode::const_iterator I = _fus[i][j]->obegin(), E = _fus[i][j]->oend();
        for(;I!=E;++I) {
          ofs << ", " << (*I)->gams_name(config);
          link_map[(*I)->gams_name(config)]=make_pair(*I,config);
        }
      }
    }
    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      ssnode::const_iterator I = _inputs[i]->obegin(), E = _inputs[i]->oend();
      for(;I!=E;++I) {
        ofs << ", " << (*I)->gams_name(config);
        link_map[(*I)->gams_name(config)]=make_pair(*I,config);
      }
    }
  }
  
  //TODO: Print extra                 links for the cross switch
  
  ofs << "/;\n";
  
  // --------------------------- Enable the Sets ------------------------
  ofs << "kindN('Input', inN(n))=YES;\n";
  ofs << "kindN('Output', outN(n))=YES;\n";
  
  for(int i = 2; i < SS_NUM_TYPES; ++i) {
    ss_inst_t ss_inst = (ss_inst_t)i;
    ofs << "kindN(\'" << name_of_inst(ss_inst) << "\', " << name_of_inst(ss_inst) << "N(n))=YES;\n";
  }
  
  
  // --------------------------- Now Print the Linkage ------------------------
  ofs << "parameter\n";
  ofs << "Hnl(n,l) \"Node Outputs\" \n/";
  //fus
  first=true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        ssnode::const_iterator I = _fus[i][j]->obegin(), E = _fus[i][j]->oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << _fus[i][j]->gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }
    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      ssnode::const_iterator I = _inputs[i]->obegin(), E = _inputs[i]->oend();
      for(;I!=E;++I) {
        ofs << ", " << _inputs[i]->gams_name(config) << "." << (*I)->gams_name(config) << " 1";
      }
    }
  }
  ofs << "/\n";
  
  
  ofs << "Hrl(r,l) \"Router Outputs\" \n/";
  first = true;
  
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        ssnode::const_iterator I = _switches[i][j]->obegin(), E = _switches[i][j]->oend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << _switches[i][j]->gams_name(config) << "." << (*I)->gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/\n";
  
  ofs << "Hln(l,n) \"Node Inputs\" \n/";
  //fus
  first=true;
  for(int config=0; config < n_configs; ++config) {  
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        ssnode::const_iterator I = _fus[i][j]->ibegin(), E = _fus[i][j]->iend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config) << "." << _fus[i][j]->gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "\n";
  
  //inputs
  for(int config=0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _outputs.size(); ++i) {
      ssnode::const_iterator I = _outputs[i]->ibegin(), E = _outputs[i]->iend();
      for(;I!=E;++I) {
        ofs << ", " << (*I)->gams_name(config) << "." << _outputs[i]->gams_name(config) << " 1";
      }
    }
  }
  ofs << "/\n";
  
  ofs << "Hlr(l,r) \"Router Inputs\" \n/";
  first = true;
  for(int config=0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex+1; ++i) {
      for (int j = 0; j < _sizey+1; ++j) {
        ssnode::const_iterator I = _switches[i][j]->ibegin(), E = _switches[i][j]->iend();
        for(;I!=E;++I) {
          CINF(ofs,first);
          ofs << (*I)->gams_name(config) << "." <<  _switches[i][j]->gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/;\n";
  
}
#endif


int dist_grid(int x, int  y) {
  int dist = abs(x) + abs(y) + 1;
  if(x < 0) dist += 1;
  if(y < 0) dist += 1;
  if(x > 0 && y > 0) dist -=1;
  return dist;
}

int dist_switch(int x, int  y) {
  int dist = abs(x) + abs(y) + 1;
  if(x < 0) dist -= 1;
  if(y < 0) dist -= 1;
  return dist;
}

//void SubModel::AllToAllDist() {
//  //this is pure laziness, but meh -- this only happens once : )
//  std::map<ssnode*, ssnode*> dist;
//
//
//
//}


void SubModel::PrintGamsModel(ostream& ofs, 
                              unordered_map<string, pair<ssnode*,int>>& node_map, 
                              unordered_map<string, pair<sslink*,int>>& link_map, 
                              unordered_map<string, pair<ssswitch*,int>>& switch_map, 
                              unordered_map<string, pair<bool, int>>& port_map, 
                              int n_configs) {

  //int in each maps is the configuration num
  //string -- name of node and position

  // --------------------------- First, print the node sets ------------------------
  ofs << "$onempty\n";
  ofs << "Sets\n";
  ofs << "n \"Hardware Nodes\"\n /";
  bool first = true;

  for (int config = 0; config < n_configs; ++config) {

    //fus    
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        CINF(ofs, first);
        ofs << _fus[i][j]->gams_name(config);
        node_map[_fus[i][j]->gams_name(config)] = make_pair(_fus[i][j], config);
      }
    }

    //inputs
    ofs << "\n";
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      ofs << ", " << _inputs[i]->gams_name(config);
      node_map[_inputs[i]->gams_name(config)] = make_pair(_inputs[i], config);
    }

    //outputs
    ofs << "\n";
    for (unsigned i = 0; i < _outputs.size(); ++i) {
      ofs << ", " << _outputs[i]->gams_name(config);
      node_map[_outputs[i]->gams_name(config)] = make_pair(_outputs[i], config);
    }

  }
  ofs << "/\n";


  // --------------------------- next, print the capabilility sets  ------------------------
  //input nodes
  first = true;
  ofs << "inN(n) \"Input Nodes\"\n /";

  for (int config = 0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      CINF(ofs, first);
      ofs << _inputs[i]->gams_name(config);
    }
  }
  ofs << "/\n";

  //output nodes
  first = true;
  ofs << "outN(n) \"Output Nodes\"\n /";

  for (int config = 0; config < n_configs; ++config) {
    for (unsigned i = 0; i < _outputs.size(); ++i) {
      CINF(ofs, first);
      ofs << _outputs[i]->gams_name(config);
    }
  }
  ofs << "/\n";

  //total capabilities 
  for (int i = 2; i < SS_NUM_TYPES; ++i) {
    ss_inst_t ss_inst = (ss_inst_t) i;

    ofs << name_of_inst(ss_inst) << "N(n) /";

    first = true;
    for (int config = 0; config < n_configs; ++config) {
      for (int i = 0; i < _sizex; ++i) {
        for (int j = 0; j < _sizey; ++j) {
          if (_fus[i][j]->fu_def() == nullptr || _fus[i][j]->fu_def()->is_cap(ss_inst)) {
            CINF(ofs, first);
            ofs << _fus[i][j]->gams_name(config);            //Each FU in the grid 
          }
        }
      }
    }

    ofs << "/\n";
  }

  //create the kindN Set
  ofs << "kindN(K,n) \"Capabilities of a Node\" \n";

  // -------------------------- print the ports ----------------------------
  ofs << "pn \"Port Interface Declarations\"\n /";

//  int num_port_interfaces=_ssio_interf.in_vports.size() + _ssio_interf.out_

  // Declare Vector Ports 
  first = true;

  for (auto I = _ssio_interf.in_vports.begin(), E = _ssio_interf.in_vports.end(); I != E; ++I) {
    CINF(ofs, first);
    ofs << "ip" << I->first;
    port_map[string("ip") + std::to_string(I->first)] = make_pair(true, I->first);
  }

  for (auto I = _ssio_interf.out_vports.begin(), E = _ssio_interf.out_vports.end(); I != E; ++I) {
    CINF(ofs, first);
    ofs << "op" << I->first;
    port_map[string("op") + std::to_string(I->first)] = make_pair(false, I->first);
  }

  if (first == true) {
    ofs << "pnXXX";
  }
  ofs << "/\n";

  // --------------------------- print the switches  ------------------------
  ofs << "r \"Routers (switches)\"\n /";
  first = true;

  for (int config = 0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex + 1; ++i) {
      for (int j = 0; j < _sizey + 1; ++j) {
        CINF(ofs, first);
        ofs << _switches[i][j]->gams_name(config);
      }
    }

    /*
    //Inputs and outputs are also switches
    //inputs
    ofs << "\n";
    for (unsigned i = 0; i < _inputs.size(); ++i) {
        ofs << ", " << _inputs[i]->gams_name(config);
        node_map[_inputs[i]->gams_name(config)]=make_pair(&_inputs[i],config);
    }
    //outputs
    ofs << "\n";
    for (unsigned i = 0; i < _outputs.size(); ++i) {
        ofs << ", " << _outputs[i]->gams_name(config);
        node_map[_outputs[i]->gams_name(config)]=make_pair(&_outputs[i],config);
    }
    */

    if (_multi_config) {
      if (config != n_configs - 1) {
        ofs << ", " << _cross_switch.gams_name(config);
        node_map[_cross_switch.gams_name(config)] = make_pair(&_cross_switch, config);
      }
    }


  }

  ofs << "/\n";

  ofs << "l \"Links\"\n /";
  first = true;
  //_switches
  for (int config = 0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex + 1; ++i) {
      for (int j = 0; j < _sizey + 1; ++j) {
        for (auto &elem: _switches[i][j]->out_links()) {
          CINF(ofs, first);
          ofs << elem->gams_name(config);
          link_map[elem->gams_name(config)] = make_pair(elem, config);
        }
        switch_map[_switches[i][j]->gams_name(config)] = make_pair(_switches[i][j], config);
      }
    }

    //fus
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        for (auto &elem : _fus[i][j]->out_links()) {
          ofs << ", " << elem->gams_name(config);
          link_map[elem->gams_name(config)] = make_pair(elem, config);
        }
      }
    }

    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      for (auto &elem : _inputs[i]->out_links()) {
        ofs << ", " << elem->gams_name(config);
        link_map[elem->gams_name(config)] = make_pair(elem, config);
      }
    }

    if (_multi_config) {
      if (config != n_configs - 1) {
        for (auto &elem: _cross_switch.in_links()) {
          ofs << ", " << elem->gams_name(config);
          link_map[elem->gams_name(config)] = make_pair(elem, config);
        }
        for (auto &elem : _cross_switch.out_links()) {
          ofs << ", " << elem->gams_name(config, config + 1);
          link_map[elem->gams_name(config, config + 1)] = make_pair(elem, config);
        }
      }

      for (auto &elem: _load_slice.in_links()) {
        ofs << ", " << elem->gams_name(config);
        link_map[elem->gams_name(config)] = make_pair(elem, config);
      }
      for (auto &elem: _load_slice.out_links()) {
        ofs << ", " << elem->gams_name(config);
        link_map[elem->gams_name(config)] = make_pair(elem, config);
      }
    }

  }
  ofs << "/;\n";

  if (_multi_config) {
    first = true;
    ofs << "set loadlinks(l) \"loadslice links\" \n /";   // Loadslice Links
    for (int config = 0; config < n_configs; ++config) {
      for (auto &elem : _load_slice.in_links()) {
        if (first) first = false;
        else ofs << ", ";
        ofs << elem->gams_name(config);
        link_map[elem->gams_name(config)] = make_pair(elem, config);
      }
      for (auto &elem: _load_slice.out_links()) {
        ofs << ", " << elem->gams_name(config);
        link_map[elem->gams_name(config)] = make_pair(elem, config);
      }

    }
    ofs << "/;\n";
  }

  // --------------------------- Enable the Sets ------------------------
  ofs << "kindN('Input', inN(n))=YES;\n";
  ofs << "kindN('Output', outN(n))=YES;\n";

  for (int i = 2; i < SS_NUM_TYPES; ++i) {
    ss_inst_t ss_inst = (ss_inst_t) i;
    ofs << "kindN(\'" << name_of_inst(ss_inst) << "\', " << name_of_inst(ss_inst) << "N(n))=YES;\n";
  }


  //Print Parameters  
  ofs << "parameter\n";
  // Node Distances
  int config = 0;

  // ----------------------- Node Distances -----------------------------------
  first = true;
  ofs << "DIST(n,n) \"Node Distances\"\n /";
  for (int x1 = 0; x1 < _sizex; ++x1) {
    for (int y1 = 0; y1 < _sizey; ++y1) {
      //me to all func units
      for (int x2 = 0; x2 < _sizex; ++x2) {
        for (int y2 = 0; y2 < _sizey; ++y2) {
          if (x1 == x2 && y1 == y2) continue;
          CINF(ofs, first);

          int d = dist_grid(x2 - x1, y2 - y1);
          ofs << _fus[x1][y1]->gams_name(config) << "."
              << _fus[x2][y2]->gams_name(config) << " " << d;
        }
      }
      //all inputs to me
      for (unsigned i = 0; i < _inputs.size(); ++i) {
        CINF(ofs, first);
        ssswitch *sw = static_cast<ssswitch *>(_inputs[i]->getFirstOutLink()->dest());
        int d = dist_switch(x1 - sw->x(), y1 - sw->y());
        ofs << _inputs[i]->gams_name(config) << "."
            << _fus[x1][y1]->gams_name(config) << " " << d;
      }

      //all outputs to me
      for (unsigned i = 0; i < _outputs.size(); ++i) {
        CINF(ofs, first);
        ssswitch *sw = static_cast<ssswitch *>(_outputs[i]->getFirstInLink()->orig());
        int d = dist_switch(x1 - sw->x(), y1 - sw->y());
        ofs << _fus[x1][y1]->gams_name(config) << "."
            << _outputs[i]->gams_name(config) << " " << d;
      }
      ofs << "\n";
    }
  }
  ofs << "/\n";


  // ----------------------- Node Loc -------------------------------------
  ofs << "PXn(n) \" Position X \"\n /";
  first = true;
  for (int x = 0; x < _sizex; ++x) {
    for (int y = 0; y < _sizey; ++y) {
      CINF(ofs, first);
      ofs << _fus[x][y]->gams_name(config) << " "
          << _fus[x][y]->x() * 2 + 1;
    }
  }
  for (unsigned i = 0; i < _inputs.size(); ++i) {
    CINF(ofs, first);
    ssswitch *sw = static_cast<ssswitch *>(_inputs[i]->getFirstOutLink()->dest());
    ofs << _inputs[i]->gams_name(config) << " " << sw->x() * 2;
  }
  for (unsigned i = 0; i < _outputs.size(); ++i) {
    CINF(ofs, first);
    ssswitch *sw = static_cast<ssswitch *>(_outputs[i]->getFirstInLink()->orig());
    ofs << _outputs[i]->gams_name(config) << " " << sw->x() * 2;
  }
  ofs << "/\n";

  ofs << "PYn(n) \" Position X \"\n /";
  first = true;
  for (int x = 0; x < _sizex; ++x) {
    for (int y = 0; y < _sizey; ++y) {
      CINF(ofs, first);
      ofs << _fus[x][y]->gams_name(config) << " "
          << _fus[x][y]->y() * 2 + 1;
    }
  }
  for (unsigned i = 0; i < _inputs.size(); ++i) {
    CINF(ofs, first);
    ssswitch *sw = static_cast<ssswitch *>(_inputs[i]->getFirstOutLink()->dest());
    ofs << _inputs[i]->gams_name(config) << " " << sw->y() * 2;
  }
  for (unsigned i = 0; i < _outputs.size(); ++i) {
    CINF(ofs, first);
    ssswitch *sw = static_cast<ssswitch *>(_outputs[i]->getFirstInLink()->orig());
    ofs << _outputs[i]->gams_name(config) << " " << sw->y() * 2;
  }
  ofs << "/\n";


  // --------------------------- Print Port Interfaces --------------------
  ofs << "PI(pn,n) \"Port Interfaces\" /\n";
  // Declare Port to Node Mapping
  first = true;

  for (auto I = _ssio_interf.in_vports.begin(), E = _ssio_interf.in_vports.end(); I != E; ++I) {
    int i = 0;

    //for each elem in the vector
    for (auto port : I->second->port_vec()) {
      CINF(ofs, first);
      if ((unsigned) port >= _inputs.size()) { assert(0 && "TOO HIGH OP INDEX"); }
      ofs << "ip" << I->first << "." << _inputs[port]->gams_name(0) << " " << i + 1; //no config supp.
    }
  }

  for (auto I = _ssio_interf.out_vports.begin(), E = _ssio_interf.out_vports.end(); I != E; ++I) {
    int i = 0;
    for (auto port : I->second->port_vec()) {
      CINF(ofs, first);
      if ((unsigned) port >= _outputs.size()) { assert(0 && "TOO HIGH OP INDEX"); }
      ofs << "op" << I->first << "." << _outputs[port]->gams_name(0) << " " << i + 1;
    }
  }

  ofs << "/\n";

  // --------------------------- Now Print the Linkage ------------------------

  ofs << "Hnl(n,l) \"Node Outputs\" \n/";
  //fus
  first = true;

  for (int config = 0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        for (auto &elem : _fus[i][j]->out_links()) {
          CINF(ofs, first);
          ofs << _fus[i][j]->gams_name(config) << "." << elem->gams_name(config) << " 1";
        }
      }
    }

    //inputs
    for (unsigned i = 0; i < _inputs.size(); ++i) {
      for (auto &elem: _inputs[i]->out_links()) {
        ofs << ", " << _inputs[i]->gams_name(config) << "." << elem->gams_name(config) << " 1";
      }
    }

    if (_multi_config) {
      //print loadslice and crosswitch links
      if (config != n_configs - 1) {
        //outputs
        for (unsigned i = 0; i < _outputs.size(); ++i) {
          for (auto &elem : _outputs[i]->out_links()) {
            ofs << ", " << _outputs[i]->gams_name(config) << "." << elem->gams_name(config) << " 1";
          }
        }
      }
      //print final loadslice links
      if (config == n_configs - 1) {
        for (auto &elem : _load_slice.in_links()) {
          ssnode *output = elem->orig();
          ofs << ", " << output->gams_name(config) << "." << elem->gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/\n";


  ofs << "Hrl(r,l) \"Router Outputs\" \n/";
  first = true;

  for (int config = 0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex + 1; ++i) {
      for (int j = 0; j < _sizey + 1; ++j) {
        for (auto &elem : _switches[i][j]->out_links()) {
          CINF(ofs, first);
          ofs << _switches[i][j]->gams_name(config) << "." << elem->gams_name(config) << " 1";
        }
      }
    }

    if (_multi_config) {
      //if not last config, print routing
      if (config != n_configs - 1) {
        for (auto &elem : _cross_switch.out_links()) {
          ofs << ", " << _cross_switch.gams_name(config)
              << "." << elem->gams_name(config, config + 1) << " 1";
        }
      }
    }
  }
  ofs << "/\n";

  ofs << "Hln(l,n) \"Node Inputs\" \n/";
  //fus
  first = true;
  for (int config = 0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex; ++i) {
      for (int j = 0; j < _sizey; ++j) {
        for (auto &elem : _fus[i][j]->in_links()) {
          CINF(ofs, first);
          ofs << elem->gams_name(config) << "." << _fus[i][j]->gams_name(config) << " 1";
        }
      }
    }
    //outputs

    for (unsigned i = 0; i < _outputs.size(); ++i) {
      for (auto &elem : _outputs[i]->in_links()) {
        ofs << ", " << elem->gams_name(config)
            << "." << _outputs[i]->gams_name(config) << " 1";
      }
    }
    if (_multi_config) {
      if (config != 0) {
        //outputs
        for (unsigned i = 0; i < _inputs.size(); ++i) {
          for (auto &elem: _inputs[i]->in_links()) {
            ofs << ", " << elem->gams_name(config - 1, config)
                << "." << _inputs[i]->gams_name(config) << " 1";
          }
        }
      }
      if (config == 0) {
        for (auto &elem : _load_slice.out_links()) {
          ssnode *input = elem->dest();
          ofs << ", " << elem->gams_name(config) << "." << input->gams_name(config) << " 1";
        }
      }
    }
  }

  ofs << "/\n";

  ofs << "Hlr(l,r) \"Router Inputs\" \n/";
  first = true;
  for (int config = 0; config < n_configs; ++config) {
    for (int i = 0; i < _sizex + 1; ++i) {
      for (int j = 0; j < _sizey + 1; ++j) {
        for (auto &elem : _switches[i][j]->in_links()) {
          CINF(ofs, first);
          ofs << elem->gams_name(config) << "." << _switches[i][j]->gams_name(config) << " 1";
        }
      }
    }
    if (_multi_config) {
      //if not last config, print routing
      if (config != n_configs - 1) {
        for (auto &elem: _cross_switch.in_links()) {
          ofs << ", " << elem->gams_name(config)
              << "." << _cross_switch.gams_name(config) << " 1";
        }
      }
    }
  }
  ofs << "/;\n";

}


SubModel::SubModel(int x, int y, PortType pt, int ips, int ops,bool multi_config) {
  build_substrate(x,y);
  connect_substrate(x,y,pt,ips,ops,multi_config,0,0,0,0);
}


void SubModel::build_substrate(int sizex, int sizey) {
  
  _sizex=sizex;
  _sizey=sizey;
  
  // Create FU array
  _fus.resize(_sizex);
  
  //Iterate each x vector -- vector of ssfu objects
  for (unsigned x = 0; x < _fus.size(); x++) {
    _fus[x].resize(_sizey);
    for(unsigned y = 0; y < (unsigned)_sizey; ++y) {
        add_fu(x,y);
    }
  }
  
  // Create Switch array
  _switches.resize(_sizex+1);
  for (unsigned x = 0; x < _switches.size(); x++) {
    _switches[x].resize(_sizey+1);
    for(unsigned y = 0; y < (unsigned)_sizey+1; ++y) {
      add_switch(x,y);
    }
  }
}


//Group Nodes/Links and Set IDs
//This should be done after all the links are added
void SubModel::regroup_vecs() {
  _node_list.clear();
  _io_list.clear();

  for (auto &elem: _inputs) {
    elem->set_id(_node_list, _link_list);
    _io_list.push_back(elem);
  }

  for (auto &elem : _fu_list)
    elem->set_id(_node_list, _link_list);

  for (auto &elem: _switch_list)
    elem->set_id(_node_list, _link_list);

  for (auto &elem: _outputs) {
    elem->set_id(_node_list, _link_list);
    _io_list.push_back(elem);
  }
}

void SubModel::connect_substrate(int _sizex, int _sizey, PortType portType, int ips, int ops, bool multi_config, int temp_x, int temp_y, int temp_width, int temp_height) {

  {
    const int di[] = {0, 1, 1, 0};
    const int dj[] = {0, 0, 1, 1};
    const SwitchDir::DIR dir[] = {SwitchDir::SE, SwitchDir::SW, SwitchDir::NW, SwitchDir::NE};

    const int t_di[] = {0, 1, 0};
    const int t_dj[] = {0, 0, 1};
    const SwitchDir::DIR t_dir[] = {SwitchDir::NW, SwitchDir::NE, SwitchDir::SW};
    //first connect switches to FUs
    for (int i = 0; i < _sizex; i++) {
      for (int j = 0; j < _sizey; j++) {

        for (int k = 0; k < 4; ++k)
          _switches[i + di[k]][j + dj[k]]->add_link(_fus[i][j])->setdir(dir[k]);

        //output from FU -- SE
        _fus[i][j]->add_link(_switches[i + 1][j + 1])->setdir(SwitchDir::SE);

        //For temporal region, lets add some extra outputs!
        if (i >= temp_x && i < temp_x + temp_width
             && j >= temp_y && j < temp_y + temp_width) {
          for (int k = 0; k < 3; ++k)
            _fus[i][j]->add_link(_switches[i + t_di[k]][j + t_dj[k]])->setdir(t_dir[k]);
        }
      }
    }
  }

  //Now Switches to eachother
  {
    const int di[] = {-1, 0, 1, 0};
    const int dj[] = {0, -1, 0, 1};
    const SwitchDir::DIR dir[] = {SwitchDir::W, SwitchDir::N, SwitchDir::E, SwitchDir::S};
    for (int i = 0; i < _sizex + 1; i++) {
      for (int j = 0; j < _sizey + 1; j++) {
        for (int k = 0; k < 4; ++k) {
          int _i = i + di[k];
          int _j = j + dj[k];
          if (_i >= 0 && _i <= _sizex && _j >= 0 && _j <= _sizey)
            _switches[i][j]->add_link(_switches[_i][_j])->setdir(dir[k]);
        }
      }
    }
  }

  if(portType == PortType::opensp) {  //OpenSplyser Inputs/Outputs

    //Inputs to Switches
    add_inputs((_sizex +_sizey)*ips);

    //left-edge switches except top-left
    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ips; p++) {
        sslink * link = _inputs[sw*ips+p]->add_link(_switches[0][_sizey - sw]);
        if(p==0) link->setdir(SwitchDir::IP0);
        else if(p==1) link->setdir(SwitchDir::IP1);
        else if(p==2) link->setdir(SwitchDir::IP2);
      }
    }

    //Top switches except top-right
    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ips; p++) {
        sslink* link = _inputs[_sizey*ips + sw*ips+p]->add_link(_switches[sw][0]);
        if(p==0) link->setdir(SwitchDir::IP0);
        else if(p==1) link->setdir(SwitchDir::IP1);
        else if(p==2) link->setdir(SwitchDir::IP2);
     }
    }

    //Switches to Outputs
    add_outputs((_sizex+_sizey)*ops);

    //bottom op switches except bottom left
    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ops; p++) {
        sslink* link = _switches[sw+1][_sizey]->add_link(_outputs[sw*ops+p]);
        if(p==0) link->setdir(SwitchDir::OP0);
        else if(p==1) link->setdir(SwitchDir::OP1);
        else if(p==2) link->setdir(SwitchDir::OP2);
      }
    }


    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ops; p++) {
        sslink* link = _switches[_sizex][_sizey-sw-1]->add_link(_outputs[_sizex*ops +sw*ops+p]);
        if(p==0) link->setdir(SwitchDir::OP0);
        else if(p==1) link->setdir(SwitchDir::OP1);
        else if(p==2) link->setdir(SwitchDir::OP2);
      }
    }


  } else if(portType == PortType::threein || portType == PortType::threetwo) {  //Three sides have inputs

    bool bonus_middle = false;    

    //Inputs to Switches
    add_inputs((_sizex*(1+bonus_middle)+_sizey*2)*ips);

    int in_index=0;
    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ips; p++) {
        sslink * link = _inputs[in_index++]->add_link(_switches[0][_sizey-sw]);
        if(p==0) link->setdir(SwitchDir::IP0);
        else if(p==1) link->setdir(SwitchDir::IP1);
        else if(p==2) link->setdir(SwitchDir::IP2);
      }
    }

    for(int sw = 0; sw < _sizex; sw++) {
      for(int p = 0; p < ips; p++) {
        sslink* link = _inputs[in_index++]->add_link(_switches[sw][0]);
        if(p==0) link->setdir(SwitchDir::IP0);
        else if(p==1) link->setdir(SwitchDir::IP1);
        else if(p==2) link->setdir(SwitchDir::IP2);
     }
    }

    for(int sw = 0; sw < _sizey; sw++) {
      for(int p = 0; p < ips; p++) {
        sslink* link = _inputs[in_index++]->add_link(_switches[_sizex][sw]);
        if(p==0) link->setdir(SwitchDir::IP0);
        else if(p==1) link->setdir(SwitchDir::IP1);
        else if(p==2) link->setdir(SwitchDir::IP2);
      }
    }

    if(bonus_middle) {  //TODO: make an option for this
      //cout << "bonus inputs: ";
      for(int sw = 0; sw < _sizex; sw++) {
        for(int p = 0; p < ips; p++) {
          //cout << in_index << " ";
          assert((unsigned)in_index < _inputs.size());
          sslink* link = _inputs[in_index++]->add_link(_switches[sw+1][_sizey]);
          if(p==0) link->setdir(SwitchDir::IP0);
          else if(p==1) link->setdir(SwitchDir::IP1);
          else if(p==2) link->setdir(SwitchDir::IP2);
        }
      }
      cout << "\n";
      assert((unsigned)in_index == _inputs.size());
    }




    if(portType == PortType::threein) {

      //Switches to Outputs
      add_outputs((_sizex)*ops);
    
      int out_index=0;
      for(int sw = 0; sw < _sizex; sw++) {
        for(int p = 0; p < ops; p++) {
          sslink* link = _switches[sw+1][_sizey]->add_link(_outputs[out_index++]);
          if(p==0) link->setdir(SwitchDir::OP0);
          else if(p==1) link->setdir(SwitchDir::OP1);
          else if(p==2) link->setdir(SwitchDir::OP2);
        }
      }

    } else if(portType == PortType::threetwo) {
      //Switches to Outputs
      add_outputs((_sizex+_sizey)*ops);
    
      int out_index=0;
      for(int sw = 0; sw < _sizex; sw++) {
        for(int p = 0; p < ops; p++) {
          sslink* link = _switches[sw+1][_sizey]->add_link(_outputs[out_index++]);
          if(p==0) link->setdir(SwitchDir::OP0);
          else if(p==1) link->setdir(SwitchDir::OP1);
          else if(p==2) link->setdir(SwitchDir::OP2);
        }
      }
      for(int sw = 0; sw < _sizey; sw++) {
        for(int p = 0; p < ops; p++) {
          sslink* link = _switches[_sizex][_sizey-sw-1]->add_link(_outputs[out_index++]);
          if(p==0) link->setdir(SwitchDir::OP0);
          else if(p==1) link->setdir(SwitchDir::OP1);
          else if(p==2) link->setdir(SwitchDir::OP2);
        }
      }
    }


  } else if(portType == PortType::everysw) {  //all switches have inputs/outputs
      
    add_inputs((_sizex+1)*(_sizey+1)*ips);
    add_outputs((_sizex+1)*(_sizey+1)*ops);
    //first connect _switches to FUs
    int inum=0;
    int onum=0;

    for(int i = 0; i < _sizex+1; i++) {
      for(int j = 0; j < _sizey+1; j++) {
        for(int p = 0; p < ips; p++) {
          sslink* link = _inputs[inum]->add_link(_switches[i][_sizey-j]);
          
          if(p==0) link->setdir(SwitchDir::IP0);
          else if(p==1) link->setdir(SwitchDir::IP1);
          else if(p==2) link->setdir(SwitchDir::IP2);

          inum++;
        }
        for(int p = 0; p < ops; p++) {
          sslink* link = _switches[i][_sizey-j]->add_link(_outputs[onum]);
          
          if(p==0) link->setdir(SwitchDir::OP0);
          else if(p==1) link->setdir(SwitchDir::OP1);
          else if(p==2) link->setdir(SwitchDir::OP2);
         
          onum++;
        }
      }
    }

  }

  if(multi_config) {
    printf("USING MULTI CONFIG (NOT REALLY SUPPORTED ANYMORE)\n");
  }

  _multi_config=multi_config;
  
  //if(multi_config) {
  //  _cross_switch.setXY(999,999);
  //  for(unsigned i = 0; i < _inputs.size(); i++) {
  //    _load_slice->add_link(_inputs[i]);
  //    _cross_switch->add_link(_inputs[i]);  //direction?

  //  }
  //  for(unsigned i = 0; i < _outputs.size(); i++) {
  //    _outputs[i]->add_link(_load_slice);
  //    _outputs[i]->add_link(_cross_switch);  
  //  }
  //}

  //The primitive temporal region that we are going to create just has local
  //connections to surrounding nodes.
  //TODO: FIXME: We need some way of specifying the max util in the config file

  for(int i = temp_x; i < temp_x+temp_width; i++) {
    for (int j = temp_y; j < temp_y + temp_height; j++) {
      _fus[i][j]->set_max_util(64);
      sslink *link = _fus[i][j]->add_link(_fus[i][j]);
      link->set_max_util(100);
      link->setdir(SwitchDir::IP0);

      if (i + 1 < temp_x + temp_width) {
        sslink *link = _fus[i][j]->add_link(_fus[i + 1][j]);
        link->setdir(SwitchDir::E);
        link->set_max_util(100);
      }
      if (i > temp_x) {
        sslink *link = _fus[i][j]->add_link(_fus[i - 1][j]);
        link->setdir(SwitchDir::W);
        link->set_max_util(100);
      }
      if (j + 1 < temp_y + temp_height) {
        sslink *link = _fus[i][j]->add_link(_fus[i][j + 1]);
        link->setdir(SwitchDir::N);
        link->set_max_util(100);
      }
      if (j > temp_y) {
        sslink *link = _fus[i][j]->add_link(_fus[i][j - 1]);
        link->setdir(SwitchDir::S);
        link->set_max_util(100);
      }
    }
  }

  for (int i = 0; i < _sizex; ++i) {
    for (int j = 0; j < _sizey; ++j) {
      auto link = _fus[i][j]->add_link(_fus[i][j]);
      link->setdir(SwitchDir::IP0);

      if (i < temp_x && i >= temp_x + temp_width && 
          j < temp_y && j >= temp_y + temp_height) {
        link->set_max_util(8);
      }
    }
  }

  regroup_vecs();
}


/*
class PrioritizeFU { public : int operator()( const func_unit_def* x, const func_unit_def *y ) 
{
    return x->diff > y->diff;
    } }; 
  */

/*
void SubMo    for(int i = 0; i < _inputs.size(); i++) {
      _inputs[i]->add_link(cross_switch);  //direction?
    }
    for(int i = 0; i < _outputs.size(); i++) {
      cross_switch->add_link(_outputs[i]);  //TODO
    }del::SetTotalFUByRatio()
{
    //map<int,int> fu2diff;
    
    float total = 0;

    for(unsigned i=0; i < fu_set.size(); ++i) 
        total += fu_set[i].ratio;  //get total ratio

    int totalfus = _sizex*_sizey;

    int int_total_fus = 0;

    
    priority_queue<func_unit*, vector<func_unit*>, PrioritizeFU> diff_queue;

    int sign;
    if(int_total_fus<totalfus) sign=1; //i need to add func units
    else sign=-1; //i need to subtract func units

    //setup rounded versions
    for(unsigned i=0; i < fu_set.size(); ++i) 
    {
        if(fu_set[i].ratio==0) continue;
        
        float requested = fu_set[i].ratio/total * totalfus;
        int new_total = (int)round(requested);
        fu_set[i].diff = (((float)new_total)-requested)*sign;
        
        fu_set[i].total= new_total;
        
        diff_queue.push(&fu_set[i]);
        int_total_fus+=new_total;
    }

    unsigned total_diff = abs(totalfus-int_total_fus);

    assert(diff_queue.size()>total_diff);
    for(unsigned i = 0; i < total_diff; ++i)
    {
        func_unit* fu = diff_queue.top(); diff_queue.pop();
        
        fu->total+= sign * 1;
    }

}*/

/*
void SubModel::RandDistributeFUs()
{
    map<func_unit*, int> curFUs;
    map<func_unit*, int> maxFUs;
    

}
*/

/*
void SubModel::CreateFUArray(int _sizex, int _sizey)
{
    // Create FU array
    fu_array.resize(_sizex);
    for (unsigned x = 0; x < fu_array.size(); x++) {
        fu_array[x].resize(_sizey);
    }
}
*/
