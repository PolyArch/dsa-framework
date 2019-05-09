#include "schedule.h"

#include <map>
#include <iostream>
#include <fstream>

#include "model_parsing.h"
#include "ssdfg.h"
#include "ssinst.h"
#include <assert.h>
#include <list>
#include  <iomanip>
#include <unordered_set>

using namespace std;
using namespace SS_CONFIG;

//Scheduling Interface

extern "C" void libssscheduler_is_present() {}

void Schedule::clear_ssdfg() {
  if (_ssDFG) {
    delete _ssDFG;
    _ssDFG = nullptr;
  }
}

void Schedule::reset_simulation_state() {
  if(_ssDFG) {
    _ssDFG->reset_simulation_state();
  }
}

//For a given dfgnode return the input or ouput port num if the pdfgnode is a
//ssinput ot ssoutput
int Schedule::getPortFor(SSDfgNode* ssdfg_in)  { 
  if (ssnode* n = locationOf(ssdfg_in)) {
    if (ssinput *assigned_ssinput = dynamic_cast<ssinput*>(n)) {
      return assigned_ssinput->port();
    }

    if (ssoutput *assigned_ssoutput = dynamic_cast<ssoutput*>(n)) {
      return assigned_ssoutput->port();
    }
  }
  return -1; 
}

std::map<SS_CONFIG::ss_inst_t,int> Schedule::interpretConfigBits(int size,
    uint64_t* bits) {

  //Figure out if this configuration is real or not
  //NOTE: the first 9 characters of the configuration must spell filename
  //for this hack to work!
  if (strncmp((char *) bits, "filename:", 9) == 0) {
    char *c_bits = ((char *) bits) + 9;
    return interpretConfigBitsCheat(c_bits);
  } else {
    for (int i = 0; i < size; ++i) { //load in 64bit slices
      slices().write(i, bits[i]);
    }
    return map<SS_CONFIG::ss_inst_t, int>();
    //return interpretConfigBitsDedicated();
  }
}

std::map<SS_CONFIG::ss_inst_t,int> Schedule::interpretConfigBitsCheat(char* s) {
  std::map<SS_CONFIG::ss_inst_t,int> inst_histo;

  ifstream config_file;

  std::string filename = string("sched/")+string(s);
  config_file.open(filename.c_str());
  if(!config_file.good()) {
    filename = string(getenv("DFG_COMMON")) + string("/")+string(s);
    config_file.open(filename.c_str());
  } 

  if(!config_file.good()) {
    cout << "Could Not Open:" << s 
         << " at folder sched/ or $DFG_COMMON\n";
    assert(0);
  }

  static std::set<string> seen_sched;
  if(!seen_sched.count(filename)) {
    seen_sched.insert(filename);
    cout << "Using Schedule: \"" << filename << "\"\n";
  }

  boost::archive::text_iarchive ia(config_file);

  //I think this should work okay, its a little kludgey, but w/e
  Schedule sched2;
  ia >> BOOST_SERIALIZATION_NVP(sched2); //magic
  sched2._ssModel = _ssModel;
  *this = sched2;

  //Now lets patch up the schedule to get recover
  //vertex->node and edge->link mappings
  for(int i = 0; i < (int)_linkProp.size(); ++i) {
    for (int j = 0; j < 8; ++j) {
      auto &lp = _linkProp[i];
      sslink *link = _ssModel->subModel()->link_list()[i];
      for (SSDfgEdge *e : lp.slots[j].edges) {
        _edgeProp[e->id()].links.insert(make_pair(j, link));
      }
    }
  }
  for(int i = 0; i < (int)_nodeProp.size(); ++i) {
    auto& np = _nodeProp[i];
    ssnode* node = _ssModel->subModel()->node_list()[i];
    for(auto elem : np.vertices) {
      auto v = elem.second;
      _vertexProp[v->id()].node = node;
    }
  }

  for(int i = 0; i < _ssModel->subModel()->sizex(); ++i) {
    for(int j = 0; j < _ssModel->subModel()->sizey(); ++j) {
      ssfu* ssfu_node = _ssModel->subModel()->fus()[i][j];
      auto* dfg_inst = dynamic_cast<SSDfgInst*>(dfgNodeOf(ssfu_node));
      if(dfg_inst) {
        auto inst=dfg_inst->inst();
        inst_histo[inst]+=1;
      }
    }
  }

  //Lets also just throw the node id at the dfg for now to make temporal
  //simulation work
  for(auto inst : _ssDFG->inst_vec()) {
    if(locationOf(inst)) {
      inst->set_node_id(locationOf(inst)->id());
    }
  }

  return inst_histo;
}

//TODO: Later we uncomment these bunch of codes to do real CGRA config.
//std::map<SS_CONFIG::ss_inst_t,int> Schedule::interpretConfigBitsDedicated() {
//  std::map<SS_CONFIG::ss_inst_t, int> inst_histo;
//
//  SwitchDir ssdir;
//  vector<vector<ssfu> > &fus = _ssModel->subModel()->fus();
//  SSDfgInst *dfg_inst;
//
//  map<ssnode *, map<SwitchDir::DIR, SwitchDir::DIR> > routeMap;
//  map<SSDfgNode *, vector<SwitchDir::DIR> > posMap;
//  map<ssnode *, SSDfgNode *> dfgnode_for;
//  _ssDFG = new SSDfg();
//
//  std::set<uint64_t> inputs_used;   //vector ports used
//  std::set<uint64_t> outputs_used;
//
//  _decode_lat_mis = slices().read_slice(IN_ACT_SLICE, 56, 63);
//
//  //Associating the DFG Nodes from the configuration bits, with the vector ports defined by the hardware
//  int start_bits_vp_mask = 0;
//  int total_bits_vp_mask = 0;
//  int slice = VP_MAP_SLICE_1;
//  for (auto &port_pair : _ssModel->subModel()->io_interf().in_vports) {
//    int i = port_pair.first; //index of port
//    std::vector<std::pair<int, std::vector<int> > > &port_m = port_pair.second;
//
//    //port mapping of 1 vector port - cgra_port_num: vector offset elements
//    total_bits_vp_mask += port_m.size();
//
//    if (start_bits_vp_mask < 64 && total_bits_vp_mask > 64) {
//      start_bits_vp_mask = port_m.size();
//      total_bits_vp_mask = port_m.size();
//      slice = VP_MAP_SLICE_2;
//    }
//
//    if (slices().read_slice(IN_ACT_SLICE, i, i)) {
//      SSDfgVecInput *vec_input = new SSDfgVecInput("I", _ssDFG->num_vec_input(),
//                                                     _ssDFG);
//      //vec_input->setLocMap(pm);
//      //_ssDFG->insert_vec_in(vec_input); -- don't need this, stored in group
//
//      //cout << "vp" << i << "  ";
//
//      vector<bool> mask;
//      mask.resize(port_m.size());
//      for (unsigned mi = 0; mi < port_m.size(); ++mi) {
//        mask[mi] = slices().read_slice(slice,
//                                       start_bits_vp_mask + mi, start_bits_vp_mask + mi);
//        if (mask[mi]) {
//          int ss_in_port = port_m[mi].first;
//          ssinput *in = _ssModel->subModel()->get_input(ss_in_port);
//          SSDfgInput *dfg_in = new SSDfgInput(_ssDFG);
//          dfg_in->setVPort(vec_input);
//          dfgnode_for[in] = dfg_in;
//          _ssDFG->addInput(dfg_in); //add input to dfg
//          vec_input->addInput(dfg_in); //add input to vector
//
//          //cout << mi << " (" << in->port() << ")";
//        }
//      }
//      //cout << "\n";
//      assign_vport(vec_input, make_pair(true/*input*/, i), mask);
//    }
//
//    start_bits_vp_mask = total_bits_vp_mask; //next
//  }
//
//  start_bits_vp_mask = 0;
//  total_bits_vp_mask = 0;
//
//  for (auto &port_pair : _ssModel->subModel()->io_interf().out_vports) {
//    int i = port_pair.first; //index of port
//    std::vector<std::pair<int, std::vector<int> > > &port_m = port_pair.second;
//
//    total_bits_vp_mask += port_m.size();
//
//    if (slices().read_slice(OUT_ACT_SLICE, i, i)) { //activate output port
//      SSDfgVecOutput *vec_output = new SSDfgVecOutput("O", _ssDFG->num_vec_output(),
//                                                        _ssDFG);
//
//      //_ssDFG->insert_vec_out(vec_output); don't do this, this is in group
//
//      vector<bool> mask;
//      mask.resize(port_m.size());
//      for (unsigned mi = 0; mi < port_m.size(); ++mi) {
//        mask[mi] = slices().read_slice(VP_MAP_SLICE_OUT,
//                                       start_bits_vp_mask + mi, start_bits_vp_mask + mi);
//        if (mask[mi]) {
//          ssoutput *out = _ssModel->subModel()->get_output(port_m[mi].first);
//          SSDfgOutput *dfg_out = new SSDfgOutput(_ssDFG);
//          dfg_out->setVPort(vec_output);
//          dfgnode_for[out] = dfg_out;
//          _ssDFG->addOutput(dfg_out);
//          vec_output->addOutput(dfg_out); //add output to vector
//        }
//      }
//      assign_vport(vec_output, make_pair(false/*input*/, i), mask);
//    }
//
//    start_bits_vp_mask = total_bits_vp_mask; //next
//  }
//
//
//  //TODO: NOT NEEDED
//  //Read input nodes
//  for (int i = 0; i < 32; ++i) {  //64 ports ?
//    uint64_t inact = _bitslices.read_slice(IN_ACT_SLICE, i, i);
//
//    if (inact) {
//      auto &vp = _ssModel->subModel()->io_interf().in_vports[i];
//      for (auto &p : vp) {     //iterate through ports of vector port i
//        inputs_used.insert(p.first);        //cgra port
//      }
//    }
//  }
//
//  //Read output nodes
//  for (int i = 0; i < 32; ++i) {
//    uint64_t outact = _bitslices.read_slice(OUT_ACT_SLICE, i, i);
//
//    //If the outport !=0
//    if (outact) {
//      auto &vp = _ssModel->subModel()->io_interf().out_vports[i];
//      for (auto &p : vp) {     //iterate through ports of vector port i
//        outputs_used.insert(p.first);
//      }
//    }
//  }
//
//  int cur_slice = SWITCH_SLICE;       //5th slice in bitslice
//
//  //In1, In2, In3, Opcode, S1, S2, ... S8, Row
//  vector<vector<ssswitch> > &switches = _ssModel->subModel()->switches();
//
//  for (int i = 0; i < _ssModel->subModel()->sizex() + 1; ++i) {
//    bool left = (i == 0);
//    bool right = (i == _ssModel->subModel()->sizex());
//    for (int j = 0; j < _ssModel->subModel()->sizey() + 1; ++j, ++cur_slice) {
//      bool top = (j == 0);
//      bool bottom = (j == _ssModel->subModel()->sizey());
//      ssswitch *sssw = &switches[i][j];
//
//      //cout << "Decode switch: " << i << "," << j << "\n";
//
//      //read the [Row]
//      int row = _bitslices.read_slice(cur_slice, ROW_LOC, ROW_LOC + ROW_BITS - 1);
//      assert(row == j);
//
//      int cur_bit_pos = SWITCH_LOC;
//
//      //---------------------------------DECODE SWITCHES ---------------------------
//      for (int o = 0; o < NUM_OUT_DIRS; ++o, cur_bit_pos += BITS_PER_DIR) {
//
//        uint64_t b = _bitslices.read_slice(cur_slice, cur_bit_pos,
//                                           cur_bit_pos + BITS_PER_DIR - 1);
//        SwitchDir::DIR out_dir = ssdir.dir_for_slot(o, top, bottom, left, right);
//        SwitchDir::DIR in_dir = ssdir.decode(b, top, bottom, left, right);
//        in_dir = SwitchDir::reverse(in_dir);
//
//        sslink *inlink = sssw->getInLink(in_dir);      //get the link object with that dir
//        if (!inlink) {
//          //cout << "no in_dir:" << SwitchDir::dirName(in_dir) << " bits:" << b << " (pos:" << o << ")\n";
//          continue; //no worries, this wasn't even a valid inlink
//        }
//
//        sslink *outlink = sssw->getOutLink(out_dir);
//        if (!outlink) {
//          //cout << "no out_dir:" << SwitchDir::dirNameDBG(out_dir) << " loc:" << o << "\n";
//          continue; //skip if no corresponding link
//        }
//
//        //cout << SwitchDir::dirName(in_dir) << "->" <<
//        //        SwitchDir::dirName(out_dir) << ":";
//
//        //cout << b << " @pos: " << o << " " << out_dir << "\n";
//
//        inst_histo[SS_CONFIG::SS_Switch] += 1;
//
//        assert(outlink->orig() == inlink->dest());
//        assign_switch(sssw, inlink, outlink);
//        //For better or worse, reconstruct takes in a route map, yes, this is redundant
//        //with assign_switch
//        routeMap[sssw][out_dir] = in_dir;
//
//        //if(ssfu* fu =dynamic_cast<ssfu*>(outlink->dest())) {
//        //  //create corresponding DFG Node
//        //  dfg_inst = new SSDfgInst();
//        //  dfgnode_for[fu]=dfg_inst;
//        //  _ssDFG->addInst(dfg_inst);
//        //} else
//
//        ////if the swithces out is an output node
//        //if(ssoutput* out = dynamic_cast<ssoutput*>(outlink->dest())) {
//        //  SSDfgOutput* dfg_out = new SSDfgOutput();
//        //  dfgnode_for[out]=dfg_out;
//        //  _ssDFG->addOutput(dfg_out);
//        //  dfg_out->setVPort(out->port());
//        //}
//
//        ////if the incoming node was from ssinput node
//        //if (ssinput* in=dynamic_cast<ssinput*>(inlink->orig())) {
//        //  SSDfgInput* dfg_in;
//        //  //Need to check if this is actually one of the useful inputs
//        //  if(inputs_used.count(in->port())) {
//        //    if(dfgnode_for.count(in)==0) {
//        //      cout << "Creating node for port " << in->port() << "\n";
//        //      dfg_in = new SSDfgInput();
//        //      dfgnode_for[in]=dfg_in;
//        //      _ssDFG->addInput(dfg_in);
//        //    } else {
//        //      dfg_in = dynamic_cast<SSDfgInput*>(dfgnode_for[in]);
//        //    }
//        //    dfg_in->setVPort(in->port());
//        //  }
//        //}//end if
//
//      }//end for
//
//    }//end for j
//  }//end for i
//
//
//  for (int g = 0; g < NUM_DFG_GROUPS; ++g) {
//    //Read input nodes
//    for (int i = 0; i < 32; ++i) {  //32 ports max
//      int i_adj = (g % 2) * 32 + i;
//      uint64_t inact = _bitslices.read_slice(IN_ACT_GROUP12 + g / 2, i_adj, i_adj);
//      if (inact) {
//        SSDfgVecInput *in_vec =
//                dynamic_cast<SSDfgVecInput *>(vportOf(make_pair(true, i)));
//        assert(in_vec);
//        _ssDFG->insert_vec_in_group(in_vec, g);
//      }
//    }
//    for (int i = 0; i < 32; ++i) {  //32 ports ?
//      int i_adj = (g % 2) * 32 + i;
//      uint64_t outact = _bitslices.read_slice(OUT_ACT_GROUP12 + g / 2, i_adj, i_adj);
//
//      if (outact) {
//        SSDfgVecOutput *out_vec =
//                dynamic_cast<SSDfgVecOutput *>(vportOf(make_pair(false, i)));
//        assert(out_vec);
//        _ssDFG->insert_vec_out_group(out_vec, g);
//      }
//    }
//  }
//
//  //---------------------------------DECODE FUNC UNITS ---------------------------
//  cur_slice = SWITCH_SLICE;
//  for (int i = 0; i < _ssModel->subModel()->sizex(); ++i) {
//    for (int j = 0; j < _ssModel->subModel()->sizey(); ++j, ++cur_slice) {
//      ssfu *ssfu_node = fus[i][j];
//
//      //opcode
//      uint64_t op = _bitslices.read_slice(cur_slice, OPCODE_LOC, OPCODE_LOC + OPCODE_BITS - 1);
//      if (op != 0) { //if O
//        auto inst = ssfu_node->fu_def()->inst_of_encoding(op);
//        dfg_inst = new SSDfgInst(_ssDFG, inst);
//        stringstream verif_name;
//        verif_name << i << "-" << j;
//        dfg_inst->set_verif_id(verif_name.str());
//
//        dfgnode_for[ssfu_node] = dfg_inst;
//        _ssDFG->addInst(dfg_inst);
//
//        inst_histo[inst] += 1;
//
//        //8-switch_dirs + 4bits_for_row
//        unsigned cur_bit_pos = FU_DIR_LOC;
//
//        for (int f = 0; f < NUM_IN_FU_DIRS; ++f, cur_bit_pos += BITS_PER_FU_DIR) {
//          uint64_t b = _bitslices.read_slice(cur_slice, cur_bit_pos,
//                                             cur_bit_pos + BITS_PER_FU_DIR - 1);
//          SwitchDir::DIR dir = ssdir.fu_dir_of(b);
//          assert(f != 0 || (f == 0 && dir != SwitchDir::END_DIR));
//          posMap[dfg_inst].push_back(dir);      //incoming FU dir
//          if (dir == SwitchDir::IM) {
//            dfg_inst->setImmSlot(f);
//          }
//        }//end for input fu dirs
//
//        //predictate inverse
//        uint64_t p = _bitslices.read_slice(cur_slice, FU_PRED_INV_LOC,
//                                           FU_PRED_INV_LOC + FU_PRED_INV_BITS - 1);
//        dfg_inst->setPredInv(p);
//      } else {
//        //cout << i << " " << j << " not mapped\n";
//      }
//    }
//    cur_slice += 1; // because we skipped the switch
//  }
//
//  cur_slice = SWITCH_SLICE +
//              (_ssModel->subModel()->sizex() + 1) * (_ssModel->subModel()->sizey() + 1);
//
//  //--------------------------------------- DECODE CONSTANTS ------------------------
//  while ((unsigned) cur_slice < _bitslices.size()) {
//    int row = _bitslices.read_slice(cur_slice, ROW_LOC, ROW_LOC + ROW_BITS - 1);
//    int col = _bitslices.read_slice(cur_slice, COL_LOC, COL_LOC + COL_BITS - 1);
//
//    assert(row < _ssModel->subModel()->sizey());
//    assert(col < _ssModel->subModel()->sizex());
//    ssfu *ssfu_node = fus[col][row];
//    assert(ssfu_node);

//    //cout << "row,col" << row << " " << col << "\n";
//    SSDfgNode *node = dfgnode_for[ssfu_node];
//    assert(node);
//    SSDfgInst *inst = dynamic_cast<SSDfgInst *>(node);
//
//
//    uint64_t ctrl_bits = _bitslices.read_slice(cur_slice, CTRL_LOC,
//                                               CTRL_LOC + CTRL_BITS - 1);
//
//    inst->set_ctrl_bits(CtrlBits(ctrl_bits));
//
//    bool has_imm = _bitslices.read_slice(cur_slice, IS_IMM_LOC, IS_IMM_LOC + IS_IMM_BITS - 1);
//
//
//    if (has_imm) {
//      uint64_t imm = _bitslices.read_slice(cur_slice, 0, 63);
//      assert(inst->immSlot() != -1);
//      inst->setImm(imm);
//    }
//
//    ++cur_slice;
//  }
//
//  //routemap -- for each ssnode - inlink and outlinks
//  //dfgnode_for -- ssnode to dfgnode mapping
//  //posMap -- for each dfgnode, vector of incoming dirs
//
//  reconstructSchedule(routeMap, dfgnode_for, posMap);
//
//  // Iterate over FUs, get the inc_edge assoc. with each FU_INPUT, set extra lat.
//  cur_slice = SWITCH_SLICE;
//  for (int i = 0; i < _ssModel->subModel()->sizex(); ++i) {
//    for (int j = 0; j < _ssModel->subModel()->sizey(); ++j, ++cur_slice) {
//      ssfu *ssfu_node = fus[i][j];
//      if (nodeAssigned(ssfu_node) != 0) {
//        SSDfgInst *dfg_node = dynamic_cast<SSDfgInst *>(dfgNodeOf(ssfu_node));
//
//        for (int n = 0; n < NUM_IN_FU_DIRS; ++n) {
//          if (dfg_node->immSlot() == n) {
//            //Do Nothing
//          } else if (n < (dfg_node->ops_end() - dfg_node->ops_begin())) {
//            SSDfgEdge *inc_edge = (dfg_node->ops_begin() + n)->get_first_edge();
//            if (!inc_edge) {
//              continue;
//            }
//
//            // delay for each input
//            int d1 = IN_DELAY_LOC + BITS_PER_DELAY * n;
//            int d2 = d1 + BITS_PER_DELAY - 1;
//            set_edge_delay(_bitslices.read_slice(cur_slice, d1, d2), inc_edge);
//
//            //_bitslices.write(cur_slice, d1, d2, _extraLatOfEdge[inc_edge]);
//            //  cout <<  i << " " << j << " slice: " << cur_slice
//            //     << ",delay:" << _extraLatOfEdge[inc_edge] << "\n";
//
//          } else {
//            assert(n != 0 && "can't be no slot for first input");
//          }
//        }
//      }
//    }
//    cur_slice += 1; // because we skipped the switch
//  }
//
//  calcLatency(_max_lat, _max_lat_mis, false);
///*  if(0 != _max_lat_mis) {
//    cerr << "The FU input latencies don't match, this may or may not be a problem!\n";
//  }*/
//
//  calc_out_lat();

//  return inst_histo;
//}

//Write to a header file
void Schedule::printConfigHeader(ostream& os, std::string cfg_name,
                                 bool use_cheat) {
  //Step 1: Write the vector port mapping
  os << "#ifndef " << "__" << cfg_name << "_H__\n";
  os << "#define " << "__" << cfg_name << "_H__\n";

  for(auto& i : _assignVPort) {
    std::pair<bool,int> pn = i.first;
    SSDfgVec* pv = i.second;
    os << "#define P_" << cfg_name << "_" << pv->name() << " " << pn.second << "\n"; 
  }
  os<< "\n";

  if(use_cheat) {
    printConfigCheat(os,cfg_name);
  } else {
    printConfigBits(os,cfg_name);
  }

  os << "#endif //" << cfg_name << "_H\n"; 
}

void Schedule::printConfigCheat(ostream& os, std::string cfg_name) {
  //First, print the config to the file
  std::string file_name = cfg_name + string(".sched");
  std::string full_file_name = string("sched/") + file_name;
  std::ofstream sched_file(full_file_name);
  boost::archive::text_oarchive oa(sched_file);

  oa << BOOST_SERIALIZATION_NVP(*this);

  os << "// CAUTION: This is a Boost::Serialization-based version\n"
     << "// of the schedule.  (ie. cheating)  It is for simulation only.\n"
     << "// corresponding dfg is in: " << full_file_name << "\n\n";

  //Approximate number of config words, good enough for now
  int config_words  = (_ssModel->subModel()->sizex()+1) *
                      (_ssModel->subModel()->sizey()+1)  + 16;

  //Negative size indicates funny thing
  os << "#define " << cfg_name << "_size " << config_words << "\n\n";

  //NOTE: Filename is necessary here! it is the indicator that we
  //are cheating and not giving the real config bits
  os << "char " << cfg_name << "_config[" << config_words << "] = \"";
  os << "filename:" << file_name.c_str() << "\";\n\n";
}

void Schedule::printConfigBits(ostream& os, std::string cfg_name) {
  //print_bit_loc();

  //Active Input Ports
  for(auto& i : _assignVPort) {
    std::pair<bool,int> pn = i.first;
    //SSDfgVec* pv = i.second;
    
    if(pn.first) { //INPUT
      _bitslices.write(IN_ACT_SLICE, pn.second,pn.second,1);
    } else { //OUTPUT
      _bitslices.write(OUT_ACT_SLICE,pn.second,pn.second,1);
    }
  }

  int max_lat_mis=0, max_lat=0;
  cheapCalcLatency(max_lat,max_lat_mis);
  _bitslices.write(IN_ACT_SLICE, 56, 63, max_lat_mis);

  int num_vec_groups = _ssDFG->num_groups();
  assert(num_vec_groups <= NUM_DFG_GROUPS);
  for(int g = 0; g < num_vec_groups; ++g) {
    vector<SSDfgVecInput*>& vec = _ssDFG->vec_in_group(g);
    for(auto* vec_in : vec) {
      int port_num = vecPortOf(vec_in).second;
      int bit_pos=(g%2)*32+port_num;
      _bitslices.write(IN_ACT_GROUP12+g/2, bit_pos, bit_pos, 1);
    }
  }

  for(int g = 0; g < num_vec_groups; ++g) {
    vector<SSDfgVecOutput*>& vec = _ssDFG->vec_out_group(g);
    for(auto* vec_out : vec) {
      int port_num = vecPortOf(vec_out).second;
      int bit_pos=(g%2)*32+port_num;
      _bitslices.write(OUT_ACT_GROUP12+g/2, bit_pos, bit_pos, 1);
    }
  }



  // --------------------------- ENCODE VP MASK ------------------------------
  int start_bits_vp_mask=0;
  int total_bits_vp_mask=0;
  int slice=VP_MAP_SLICE_1;
  for(auto& port_pair : _ssModel->subModel()->io_interf().in_vports) {
    const std::vector<int>& port_m = port_pair.second->port_vec();
    total_bits_vp_mask+=port_m.size();
 
    if(start_bits_vp_mask < 64 && total_bits_vp_mask >64) {
      start_bits_vp_mask=port_m.size();
      total_bits_vp_mask=port_m.size();
      slice=VP_MAP_SLICE_2;
    }

    //Is this port assigned?  if not can skip 
    if(SSDfgVec* dfg_vec_in = vportOf(make_pair(true/*input*/,port_pair.first))) {
      vector<bool> mask = maskOf(dfg_vec_in);

      for(unsigned i = 0; i < port_m.size(); ++i) {
        _bitslices.write(slice, start_bits_vp_mask+i,start_bits_vp_mask+i,mask[i]);
      }
    }
  
    start_bits_vp_mask=total_bits_vp_mask; //next
  }

  start_bits_vp_mask=0;
  total_bits_vp_mask=0;
  for(auto& port_pair : _ssModel->subModel()->io_interf().out_vports) {
    // pair<cgra_port_location, vector<indicies_into_vec>>
    const std::vector<int>& port_m = port_pair.second->port_vec();
    total_bits_vp_mask+=port_m.size();
 
    //Is this port assigned?  if not can skip 
    if(SSDfgVec* dfg_vec_out = vportOf(make_pair(false/*output*/,port_pair.first))) {
      vector<bool> mask = maskOf(dfg_vec_out);

      for(unsigned i = 0; i < port_m.size(); ++i) {
        _bitslices.write(VP_MAP_SLICE_OUT, start_bits_vp_mask+i,start_bits_vp_mask+i,mask[i]);
      }
    }
  
    start_bits_vp_mask=total_bits_vp_mask; //next
  }

  xfer_link_to_switch(); // makes sure we have switch representation of routing
  int cur_slice=SWITCH_SLICE;

  vector< vector<ssfu*> >& fus = _ssModel->subModel()->fus();

  //In1, In2, In3, Opcode, S1, S2, ... S8, Row
  vector< vector<ssswitch*> >& switches = _ssModel->subModel()->switches();
  for(int i = 0; i < _ssModel->subModel()->sizex()+1; ++i) {
    
    bool left = (i==0);                 //left edge switch
    bool right = (i==_ssModel->subModel()->sizex());    //right edge switch
    
    for(int j = 0; j < _ssModel->subModel()->sizey()+1; ++j,++cur_slice) {
      
      //cout << "Encode switch: " << i << "," << j << "\n";

      bool top = (j==0);
      bool bottom = (j==_ssModel->subModel()->sizey());

      //Write the [Row] -- corresponding row of the siwtch based on the j 
      _bitslices.write(cur_slice, ROW_LOC, ROW_LOC + ROW_BITS - 1, j);

      //---------------------------------ENCODE SWITCHES -------------------------------
      ssswitch* sssw = switches[i][j];

      //after switch respresntation
      std::map<SS_CONFIG::sslink*,SS_CONFIG::sslink*>& link_map = _assignSwitch[sssw];
      if(link_map.size()!=0) {

        //Step 1: Encode all output switches with unused input

        //get used inputs
        std::set<int> used_in_enc;
        for(auto I=link_map.begin(), E=link_map.end();I!=E;++I) {
          sslink* inlink=I->second;
          auto in_port  = SwitchDir::reverse(inlink->dir());
          int in_encode = ssdir.encode(in_port,top,bottom,left,right);
          used_in_enc.insert(in_encode);
        }

        int unused_dir_enc=NUM_IN_DIRS; //num input dirs
        for(int i = 0; i < NUM_IN_DIRS; ++i) {
          if(used_in_enc.count(i)==0) {
            unused_dir_enc=i;           //unused input dir
            break;
          }
        }
            
        assert(unused_dir_enc!=NUM_IN_DIRS&&"no unused direction,does this ever happen?");
        //cout << "unused:" << unused_dir_enc << "\n";

        for(int i = 0; i < NUM_OUT_DIRS; ++i) {
          unsigned p1 = SWITCH_LOC + i*BITS_PER_DIR;
          unsigned p2 = p1 + BITS_PER_DIR-1; 
          _bitslices.write(cur_slice,p1,p2,unused_dir_enc);     //Why write unused input dir
        }

        //Step 2: Fill in correct switches
        for(auto I=link_map.begin(), E=link_map.end();I!=E;++I) {
          sslink* outlink=I->first;
          sslink* inlink=I->second;
         
          auto in_port  = SwitchDir::reverse(inlink->dir());
          int in_encode = ssdir.encode(in_port,top,bottom,left,right);
          int out_pos   = ssdir.slot_for_dir(outlink->dir(),top,bottom,left,right);
         
          //cout << SwitchDir::dirName(inlink->dir()) << "->" <<
          //        SwitchDir::dirName(outlink->dir()) << ":";

          //cout << in_encode << " @pos: " << out_pos << "\n";

          unsigned p1 = SWITCH_LOC + out_pos*BITS_PER_DIR;
          unsigned p2 = p1 + BITS_PER_DIR-1; 

          _bitslices.write(cur_slice,p1,p2,in_encode,false /*don't check*/);
        }

      }//end if


      //---------------------------------ENCODE FUNC UNITS ---------------------------
      //
      if(i < _ssModel->subModel()->sizex() && j < _ssModel->subModel()->sizey()) {
        ssfu* ssfu_node = fus[i][j];

        if(isPassthrough(ssfu_node)) {
          int cur_bit_pos=FU_DIR_LOC;
          int i = 0; //only one dir allowed
          unsigned p1 = cur_bit_pos+BITS_PER_FU_DIR*i;
          unsigned p2 = p1 + BITS_PER_FU_DIR-1;


          for(auto &inlink: ssfu_node->in_links()) {
            for (int slot = 0; slot < 8; ++slot) {
              if (linkAssigned(slot, inlink)) {
                int in_encode = ssdir.encode_fu_dir(inlink->dir()); //get encoding of dir
                _bitslices.write(cur_slice, p1, p2, in_encode);   //input dir for each FU in
                break;
              }
            }
          }

          //opcode encdoing
          unsigned op_encode = ssfu_node->fu_def()->encoding_of(SS_CONFIG::SS_Copy);
          _bitslices.write(cur_slice,OPCODE_LOC,OPCODE_LOC+OPCODE_BITS-1,op_encode);
        }
       
        //get the dfg node assigned to that FU  
        if(nodeAssigned(ssfu_node)!=0) {
          SSDfgInst* dfg_node = 
            dynamic_cast<SSDfgInst*>(dfgNodeOf(ssfu_node));
          
          int cur_bit_pos=FU_DIR_LOC;

          for(int n = 0; n < NUM_IN_FU_DIRS; ++n) {
            unsigned p1 = cur_bit_pos+BITS_PER_FU_DIR*n;
            unsigned p2 = p1 + BITS_PER_FU_DIR-1;

            if(dfg_node->immSlot()==n) {
              _bitslices.write(cur_slice,p1,p2,ssdir.encode_fu_dir(SwitchDir::IM));  //imm slot for FU
            } else if(n  < (dfg_node->ops().end()-dfg_node->ops().begin())) {
              SSDfgEdge* inc_edge = dfg_node->ops()[n].get_first_edge();
              if(!inc_edge) {continue;}
              SSDfgNode* inc_dfg_node = inc_edge->def();
              
              bool assigned=false;
              for(auto inlink: ssfu_node->in_links()) {
                for (int slot = 0; slot < 8; ++slot) {
                  if (linkAssigned(slot, inlink) && dfgNodeOf(slot, inlink) == inc_dfg_node) {
                    assert(inlink->dir() != SwitchDir::END_DIR);
                    int in_encode = ssdir.encode_fu_dir(inlink->dir()); //get the encoding of the dir
                    _bitslices.write(cur_slice, p1, p2, in_encode);      //input direction for each FU in
                    assigned = true;
                    break;
                  }
                }
              }
              if(!assigned) {
                cout << "Could not find mapped input link for mapped edge: " 
                     << inc_edge->name() << " at loc: " << ssfu_node->name() 
                     << "\n";
                printGraphviz("viz/sched-fail-connection.gv");
                assert(assigned);
              }
              //assert(assigned);

              //delay for each input
              int d1=IN_DELAY_LOC+BITS_PER_DELAY*n;
              int d2=d1+BITS_PER_DELAY-1;

              int ed = edge_delay(inc_edge);
              int max_representable = ((1 << BITS_PER_DELAY)-1);
              if(ed > max_representable) {
                cout << "FU " << i << "," << j << " delay too large (" 
                  << ed << ", but if that doesn't matter just ignore.\n";
                ed = max_representable;
              }

              _bitslices.write(cur_slice, d1, d2, ed);
              //cout <<  i << " " << j << " slice: " << cur_slice 
              //   << ",delay:" << _extraLatOfEdge[inc_edge] << "\n";

            } else {
              assert(n!=0 && "can't be no slot for first input");
            }
          }
          
          //print predicate
          _bitslices.write(cur_slice,FU_PRED_INV_LOC,
                                     FU_PRED_INV_LOC+FU_PRED_INV_BITS-1,
                                     dfg_node->predInv());
         
          //opcode encdoing
          unsigned op_encode = ssfu_node->fu_def()->encoding_of(dfg_node->inst());
          _bitslices.write(cur_slice,OPCODE_LOC,OPCODE_LOC+OPCODE_BITS-1,op_encode);
        }

      } //end if for func encode
    
    }//end for switch x
  }//end for switch y 

  //--------------------------------------- ENCODE CONSTANTS ------------------------
  for(int i = 0; i < _ssModel->subModel()->sizex(); ++i) {    
    for(int j = 0; j < _ssModel->subModel()->sizey(); ++j) {
      ssfu* ssfu_node = fus[i][j];
      if(nodeAssigned(ssfu_node)!=0) {
        SSDfgInst* dfg_node = dynamic_cast<SSDfgInst*>(dfgNodeOf(ssfu_node));
        bool has_imm_slot = dfg_node->immSlot()!=-1;
        uint64_t ctrl_bits = dfg_node->ctrl_bits();
        if(has_imm_slot || ctrl_bits) {
           //cout << i << " " << j << " " << dfg_node->immSlot() << "\n";
           _bitslices.write(cur_slice,ROW_LOC,ROW_LOC+ROW_BITS-1,j);
           _bitslices.write(cur_slice,COL_LOC,COL_LOC+COL_BITS-1,i);
           _bitslices.write(cur_slice,IS_IMM_LOC,
                                      IS_IMM_LOC+IS_IMM_BITS-1,has_imm_slot);
           _bitslices.write(cur_slice,CTRL_LOC,CTRL_LOC+CTRL_BITS-1,ctrl_bits);
           ++cur_slice;
        }
        if(has_imm_slot) {
           _bitslices.write(cur_slice,0,63,dfg_node->imm());
           ++cur_slice;
        }
      }
    }
  } 
  os << "#define " << cfg_name << "_size " << _bitslices.size() << "\n\n";

  os << "unsigned long long " << cfg_name << "_config[" << _bitslices.size() << "] = {";
  for(unsigned i = 0; i < _bitslices.size(); ++i) {
    if(i!=0) {os <<", ";}
    if(i%8==0) {os <<"\n";}
    os << "0x" << std::setfill('0') << std::setw(2) << std::hex << _bitslices.read_slice(i);
  }
  os << "};\n";
  os << std::dec;
}

void Schedule::printConfigVerif(ostream& os) {
  for(unsigned i = 0; i < _bitslices.size(); ++i) {
    os << std::setfill('0') << std::setw(16) 
       << std::hex << _bitslices.read_slice(i) << "\n";
  }
}

void Schedule::printMvnGraphviz(std::ofstream& ofs, ssnode* node) {
  auto& np = _nodeProp[node->id()];

  if(np.vertices.size() == 0) {
      ofs << "<tr><td border=\"1\"> " << node->name() << " </td></tr>";
  } else {
    for(auto elem : np.vertices) {
      auto v = elem.second;
      ofs << "<tr><td port=\"" << v->name() 
          << "\" border=\"1\" bgcolor=\"#" 
          << std::hex << colorOf(v) << std::dec << "\">" 
//          << ((node->max_util()!=1) ? "T!" : "")
          << v->name() << "</td></tr>";
    }
  }

}

void Schedule::printMelGraphviz(std::ofstream& ofs, ssnode* node) {
  for(auto link: node->out_links()) {
    for (int slot= 0; slot < 8; ++slot) {
      auto &lp = _linkProp[link->id()];
      vector<SSDfgNode*> temp;
      for (auto *e : lp.slots[slot].edges)
        temp.push_back(e->def());
      sort(temp.begin(), temp.end());
      auto unique_end = unique(temp.begin(), temp.end());
      for (auto iter = temp.begin(); iter != unique_end; ++iter)
        ofs << link->orig()->name() << "->" << link->dest()->name()
            << " [color=\"#" << std::hex << colorOf(*iter) << std::dec
            << "\"];";
    }
  } 
 
}


void Schedule::printFUGraphviz(std::ofstream& ofs, ssfu* fu) {
  int sy = _ssModel->subModel()->sizey();

  ofs << fu->name() << "[shape=plaintext, ";
  ofs << "label = <<table border=\"0\" cellspacing=\"0\">";

  printMvnGraphviz(ofs,fu);

  ofs << "\n</table>>, pos = \"" << gvsf*fu->x() + gvsf/2.0 << "," 
                                 << sy-gvsf*fu->y()-1 - gvsf/2.0<< "!\"";
  ofs << ", pin=true];\n";  
}

void Schedule::printSwitchGraphviz(std::ofstream& ofs, ssswitch* sw) {
  int sy = _ssModel->subModel()->sizey();

  ofs << sw->name() << " [shape=diamond, ";
  ofs << "pos = \"" << gvsf*sw->x()  << "," << sy-gvsf*sw->y()-1 << "!\"";
  ofs << ", pin=true];\n";  
}

void Schedule::printInputGraphviz(std::ofstream& ofs, ssnode* node) {
  ofs << node->name() << "[shape=plaintext, ";
  ofs << "label = <<table border=\"0\" cellspacing=\"0\">";

  printMvnGraphviz(ofs,node);

  ofs << "\n</table>>";
  ofs << "];\n"; 
}
void Schedule::printOutputGraphviz(std::ofstream& ofs, ssnode* node) {
  ofs << node->name() << "[shape=plaintext, ";
  ofs << "label = <<table border=\"0\" cellspacing=\"0\">";

  printMvnGraphviz(ofs,node);

  ofs << "\n</table>>";
  ofs << "];\n";  
}



void Schedule::printGraphviz(const char* name) {
  ofstream ofs(name);
  assert(ofs.good());
  
  SS_CONFIG::SubModel* sub = _ssModel->subModel();

  ofs << "digraph sched {\n";

  for (auto &elem: sub->switch_list())
    printSwitchGraphviz(ofs, elem);

  for (auto elem : sub->fu_list())
    printFUGraphviz(ofs, elem);

  for(ssinput* in : sub->inputs()) {
    NodeProp& np = _nodeProp[in->id()];
    if(!np.vertices.empty()) {
      printInputGraphviz(ofs,in);
    }
  }
  for(ssoutput* out : sub->outputs()) {
    NodeProp& np = _nodeProp[out->id()];
    if(!np.vertices.empty()) {
      printOutputGraphviz(ofs,out);
    }
  }
 
  for(ssnode* node : sub->node_list())
    printMelGraphviz(ofs,node);

  ofs << "}\n\n";
}

//for(auto& p : _vecProp) {
//  SSDfgVec* vec = p.first;
//  ofs << "subgraph cluster" << vec->name() << " {\n";
//  if(SSDfgVecInput* invec = dynamic_cast<SSDfgVecInput*>(vec)) {
//    for(auto I = invec->input_begin(), E = invec->input_end(); I!=E; ++I) {
//      SSDfgInput* in = *I;
//      auto& vp = _vertexProp[in->id()];
//      printInputGraphviz(ofs,vp.node);
//    }
//  }
//  if(SSDfgVecOutput* outvec = dynamic_cast<SSDfgVecOutput*>(vec)) {
//    for(auto I = outvec->output_begin(), E = outvec->output_end(); I!=E; ++I) {
//      SSDfgOutput* out = *I;
//      auto& vp = _vertexProp[out->id()];
//      printOutputGraphviz(ofs,vp.node);
//    }
//  }



//reconstruct the schedule
void Schedule::reconstructSchedule(
                  map<ssnode*, map<SwitchDir::DIR,SwitchDir::DIR> >& routeMap,
                  map<ssnode*, SSDfgNode* >& dfgnode_for, 
                  map<SSDfgNode*, vector<SwitchDir::DIR> >& posMap
                  ) {
  //iterate over inputs (ssinputs)
  for (auto elem : _ssModel->subModel()->inputs()) {
    ssinput* ssinput_node = (ssinput*) &elem;
   
    //get the dfg node for ssinput
    if(dfgnode_for.count(ssinput_node)!=0) {
        //cout << "reconstruction from input" << ssinput_node->name() << " " << ssinput_node->port() << "\n";
        SSDfgNode* dfg_node = dfgnode_for[ssinput_node];
        tracePath(ssinput_node, dfg_node, routeMap, dfgnode_for, posMap);
    }
  }

  //iterate over fus
  vector< vector<ssfu*> >& fus = _ssModel->subModel()->fus();
  for(int i = 0; i < _ssModel->subModel()->sizex(); ++i) {
    for(int j = 0; j < _ssModel->subModel()->sizey(); ++j) {
      ssfu* ssfu_node = fus[i][j];
      if(dfgnode_for.count(ssfu_node)!=0) {
        //cout << "reconstruct from fu " << i << " " << j << "\n";
        SSDfgNode* dfg_node = dfgnode_for[ssfu_node];
        tracePath(ssfu_node, dfg_node, routeMap, dfgnode_for, posMap);
      }
        
    }
  }
  allocate_space();
}

/*
struct edgeLinkItem{ 
  sslink* dlink;
  int SSDfgEdge*
}
*/
void Schedule::calcAssignEdgeLink_single(SSDfgNode* dfgnode) {
  //_assignEdgeLink

  //paris of link to edge
  list<tuple<int, sslink*,SSDfgEdge*>> openset;

  auto node = location_of(dfgnode);

  if(!node.second) {
    //cerr << "SSDfgNode: " << dfgnode->name() << " is not scheduled\n";
    return;
  }
  
  for(auto source_dfgedge : dfgnode->in_edges()) {
    SSDfgNode* source_dfgnode = source_dfgedge->def();

    //route edge if source dfgnode is scheduled
    if(is_scheduled(source_dfgnode)) {
      for(auto& link: node.second->in_links()) {
        for (int slot=0; slot < 8; ++slot) {
          if (dfgNodeOf(slot, link) == source_dfgnode) {
            openset.emplace_back(make_tuple(slot, link, source_dfgedge));
          }
        }
      }
    }
  }
  /*

  
  ssnode::const_iterator Il,El;
  for(Il = node->ibegin(), El = node->iend(); Il!=El; ++Il) {
      sslink* link = *Il;
      if(dfgNodeOf(link,config)!=nullptr) {
        set<SSDfgEdge*>& edgelist = _assignEdgeLink[make_pair(link,config)];
        set<SSDfgEdge*>::iterator Ie,Ee;
        for(Ie=edgelist.begin(), Ee=edgelist.end(); Ie!=Ee; Ie++) {
          SSDfgEdge* dfgedge = *Ie;
          openset.push_back(make_pair(make_pair(link,config),source_dfgedge));
        }
      }
  }*/
  
  while(!openset.empty()) {
    int slot = get<0>(openset.front());
    sslink* cur_link = get<1>(openset.front());
    SSDfgEdge* cur_edge = get<2>(openset.front());
    ssnode* cur_node = cur_link->orig();
    SSDfgNode* cur_dfgnode = cur_edge->def();
    openset.pop_front();
    
    //cout << cur_link->name() << " gets " << cur_edge->name() << "\n";

    //_linkProp[cur_link->id()].edges.insert(cur_edge);
    assign_edgelink(cur_edge, slot, cur_link);
    
    for(auto &from_link: cur_node->in_links()) {

        if(from_link->orig()==_ssModel->subModel()->cross_switch()) continue;
        if(from_link->orig()==_ssModel->subModel()->load_slice()  ) continue;

        if(dfgNodeOf(slot, from_link)==cur_dfgnode) {
          openset.push_back(make_tuple(slot, from_link,cur_edge));
        }
    }
  }

}

void Schedule::calcAssignEdgeLink() {
  assert(0);

  //for(auto& i : _linkProp) {
  //  i.second.edges.clear();
  //}
  
  for (SSDfgInst* dfginst : _ssDFG->inst_vec()) {
    calcAssignEdgeLink_single(dfginst);
  }
  
  for (auto dfgout : _ssDFG->outputs()) {
    calcAssignEdgeLink_single(dfgout);
  }
  
  
  /*
  
  SubModel::const_input_iterator I,E;
  for(I=_ssModel->subModel()->input_begin(),
      E=_ssModel->subModel()->input_end(); I!=E; ++I) {
     ssinput* cand_input = const_cast<ssinput*>(&(*I));
    
    if(dfgNodeOf(cand_input,config)!=nullptr) {
       sslink* firstOutLink = cand_input->getFirstOutLink();
       openset.push_back(firstOutLink);
       lat_edge[firstOutLink]=0;
    }
  }
    
    
  
  while(!openset.empty()) {
    sslink* inc_link = openset.front(); 
    openset.pop_front();
    
    ssnode* node = inc_link->dest();
    ssnode::const_iterator I,E,II,EE;
    
    SSDfgNode* cur_dfgnode = dfgNodeOf(inc_link,config);
    assert(cur_dfgnode);
    
    if(ssfu* next_fu = dynamic_cast<ssfu*>(node)) {
      SSDfgNode* next_dfgnode = dfgNodeOf(node,config);
      //cout << next_fu->name() << "\n"; 
      assert(next_dfgnode);
      SSDfgInst* next_dfginst = dynamic_cast<SSDfgInst*>(next_dfgnode); 
      assert(next_dfginst);
    
      bool everyone_is_here = true;
      int latency=0;
      for(II = next_fu->ibegin(), EE = next_fu->iend(); II!=EE; ++II) {
        sslink* inlink = *II;
        if(dfgNodeOf(inlink,config) != nullptr) {
          if(lat_edge.count(inlink)==1) {
            if(lat_edge[inlink]>latency) {
              latency=lat_edge[inlink];
            }
          } else {
            everyone_is_here = false;
            break;
          }
        }
      }
      if(everyone_is_here) {
        sslink* new_link = next_fu->getFirstOutLink();
        lat_edge[new_link] = latency + inst_lat(next_dfginst->inst());;
        openset.push_back(new_link);
      }
    } else if (dynamic_cast<ssoutput*>(node)) {
      if(lat_edge[inc_link] > max_lat) {
        max_lat = lat_edge[inc_link];
      }
    } else {
    
      for(I = node->obegin(), E = node->oend(); I!=E; ++I) {
        sslink* link = *I;
        
        if(dfgNodeOf(link,config) == dfgNodeOf(inc_link,config)) {
          lat_edge[link] = lat_edge[inc_link] + 1;
          openset.push_back(link);
        }
      }
    }
  }
    

  return max_lat; 
  */
}

void Schedule::stat_printOutputLatency() {
  int n = _ssDFG->num_vec_output();
  cout << "** Output Vector Latencies **\n";
  for (int i = 0; i < n; i++) {
    SSDfgVecOutput *vec_out = _ssDFG->vec_out(i);
    cout << vec_out->gamsName() << ": ";
    for (auto dfgout: vec_out->outputs()) {
      cout << latOf(dfgout) << " ";
    }
    cout << endl;
  }
}

void Schedule::checkOutputMatch(int &max_lat_mis) {

  //int violation=0;

  for (int i=0; i<_ssDFG->num_vec_output(); i++) {
    SSDfgVecOutput* vec_out = _ssDFG->vec_out(i);
    int low_lat=10000, up_lat=0;
    for (auto dfgout: vec_out->outputs()) {
      int lat = latOf(dfgout);
      if(lat < low_lat) {
        low_lat = lat;
      }
      if(lat > up_lat) {
        up_lat = lat;
      }
      int diff = up_lat - low_lat;
      if(diff > max_lat_mis) {
        max_lat_mis = diff;
      }
    }

    //int min_vec_lat = 0;
    //int max_vec_lat = 1000000;
    //for(unsigned m=0; m < vec_out->num_outputs(); ++m) {
    //   SSDfgOutput* dfgout = vec_out->getOutput(m);

    //   auto p = _latBounds[dfgout];
    //   int min_inc_lat = p.first; 
    //   int max_inc_lat = p.second;
 
    //   //earliest starting time is *latest* incomming edge
    //   if(min_inc_lat > min_vec_lat) {
    //     min_vec_lat = min_inc_lat;
    //   }
    //   //latest starting time is *earliest* incomming edge
    //   if(max_inc_lat < max_vec_lat) {
    //     max_vec_lat = max_inc_lat;
    //   }
    //}

    //for(unsigned m=0; m < vec_out->num_outputs(); ++m) {
    //   SSDfgOutput* dfgout = vec_out->getOutput(m);
    //   assign_lat_bounds(dfgout,min_vec_lat,max_vec_lat);
    //}


    //if(min_vec_lat > max_vec_lat) {
    //  violation += min_vec_lat-max_vec_lat;
    //}
  }
  //add_violation(violation);
}

bool Schedule::fixLatency(int &max_lat, int &max_lat_mis) {
  for(auto& i : _edgeProp) {
    i.extra_lat=0;
  }

  if(num_left()) { //schedule is incomplete!
    cheapCalcLatency(max_lat, max_lat_mis, true);
    max_lat=1000;
    max_lat_mis=1000; //no point in optimizing for this yet
    return false;
  } else {
    //bool succ = fixLatency_fwd(max_lat, max_lat_mis); //fwd pass
    //int max_lat2=0,max_lat_mis2=0;
    //cheapCalcLatency(max_lat, max_lat_mis, true);
    //fixLatency_fwd(max_lat,max_lat_mis);
    //fixLatency_bwd(); //bwd pass
    iterativeFixLatency();

    max_lat=0;
    max_lat_mis=0;
    cheapCalcLatency(max_lat, max_lat_mis, false);

    //int max_lat2=0;
    //int max_lat_mis2=0;
    //calcLatency(max_lat2, max_lat_mis2);
    //checkOutputMatch(max_lat_mis2);

    //if(max_lat != max_lat2|| max_lat_mis!=max_lat_mis2) {
    //  cout << "max_lat: " << max_lat << " mis:" << max_lat_mis << "\n";
    //  cout << "max_lat2: " << max_lat2 << " mis2:" << max_lat_mis2 << "\n";
    //}

    //_ssDFG->printGraphviz("viz/remap-fail.dot");
    //printGraphviz("viz/sched.gv");
    //cout << "-------------------------------------------------------------------\n";
    //cout << "-------------------------------------------------------------------\n"; 
    //exit(1);

    //printGraphviz("viz/sched.gv");

    return max_lat_mis==0;
  }
}

bool Schedule::fixLatency_bwd() {
  int n = _ssDFG->num_vec_output();
  for (int i=0; i<n; i++) { //iterate over output vectors
    SSDfgVecOutput* vec_out = _ssDFG->vec_out(i);
    int maxLat = 0;
    
    for (auto dfgout : vec_out->outputs()) {
      if (latOf(dfgout) > maxLat) {
        maxLat = latOf(dfgout);
      }
    }
    unordered_set<SSDfgNode*> visited;
    //TODO: sort ports from small to large latency
    for (auto dfgout: vec_out->outputs()) {
      int ed = maxLat - latOf(dfgout); //extra delay to insert
      //cout<<"Output "<<dfgout->name()<<"  maxLat: "<<maxLat<<" _latOf: "<<_latOf[dfgout]<<" ed: "<<ed<<endl;
      if (ed > 0) {
        bool succ = fixDelay(dfgout, ed, visited);
        if (!succ) return false;
      }
    }
  }
  return true;
}


bool Schedule::fixDelay(SSDfgOutput* dfgout, int ed, unordered_set<SSDfgNode*>& visited) {

  SSDfgInst* n = dfgout->out_inst();
  if(!n || n->num_out()>1) { //bail if orig is input or has >1 use
    return false;
  }

  //assert(n && "output is always associated with an instruction node or input node");
  //cout<<"Fixing delay for node "<<n->name()<<" associated with output "<<dfgout->name()<<endl;
  if (visited.count(n) == 1) {
    cout<<"DFGNode "<<n->name()<<" is already visited!"<<endl;
    assert(0);
    return false;
  }
  visited.insert(n);

  for(auto source_dfgedge : n->in_edges()) {

    _edgeProp[source_dfgedge->id()].extra_lat = std::min(_ssModel->maxEdgeDelay(),
        _edgeProp[source_dfgedge->id()].extra_lat + ed);

    if (_edgeProp[source_dfgedge->id()].extra_lat + ed > _ssModel->maxEdgeDelay()) {
      return false;
    }
  }
  return true;
}


bool Schedule::fixLatency_fwd(int &max_lat, int &max_lat_mis) {
  list<sslink*> openset;
  //map<ssnode*,sslink*> came_from;
  map<sslink *, int> lat_edge;

  max_lat = 0;
  max_lat_mis = 0;

  for (auto elem : _ssModel->subModel()->inputs()) {
    ssinput *cand_input = const_cast<ssinput *>(elem);

    SSDfgNode *dfgnode = dfgNodeOf(cand_input);
    if (dfgnode != nullptr) {
      sslink *firstOutLink = cand_input->getFirstOutLink();
      assert(firstOutLink);
      openset.push_back(firstOutLink);
      lat_edge[firstOutLink] = latOf(dfgnode);
    }
  }


  //Outlinks of all the inputs
  while (!openset.empty()) {
    sslink *inc_link = openset.front();
    openset.pop_front();

    //dest node
    ssnode *node = inc_link->dest();

    if (ssfu *next_fu = dynamic_cast<ssfu *>(node)) {
      SSDfgNode *next_dfgnode = dfgNodeOf(node);
      //cout << next_fu->name() << "\n"; 
      if (!next_dfgnode && !isPassthrough(node)) {
        cout << "problem with latency calculation!\n";
        cout << node->name() << " has no mapping\n";
        max_lat = -1;
        max_lat_mis = -1;
        assert(next_dfgnode);
        return false;
      }

      SSDfgInst *next_dfginst = dynamic_cast<SSDfgInst *>(next_dfgnode);
      assert(next_dfginst || isPassthrough(node));

      bool everyone_is_here = true;

      int latency = 0;
      int low_latency = 100000000;  //magic number, forgive me
      for (auto &inlink: next_fu->in_links()) {
        for (int slot = 0; slot < 8; ++slot) {
          if (dfgNodeOf(slot, inlink) != nullptr) {
            if (lat_edge.count(inlink) == 1) {
              if (lat_edge[inlink] > latency) {
                latency = lat_edge[inlink];
              }
              if (lat_edge[inlink] < low_latency) {
                low_latency = latency;
              }
            } else {
              everyone_is_here = false;
              break;
            }
          }
        }
      }

      if (everyone_is_here && !isPassthrough(node)) {
        for (auto &inlink: next_fu->in_links()) {
          for (int slot = 0; slot < 8; ++slot) {
            SSDfgNode *origNode = dfgNodeOf(slot, inlink);
            if (origNode != nullptr) {
              SSDfgEdge *edge = origNode->getLinkTowards(next_dfgnode);
              if (lat_edge[inlink] < latency) {
                int diff = latency - lat_edge[inlink]; //latency per edge
                assert(edge);
                if (diff > _ssModel->maxEdgeDelay()) {

                  //cout << diff  << " > " << _ssModel->maxEdgeDelay() << "\n";
                  return false;
                }
                set_edge_delay(diff, edge);
              }
              //cout<<"Edge Name: "<<edge->name()<< " extra lat:" << _extraLatOfEdge[edge] << endl;
              //cout << "(fwdPass) Edge "<<edge->name()<<"  maxLat: " << latency << "  lat_edge: "<<lat_edge[inlink]<<"  extralat: " << _extraLatOfEdge[edge] << "\n";
            }
          }
        }
        //cout << next_fu->name() << " latency " << latency <<"\n";
      }

      if (everyone_is_here) {
        sslink *new_link = next_fu->getFirstOutLink();
        if (isPassthrough(node)) {
          lat_edge[new_link] = latency + 1; //TODO: Check this
        } else { //regular inst
          lat_edge[new_link] = latency + inst_lat(next_dfginst->inst());
        }
        openset.push_back(new_link);

        int diff = latency - low_latency;
        if (diff > max_lat_mis) {
          max_lat_mis = diff;
        }
      }
    } else if (dynamic_cast<ssoutput *>(node)) {
      ssoutput *ss_out = dynamic_cast<ssoutput *>(node);
      SSDfgNode *dfgnode = dfgNodeOf(ss_out);
      assign_lat(dfgnode, lat_edge[inc_link]);

      if (lat_edge[inc_link] > max_lat) {
        max_lat = lat_edge[inc_link];
      }
    } else {
      for (auto &link : node->out_links()) {
        for (int slot = 0; slot < 8; ++slot) {
          if (dfgNodeOf(slot, link) == dfgNodeOf(0, inc_link)) {
            lat_edge[link] = lat_edge[inc_link] + 1;
            openset.push_back(link);
          }
        }
      }
    }
  }
  return true;
}

std::vector<SSDfgInst*> Schedule::ordered_non_temporal() {
  std::vector<SSDfgInst*> res;
  for (SSDfgInst *i : _ssDFG->ordered_insts()) {
    if (!i->is_temporal()) {
      res.push_back(i);
    }
  }
  return res;
}


void Schedule::iterativeFixLatency() {
  bool changed = true;
  reset_lat_bounds();

  int max_ed = _ssModel->maxEdgeDelay();
  int iters = 0;

  bool overflow = false;
  int max_mis = 0;

  // TODO: We didn't quite fix the problem of IO vectors in temporal region, though hopefully this isn't necessary

  std::vector<SSDfgInst *> ordered_non_temp = ordered_non_temporal();

  while (changed || overflow) {
    changed = false;

    iters++;
    if (overflow) {
      overflow = false;
      reset_lat_bounds();
      max_mis++;
    }

    //FORWARD PASS -- INSTS
    for (SSDfgInst *inst : ordered_non_temp) {
      auto &vp = _vertexProp[inst->id()];
      int new_min = vp.min_lat;
      int new_max = vp.max_lat;

      for (auto edge : inst->in_edges()) {
        SSDfgNode *origNode = edge->def();
        auto &orig_vp = _vertexProp[origNode->id()];

        int edge_lat = origNode->lat_of_inst() + link_count(edge) - 1;

        //cout << " -----------------" <<  edge->name() << ": " << edge_lat << "\n";

        new_min = std::max(new_min, orig_vp.min_lat + edge_lat);
        new_max = std::min(new_max, orig_vp.max_lat + edge_lat + max_ed + max_mis);
      }
      changed |= new_min != vp.min_lat;
      changed |= new_max != vp.max_lat;
      vp.min_lat = new_min;
      vp.max_lat = new_max;

      if (new_min > new_max) {
        overflow = true;
        break;
      }

      //cout << inst->name() << "  min_lat:" << vp.min_lat 
      //                     << " max_lat:"<< vp.max_lat << "\n";
    }

    if (overflow) continue;

    //FORWARD PASS -- VEC_OUTPUTS 
    for (int i = 0; i < _ssDFG->num_vec_output(); i++) {
      SSDfgVecOutput *vec_out = _ssDFG->vec_out(i);
      auto &vecp = _vecProp[vec_out];
      int new_min = vecp.min_lat;
      int new_max = vecp.max_lat;


      for (auto dfgout : vec_out->outputs()) {
        SSDfgEdge *edge = dfgout->first_inc_edge();

        SSDfgNode *origNode = edge->def();
        auto &orig_vp = _vertexProp[origNode->id()];

        int edge_lat = origNode->lat_of_inst() + link_count(edge) - 1;

        //cout << " -----------------" <<  edge->name() << ": " << edge_lat << "\n";

        //remove max_ed
        new_min = std::max(new_min, orig_vp.min_lat + edge_lat);
        new_max = std::min(new_max,
                           orig_vp.max_lat + edge_lat + max_mis); //no max_ed here because these are output nodes
      }
      changed |= new_min != vecp.min_lat;
      changed |= new_max != vecp.max_lat;
      vecp.min_lat = new_min;
      vecp.max_lat = new_max;

      for (auto dfgout : vec_out->outputs()) {
        auto &vp = _vertexProp[dfgout->id()];
        vp.min_lat = new_min;
        vp.max_lat = new_max;
      }

      if (new_min > new_max) {
        overflow = true;
        break;
      }
      //cout << vec_out->name() << "  min_lat:" << vecp.min_lat 
      //                        << " max_lat:"<< vecp.max_lat << "\n";
    }

    if (overflow) continue;

    //BACKWARDS PASS
    for (int i = ordered_non_temp.size() - 1; i >= 0; i--) {
      SSDfgInst *inst = ordered_non_temp[i];
      auto &vp = _vertexProp[inst->id()];
      int new_min = vp.min_lat;
      int new_max = vp.max_lat;

      for (auto edge : inst->uses()) {
        if (edge == nullptr) continue;
        SSDfgNode *useNode = edge->use();
        auto &use_vp = _vertexProp[useNode->id()];

        int edge_lat = link_count(edge) - 1 + inst->lat_of_inst();
        new_min = std::max(new_min, use_vp.min_lat - edge_lat - max_ed - max_mis);
        new_max = std::min(new_max, use_vp.max_lat - edge_lat);
      }
      changed |= new_min != vp.min_lat;
      changed |= new_max != vp.max_lat;
      vp.min_lat = new_min;
      vp.max_lat = new_max;

      if (new_min > new_max) {
        overflow = true;
        break;
      }
      //cout << inst->name() << "  min_lat:" << vp.min_lat 
      //                     << " max_lat:"<< vp.max_lat << " (b) \n";

    }
  }

  //cout << "iters until converge: " << iters << ", mismatch: " << max_mis << "\n";
  // NOW SET THE LATENCY!

  for (SSDfgInst *inst : ordered_non_temp) {
    auto &vp = _vertexProp[inst->id()];
    //cout << inst->name() << "  min_lat:" << vp.min_lat << " max_lat:" 
    //                                    << vp.max_lat << "\n";
    int target = vp.min_lat;;// < vp.max_lat ? vp.min_lat : 
    //              (vp.min_lat + vp.max_lat) / 2;
    //cout << "target : " << target << "\n";

    int max = 0;
    int mis = 0;
    for (auto edge : inst->in_edges()) {
      if (edge == nullptr) continue;
      SSDfgNode *origNode = edge->def();

      int lat = latOf(origNode) + link_count(edge) - 1;
      int diff = std::max(std::min(_ssModel->maxEdgeDelay(), target - lat), 0);
      mis = std::max(mis, (target - lat) - diff);
      set_edge_delay(diff, edge);

      max = std::max(max, lat + diff);
      //cout << " -- " << origNode->name() << "diff"  << diff 
      //                         << "links:" << link_count(edge)-1 << "\n";
    }
    //cout << " * mis: " << mis << "\n";
    assign_lat(inst, inst->lat_of_inst() + max);
  }

  for (int i = 0; i < _ssDFG->num_vec_output(); i++) {
    SSDfgVecOutput *vec_out = _ssDFG->vec_out(i);
    auto &vp = _vecProp[vec_out];

    //cout << vec_out->name() << "  min_lat:" << vp.min_lat << " max_lat:" 
    //                                   << vp.max_lat << "\n";

    int target = vp.min_lat; // < vp.max_lat;// ? vp.min_lat : 
    //  (vp.min_lat + vp.max_lat) / 2;

    //cout << "target : " << target << "\n";

    int mis = 0;
    for (auto dfgout: vec_out->outputs()) {
      SSDfgEdge *edge = dfgout->first_inc_edge();

      SSDfgNode *origNode = edge->def();

      int lat = latOf(origNode) + link_count(edge) - 1;
      int diff = std::max(std::min(_ssModel->maxEdgeDelay(), target - lat), 0);
      mis = std::max(mis, (target - lat) - diff);

      set_edge_delay(diff, edge);
      //cout << origNode->name() << " lat:" << lat << " diff"  << diff 
      //                                 << " links:" << link_count(edge)-1 << "\n";
    }
    //cout << " * mis: " << mis << "\n";

  }
}

void Schedule::cheapCalcLatency(int &max_lat, int &max_lat_mis, bool set_delay) {
  _totalViolation=0;
  max_lat_mis=0;
  max_lat=0;
  _groupMismatch.clear();

  std::vector<SSDfgInst*> ordered_non_temp = ordered_non_temporal();

  for(SSDfgInst* inst : ordered_non_temp) {
    calcNodeLatency(inst,max_lat,max_lat_mis,set_delay);
  }

  for (int i=0; i<_ssDFG->num_vec_output(); i++) {
    SSDfgVecOutput* vec_out = _ssDFG->vec_out(i);
    int low_lat=MAX_SCHED_LAT, up_lat=0;

    if(vecMapped(vec_out)) {
      for (auto dfgout : vec_out->outputs()) {
        SSDfgEdge* edge = dfgout->first_inc_edge();
  
        if(is_scheduled(dfgout)) {
          int lat = latOf(edge->def()) + edge_delay(edge) + link_count(edge)-1;
          //cout << edge->name() << "\n";
          //cout << edge->def()->name() << "\n";
          //cout << edge->use()->name() << "\n";
          //cout << "C " << vec_out->name() << " " << lat << " -- "
          //     << " def_lat:" << latOf(edge->def()) << " ed:" << edge_delay(edge)
          //     << " link_count:" << link_count(edge) << "inst: "
          //     << edge->def() << " " << edge->def()->name() << "\n";

          assign_lat(dfgout,lat);

          if(lat>up_lat) up_lat=lat;
          if(lat<low_lat) low_lat=lat;
        }
      }
    }

    assign_lat_bounds(vec_out,low_lat,up_lat); //turn off, just for debug
 
    int diff = up_lat - low_lat;

    if(!vec_out->is_temporal()) {
      if(diff>max_lat_mis) max_lat_mis=diff;
    }

    if(max_lat < up_lat) max_lat=up_lat;
 
    if(diff > 0) {
      add_violation(diff);
    } 

    /*
    //Don't play with delay on output nodes because no delay fifos
    if(set_delay && vecMapped(vec_out)) { 
      for (unsigned m=0; m < vec_out->num_outputs(); ++m) {
        SSDfgOutput* dfgout = vec_out->getOutput(m);
        SSDfgEdge* edge = dfgout->first_inc_edge();
  
        if(is_scheduled(dfgout)) {
          int lat = latOf(edge->def()) + link_count(edge)-1; 
          int diff = std::min(_ssModel->maxEdgeDelay(), up_lat - lat);
          set_edge_delay(diff,edge);
        }
      }
    }
    */


    //cout << "C " <<vec_out->name()<< " up:" << up_lat << " low:" << low_lat << "\n";
  }
}

void Schedule::calcNodeLatency(SSDfgInst* inst, int &max_lat, int &max_lat_mis, 
    bool set_delay) {
  int low_lat=MAX_SCHED_LAT, up_lat=0;

  if(is_scheduled(inst)) {
    for(auto edge : inst->in_edges()) {

      SSDfgNode* origNode = edge->def();

      if(is_scheduled(origNode)) {
        if (origNode != nullptr) {
          int edge_lat = edge_delay(edge) + link_count(edge)-1;
          assert(edge_lat >= 0);
          assert(edge_delay(edge)==0 || set_delay == 0);
          int lat = latOf(origNode) + edge_lat; 

          if(lat>up_lat) up_lat=lat;
          if(lat<low_lat) low_lat=lat;
        }
      }
    }
  }

  assign_lat_bounds(inst,low_lat,up_lat); //FIXME: turn off, just for debug

  int diff = up_lat - low_lat; // - _ssModel->maxEdgeDelay();

  if(diff>max_lat_mis) {max_lat_mis=diff;}
  if(diff>_groupMismatch[inst->group_id()]) {
    _groupMismatch[inst->group_id()] = diff;
  }

  int new_lat = inst->lat_of_inst() + up_lat;
  assign_lat(inst,new_lat);

  //ssnode* n = locationOf(inst);
  //cout << "C " << inst->name() << " node: " << n->name() 
  //  << " low_lat: " << low_lat << " up_lat:" << up_lat << " latmis:" 
  //  << max_lat_mis << "diff: " << diff << "\n";
  
               
  if(max_lat < new_lat) max_lat=new_lat;

  if(diff > 0) {
    add_violation(diff);
    record_violation(inst,diff);
  }

  //Set the delay based on a simple model
  if(set_delay && is_scheduled(inst)) {
    for (auto edge : inst->in_edges()) {

      SSDfgNode* origNode = edge->def();
      if(is_scheduled(origNode)) {
        if (origNode != nullptr) {
          int lat = latOf(origNode) + link_count(edge)-1; 
          int diff = std::min(_ssModel->maxEdgeDelay(), up_lat - lat);
          set_edge_delay(diff,edge);
        }
      }
    }
  }
}


void Schedule::calcLatency(int &max_lat, int &max_lat_mis, bool warnMismatch) {
  queue<sslink*> openset;
  //map<ssnode*,sslink*> came_from;
  unordered_map<sslink*,int> lat_edge;
  
  max_lat=0;  
  max_lat_mis=0;

  for(auto elem : _ssModel->subModel()->inputs()) {
    ssinput *cand_input = const_cast<ssinput *>(elem);

    SSDfgNode *dfgnode = dfgNodeOf(cand_input);
    if (dfgnode != nullptr) {
      sslink *firstOutLink = cand_input->getFirstOutLink();
      openset.push(firstOutLink);
      lat_edge[firstOutLink] = latOf(dfgnode);
    }
  }
    
 //Outlinks of all the inputs 
  while(!openset.empty()) {
    sslink* inc_link = openset.front(); 
    openset.pop();
    //cout << inc_link->name() << "\n";   

    //dest node
    ssnode* node = inc_link->dest();
    
    if(ssfu* next_fu = dynamic_cast<ssfu*>(node)) {
      sslink* new_link = next_fu->getFirstOutLink();
      if(lat_edge.count(new_link)) {
        //cout << "skipping because full:" << new_link->name() << "\n";
        continue; //skip if we've done it already
      }

      SSDfgNode* next_dfgnode = dfgNodeOf(node);
      //cout << next_fu->name() << "\n"; 
      if(!next_dfgnode && !isPassthrough(node)) {
        assert(next_dfgnode);
        cout << "problem with latency calculation!\n";
        max_lat=-1;
        max_lat_mis=-1;
        return;
      }

      SSDfgInst* next_dfginst = dynamic_cast<SSDfgInst*>(next_dfgnode); 
      assert(next_dfginst || isPassthrough(node));
    
      bool everyone_is_here = true;

      for(auto &inlink: next_fu->in_links()) {
        for (int slot = 0; slot < 8; ++slot) {
          if (dfgNodeOf(slot, inlink) != nullptr) {
            if (!lat_edge.count(inlink)) {
              everyone_is_here = false;
              break;
            }
          }
        }
      }

      int max_latency=0;
      int low_latency=100000000;  //magic number, forgive me

 
      if (everyone_is_here) {
        //cout << "----------------------------------- DONE WITH " 
        //     <<  next_fu->name() << "\n";
        // Latency should be the same across all the incoming edges
        for (auto &inlink: next_fu->in_links()) {
          for (int slot = 0; slot < 8; ++slot) {
            SSDfgNode *origNode = dfgNodeOf(slot, inlink);
            if (origNode != nullptr) {
              int curLat = lat_edge[inlink];
              //cout << "reading: " << inlink->name() << "\n";

              if (!isPassthrough(node)) {
                SSDfgEdge *edge = origNode->getLinkTowards(next_dfgnode);
                if (!edge) {
                  cout << "Edge: " << origNode->name() << " has no edge towards "
                       << next_dfgnode->name() << ", for link:"
                       << inlink->name() << "\n";
                  if (next_dfginst->isDummy()) cout << "dummy!\n";

                  _ssDFG->printGraphviz("viz/remap-fail2.dot");
                  printGraphviz("viz/remap-fail2.gv");

                  assert(edge);
                }
                if (edge_delay(edge)) {
                  curLat += edge_delay(edge);
                }
              }

              if (curLat > max_latency) {
                max_latency = curLat;
              }
              if (curLat < low_latency) {
                low_latency = curLat;
              }

              if (warnMismatch && max_latency != low_latency) {
                cout << "Mismatch, min_lat:" << low_latency
                     << ", max_lat:" << max_latency
                     << ", link:" << inlink->name() << "\n";
                if (!isPassthrough(node)) {
                  SSDfgEdge *edge = origNode->getLinkTowards(next_dfgnode);
                  cout << "(calcLat) Edge " << edge->name() << "  lat_edge: " << lat_edge[inlink] << "  extralat:"
                       << edge_delay(edge) << "\n";
                } else {
                  cout << "passthrough\n";
                }

              }
            }
          }
        }
      }

      if (everyone_is_here) {
        // Update latency of outgoing edge
        if (isPassthrough(node)) {
          lat_edge[new_link] = max_latency + 1; //TODO: Check this
        } else { //regular inst
          int l = max_latency + inst_lat(next_dfginst->inst());
          lat_edge[new_link] = l;

          //if(next_dfginst) {
          //  cout << "L " << next_dfginst->name() << " lat:" << l
          //       << " low:" << low_latency << " up:" << max_latency << " - "
          //       << " lat:" << latOf(next_dfgnode)
          //       << " low:" << lat_bounds(next_dfgnode).first << " up:"
          //       << lat_bounds(next_dfgnode).second << "\n";
          //}


          assign_lat(next_dfgnode,l);
        }

        //if(next_dfginst) {
        //  cout << "L " << next_dfginst->name() << " " << latOf(next_dfgnode) 
        //       << " up:" << max_latency << " low:" << low_latency << "\n";
        //}

        openset.push(new_link);

        //cout << "lat of " << next_dfgnode->name() 
        //     << ", old:" << _latOf[next_dfgnode]
        //     << ", new:" << max_latency << " " << lat_edge[new_link] << "\n";

        int diff = max_latency-low_latency;
        if(diff>max_lat_mis) {
          max_lat_mis=diff;
        }
      }
    } else if (dynamic_cast<ssoutput*>(node)) {
      ssoutput* ss_out = dynamic_cast<ssoutput*>(node);
      SSDfgNode* dfgnode = dfgNodeOf(ss_out);
      SSDfgEdge* inc_edge = dfgnode->first_inc_edge();

      int l = lat_edge[inc_link] + edge_delay(inc_edge);

      //cout<<"L output " << dfgnode->name() << " "
      //       <<inc_link->name()<<" lat: "<< l << " - " << 
      //       "lat: " << latOf(dfgnode) << endl; 

      assign_lat(dfgnode,l);

      dfgnode->set_sched_lat(l); //for graphviz printing

      if(lat_edge[inc_link] > max_lat) {
        max_lat = lat_edge[inc_link];
      }
    } else {

      for (auto &out_link : node->out_links()) {
        for (int slot = 0; slot < 8; ++slot) {

          //We'll need to check to make sure if there is ambiguity between links
          if (have_switch_links()) {
            ssswitch *sssw = dynamic_cast<ssswitch *>(node);
            assert(sssw);
            sslink *new_inc_link = get_switch_in_link(sssw, out_link);
            if (new_inc_link != inc_link) {
              continue;
            }
            SSDfgNode *n_out = dfgNodeOf(slot, out_link);
            SSDfgNode *n_inc = dfgNodeOf(0, inc_link);

            if (n_out != n_inc) {
              cout << "ERROR: Links don't match!  "
                   << out_link->name() << "is assigned "
                   << (n_out ? n_out->name() : "nothing, and ")
                   << inc_link->name() << "is assigned "
                   << (n_inc ? n_inc->name() : "nothing")
                   << "\n";
            }
            printGraphviz("viz/fail-link-match.gv");
          } else if (dfgNodeOf(slot, out_link) != dfgNodeOf(0, inc_link)) {
            continue;
          }

          lat_edge[out_link] = lat_edge[inc_link] + 1;
          openset.push(out_link);
        }
      }
    }
  }

  for(auto& i : lat_edge) {
    sslink *link = i.first;
    int lat = i.second;
    set_link_order(0, link, lat);
  }

  _max_lat=max_lat;
  _max_lat_mis=max_lat_mis; 

}

//Trace the path of a schedule to help re-create the DFG
//This is not a high-performance implementation, but shouldn't have to be
//because its not called often
void Schedule::tracePath(ssnode* ssspot, SSDfgNode* dfgnode, 
    map<ssnode*, map<SwitchDir::DIR,SwitchDir::DIR> >& routeMap,
    map<ssnode*, SSDfgNode* >& dfgnode_for, 
    map<SSDfgNode*, vector<SwitchDir::DIR> >& posMap) {

  //FIXME: later put the value makes more sense
  assign_node(dfgnode, make_pair(0, ssspot));
  
  vector<tuple<ssnode*, SwitchDir::DIR, std::vector<sslink*>> > worklist;

  sslink* firstLink = ssspot->getFirstOutLink();

  //FIXME: assign_link(dfgnode,firstLink); //TODO: Check if broke anything
  std::vector<sslink*> lvec;
  lvec.push_back(firstLink);

  ssnode* startItem = firstLink->dest();
  SwitchDir::DIR initialDir = firstLink->dir();
  worklist.push_back(make_tuple(startItem,initialDir,lvec));

  //cerr << "---   tracing " << ssspot->name() << "   ---\n"; 
  
  while(!worklist.empty()) {
    
    //cerr << "worklist: ";
    //for(unsigned i = 0; i < worklist.size(); ++i) {
    //  ssnode* item = worklist[i].first;
    //  SwitchDir::DIR dir = worklist[i].second;
    //  cerr << SwitchDir::dirName(dir) << ", " << item->name() << "";
    //  cerr << " | ";
    //}
    //cerr << "\n";
   
    auto& item = worklist.back();
    ssnode* curItem = std::get<0>(item);
    SwitchDir::DIR inDir = std::get<1>(item);

    auto item_links = std::get<2>(item);
    worklist.pop_back();
    
    map<SwitchDir::DIR,SwitchDir::DIR>::iterator I,E;
    for(I=routeMap[curItem].begin(), E=routeMap[curItem].end(); I!=E; ++I) {
      SwitchDir::DIR newOutDir = I->first;
      SwitchDir::DIR newInDir = I->second;
      
      if(inDir == newInDir) { //match!

        //sslink* inLink = curItem->getInLink(newInDir);
        
        sslink* outLink = curItem->getOutLink(newOutDir);
        //FIXME: assign_link(dfgnode,outLink);

        auto links = item_links;
        links.push_back(outLink);
        
        if(outLink==nullptr) {
          cerr << "outlink is null: ";
          cerr << curItem->name() << " (dir:" << SwitchDir::dirName(newOutDir) << "\n";
          assert(0);
        }

        ssnode* nextItem = outLink->dest();

        //cerr << "match" << curItem->name() << " (dir:" << SwitchDir::dirName(newOutDir)
        //     << " " << nextItem->name() << "\n";


       //Output node 
        if(ssoutput* ssout = dynamic_cast<ssoutput*>(nextItem)) {
          //cerr << SwitchDir::dirName(newInDir) << " -> " << SwitchDir::dirName(newOutDir)
          //     << "    out port" << ssout->port() << "\n";

          SSDfgNode* dest_dfgnode = dfgnode_for[ssout];
          assert(dest_dfgnode);
          
          //_assignNode[ssout]=dest_dfgnode;  //perform the assignment
          //_ssnodeOf[dest_dfgnode]=ssout;
          assign_node(dest_dfgnode, make_pair(0, ssout));

          SSDfgEdge* edge = _ssDFG->connect(dfgnode,dest_dfgnode,0, SSDfgEdge::data); 
          for(auto& i : links) {
            //FIXME: for now, we do not use this function to rebuild the schedule, so I just put some random value for the slot
            assign_edgelink(edge, 0, i);
          }

        } else if(ssfu* fu_node = dynamic_cast<ssfu*>(nextItem)) {

          //cerr << SwitchDir::dirName(newInDir) << " -> " << SwitchDir::dirName(newOutDir)
          //     << "    (" << fu_node->x() << " " << fu_node->y() << " .. FU)" << "\n";

          SSDfgInst* dest_dfgnode = dynamic_cast<SSDfgInst*>(dfgnode_for[fu_node]);
          assert(dest_dfgnode);

          int n_ops = SS_CONFIG::num_ops[dest_dfgnode->inst()];

          for(int slot = 0; slot < NUM_IN_FU_DIRS; ++slot) { 
            if(posMap[dest_dfgnode][slot] == outLink->dir()) {
              auto edge_type = SSDfgEdge::data;
              if(slot==n_ops) edge_type = dest_dfgnode->predInv()? 
                             SSDfgEdge::ctrl_false : SSDfgEdge::ctrl_true;

              SSDfgEdge * edge = _ssDFG->connect(dfgnode,dest_dfgnode, slot, edge_type);
              //FIXME: put a placeholder for the slot to pass compilation first. Later we will use this function to reconstruct the scheudle
              for(auto& i : links) {
                assign_edgelink(edge, 0, i);
              }
            }
          }
        
        } else if(dynamic_cast<ssswitch*>(nextItem)){ //must be switch
          worklist.emplace_back(make_tuple(nextItem,newOutDir,links));
        } else {
          assert(0);
        }
      }
    }
  }
}

template<typename T>
int count_unique(std::vector<T> &vec) {
  sort(vec.begin(), vec.end());
  return unique(vec.begin(), vec.end()) - vec.begin();
}

void Schedule::get_overprov(int& ovr, int& agg_ovr, int& max_util) {
  ovr = 0;
  agg_ovr = 0;
  max_util = 0;

  for (auto v: _vertexProp) {
    if (v.node) {
      const auto &np = _nodeProp[v.node->id()];
      int cnt[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      int max_cnt = 0;
      vector<SSDfgVec *> io;
      for (auto elem: np.vertices) {
        if (elem.second->is_temporal()) {
          if (auto in = dynamic_cast<SSDfgInput *>(elem.second))
            io.push_back(in->input_vec());
          else if (auto out = dynamic_cast<SSDfgOutput *>(elem.second))
            io.push_back(out->output_vec());
        } else {
          max_cnt = max(max_cnt, ++cnt[elem.first]);
        }
      }
      int unique_io = count_unique(io);

      //Calculate aggregate overage
      for(int i = 0; i<8; ++i) {
        int cur_util = cnt[i] + np.num_passthroughs + unique_io;
        int cur_ovr = cur_util - v.node->max_util();
        agg_ovr += std::max(cur_ovr, 0);
        ovr = max(ovr, cur_ovr);
      }
      max_cnt += unique_io + np.num_passthroughs;
      max_util = max(max_util, max_cnt);
    }
  }

  for (auto &n : _ssModel->subModel()->node_list()) {
    for (auto &elem: n->out_links()) {
      for (int slot = 0; slot < 8; ++slot) {
        auto &lp = _linkProp[elem->id()];
        int util = 0;
        //if (!lp.slots[slot].edges.empty())
          //std::cout << elem->name() << " " << slot << "\n";

        std::vector<SSDfgVec *> vecs;
        std::vector<SSDfgNode*> nodes;

        for (auto edge : lp.slots[slot].edges) {
          //std::cout << edge->name() << "\n";
          auto v = edge->def();
          if (v->is_temporal()) {
            if (auto input = dynamic_cast<SSDfgInput *>(v)) {
              vecs.push_back(input->input_vec());
              continue;
            }
            for (auto use : v->uses()) {
              if (auto *out = dynamic_cast<SSDfgOutput *>(use->use())) {
                vecs.push_back(out->output_vec());
                continue;
              }
            }
          } else {
            nodes.push_back(v);
          }
        }
        util = count_unique(nodes) + count_unique(vecs);
        int cur_ovr = util - n->max_util();
        ovr = std::max(cur_ovr, ovr);
        agg_ovr += std::max(cur_ovr, 0);
        max_util = std::max(util, max_util);
      }
    }
  }

}

