#include "scheduler_gams.h"

using namespace SS_CONFIG;
using namespace std;

#include <unordered_map>
#include <fstream>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include "model_parsing.h"

#include "gams_models/softbrain_gams.h"
#include "gams_models/softbrain_gams_hw.h"
#include "gams_models/spill_model.h"
#include "gams_models/multi_model.h"
#include "gams_models/single_fixed_general.h"
#include "gams_models/timing_model.h"
#include "gams_models/hw_model.h"
#include "gams_models/stage_model.h"


#include "scheduler_sa.h"

void GamsScheduler::print_mipstart(ofstream& ofs,  Schedule* sched, SSDfg* ssDFG, 
                                   bool fix) {

  sched->xfer_link_to_switch(); // makes sure we have switch representation of routing

  int config=0;
  //Mapping Variables
  for (auto dfgnode : ssDFG->nodes()) {
    ssnode* spot = sched->locationOf(dfgnode);
    ofs << "Mn.l('" << dfgnode->gamsName() 
        << "','" << spot->gams_name(config) << "')=1;\n";

    for(auto dfgedge : dfgnode->uses()) {
      auto& link_set = sched->links_of(dfgedge);
    
      for(auto elem : link_set) {
        sslink* link = elem.second;
        ofs << "Mvl.l('" << dfgnode->gamsName() 
            << "','" << link->gams_name(config) << "')=1;\n";
        ofs << "Mel.l('" << dfgedge->gamsName() << "','" 
            << link->gams_name(config) << "')=1;\n";

        int order = sched->link_order(elem);
        if(order!=-1) {
          ofs << "O.l('" << link->gams_name(config) << "')=" << order << ";\n";
        }

      }
    }
  }

  vector< vector<ssswitch*> >& switches = _ssModel->subModel()->switches();
  for(int i = 0; i < _ssModel->subModel()->sizex()+1; ++i) {
    for(int j = 0; j < _ssModel->subModel()->sizey()+1; ++j) {
      ssswitch* sssw = switches[i][j];
      auto link_map = sched->link_map_for_sw(sssw);
      for(auto I=link_map.begin(), E=link_map.end();I!=E;++I) {
        sslink* outlink=I->first;
        sslink* inlink=I->second;
        ofs << "Sll.l('" << inlink->gams_name(config) << "','"
                         << outlink->gams_name(config) << "')=1;\n";
      }
    }//end for switch x
  }//end for switch y            

  //Ports
  for(int i = 0; i < ssDFG->num_vec_input(); ++i) {
    SSDfgVecInput* vec_in = ssDFG->vec_in(i);
    pair<bool,int> vecPort = sched->vecPortOf(vec_in); 
    ofs << "Mp.l('" << vec_in->gamsName() << "','"
        << "ip" << vecPort.second << "')=1;\n";
  }

  for(int i = 0; i < ssDFG->num_vec_output(); ++i) {
    SSDfgVecOutput* vec_out = ssDFG->vec_out(i);
    pair<bool,int> vecPort = sched->vecPortOf(vec_out); 
    ofs << "Mp.l('" << vec_out->gamsName() << "','"
        << "op" << vecPort.second << "')=1;\n";
  }

  //Ordering
  //auto link_order = sched->get_link_prop();
  //for(auto i : link_order) {
  //  sslink* l = i.first;
  //  int order = i.second.order;
  //  if(order!=-1) {
  //    ofs << "O.l('" << l->gams_name(config) << "')=" << order << ";\n";
  //  }
  //}

  //Timing
  int d1,d2;
  sched->calcLatency(d1,d2); //make sure stuff is filled in
  for (auto n : ssDFG->nodes()) {
    int l = sched->latOf(n);
    if(SSDfgInst* inst = dynamic_cast<SSDfgInst*>(n)) {
      l -= inst_lat(inst->inst());
    }
    ofs << "Tv.l('" << n->gamsName() << "')=" << l << ";\n";
  }

  //Mp.fx(pv,pn) = round(Mp.l(pv,pn));
  //Mn.fx(v,n) = round(Mn.l(v,n));

  ofs << "loop((e,n),\n"
      << "  if(sum(l$Hln(l,n),Mel.l(e,l)) and sum(l$Hnl(n,l), Mel.l(e,l)),\n"
      << "    PTen.l(e,n)=1;\n"
      << "  );\n"
      << ");\n\n";

  ofs << "loop((v1,e,v2)$(Gve(v1,e) and Gev(e,v2)),\n"
      << "extra.l(e) = Tv.l(v2) - Tv.l(v1) - sum(l,Mel.l(e,l)) - delta(e) + 1;\n"
      << "extra.l(e) = min(extra.l(e),  max_edge_delay * (1 + sum(n, PTen.l(e,n))))"
      << ");\n\n";


  ofs << "minTpv.l(pv)=10000;" 
      << "maxTpv.l(pv)=0;" 
      << "loop((pv,v)$(VI(pv,v) <> 0 and KindV('Output',v)),\n"
      << "  minTpv.l(pv)=min(Tv.l(v),minTpv.l(pv));\n"
      << "  maxTpv.l(pv)=max(Tv.l(v),maxTpv.l(pv));\n"
      << ");\n\n";

  //ofs << "Tv.l(v2) = smax((v1,e)$(Gve(v1,e) and Gev(e,v2)),Tv.l(v1) + sum(l,Ml.l(e,l))) + delta.l(e);\n";
  //
  ofs << "Nl.l(l) = 1 - sum(v,Mvl.l(v,l));\n";
  ofs << "length.l=smax(v,Tv.l(v));\n";
  ofs << "cost.l = length.l;\n";
//  ofs << "cost.l = 1000000* sum((iv,k)$kindV(K,iv),(1-sum(n$(kindN(K,n)), Mn.l(iv, n)))) +  1000 * length.l + sum(l,sum(v,Mvl.l(v,l)));\n";
  ofs << "display Tv.l;\n";
  ofs << "display length.l;\n";
  ofs << "display cost.l;\n";

}

bool GamsScheduler::schedule(SSDfg* ssDFG,Schedule*& schedule) {
  int iters=0;

  Schedule* best_schedule=nullptr;
  Schedule* cur_schedule=nullptr;

  while(total_msec() < _reslim * 1000) {
    bool success = schedule_internal(ssDFG,cur_schedule);
    
    int lat,latmis;
    cur_schedule->calcLatency(lat,latmis);

    printf("BIG ITER DONE: lat %d, latmis %d\n",lat,latmis);
    if(best_schedule) {
      printf("Best latmis so far: latmis %d\n",best_schedule->max_lat_mis());
    }

    if(success) {
      if(best_schedule==nullptr || 
            (cur_schedule->max_lat_mis() < best_schedule->max_lat_mis()) ) {
        if(best_schedule) delete best_schedule;
        best_schedule=cur_schedule;
      }
  
      //success = success && (latmis == 0);
  
      iters++;
      if(latmis==0 || iters > 1000) {
        break;
      }     
    }
  }

  schedule = best_schedule;
  return best_schedule != nullptr;
}


bool GamsScheduler::schedule_internal(SSDfg* ssDFG,Schedule*& schedule) {

  //Get the heuristic scheduling done first
  Schedule* heur_sched=nullptr;
  bool heur_success=false;

  bool mrt_heur = ModelParsing::StartsWith(str_subalg,"MRT'");
  bool mr_heur = ModelParsing::StartsWith(str_subalg,"MR'");
  bool heur_fix = mrt_heur || mr_heur;

  if(_mipstart || heur_fix) {
    SchedulerSimulatedAnnealing heur_scheduler(_ssModel);
    heur_scheduler.suppress_timing_print=true;
    heur_scheduler.verbose=verbose;
    heur_scheduler.set_max_iters(_max_iters);
    heur_scheduler.setTimeout(_reslim - (total_msec()/1000));

    //if(mrt_heur) {
    //} else if(mr_heur) {
    //  heur_scheduler.set_integrate_timing(false);
    //}
    heur_success=heur_scheduler.schedule_timed(ssDFG,heur_sched);
    //heur_sched->calcAssignEdgeLink();
  }

  if(str_subalg == "MRT'" || str_subalg == "MR'.T'" || str_subalg == "MR'" ||
     total_msec() > _reslim * 1000) {
    schedule = heur_sched;
    return heur_success;
  }



  //load up various models?
  string hw_model          = string((char*)gams_models_hw_model_gms,
                                           gams_models_hw_model_gms_len);
  string timing_model      = string((char*)gams_models_timing_model_gms,
                                           gams_models_timing_model_gms_len);
  string stage_model       = string((char*)gams_models_stage_model_gms,
                                           gams_models_stage_model_gms_len);
  string softbrain_gams    = string((char*)gams_models_softbrain_gams_gms,
                                           gams_models_softbrain_gams_gms_len);
  string softbrain_gams_hw = string((char*)gams_models_softbrain_gams_hw_gms,
                                           gams_models_softbrain_gams_hw_gms_len);

  //mkfifo("/tmp/gams_fifo",S_IRWXU);
  stringstream ss;
  ss << _gams_work_dir << "/softbrain.out";
  string gams_out_file = ss.str();
  
  ss.str(std::string());
  ss << "softbrain.gams";
  string gams_file_name = ss.str();
  
  checked_system(("rm -f " + gams_out_file).c_str());
  
  // New schedule to work on
  schedule = new Schedule(_ssModel,ssDFG);

  //bool use_hw=true;
  bool use_hw=false;

  // ----------------- setup the ssmodel gams files --------------------------
  if(!_gams_files_setup) {
        
    // Print the Constraints
    ofstream ofs_constraints(_gams_work_dir+"/constraints.gams", ios::out);
    assert(ofs_constraints.good());

    ofs_constraints << "set stages_opt /place_heur_deform, place_heur_pos, fixR, fixM, MRT, MR, mipstart, passthrough, sll/;\n";
    ofs_constraints << "set stages(stages_opt);\n";

    ofs_constraints << "stages('passthrough')=1;\n";
    ofs_constraints << "scalar max_edge_delay /" << _ssModel->maxEdgeDelay() << "/;\n";

    if(_mipstart || heur_fix) {
      ofs_constraints << "stages('mipstart')=1;\n";
    }

    if(_sll) {
      ofs_constraints << "stages('sll')=1;\n";
    }

    if(str_subalg == "") {
      str_subalg = "MR.RT";
    }

    if(str_subalg == "MRT") {
      ofs_constraints << "stages('MRT')=1;\n";
    } else if(str_subalg == "MR.RT") {
      ofs_constraints << "stages('MR')=1;\n";
      ofs_constraints << "stages('MRT')=1;\n";
    } else if(str_subalg == "M.RT") {
      ofs_constraints << "stages('place_heur_deform')=1;\n";
      ofs_constraints << "stages('fixM')=1;\n";
      ofs_constraints << "stages('MRT')=1;\n";
    } else if(str_subalg == "MR.T") {
      ofs_constraints << "stages('MR')=1;\n";
      ofs_constraints << "stages('fixR')=1;\n";
      ofs_constraints << "stages('MRT')=1;\n";
    } else if(str_subalg == "M.R.T") {
      ofs_constraints << "stages('place_heur_deform')=1;\n";
      ofs_constraints << "stages('fixM')=1;\n";
      ofs_constraints << "stages('MR')=1;\n";
      ofs_constraints << "stages('fixR')=1;\n";
      ofs_constraints << "stages('MRT')=1;\n";
    } else if(str_subalg == "MR'.RT") {
      ofs_constraints << "stages('fixM')=1;\n";
      ofs_constraints << "stages('MRT')=1;\n";
    } else if(str_subalg == "MRT'.RT") {
      ofs_constraints << "stages('fixM')=1;\n";
      ofs_constraints << "stages('MRT')=1;\n";
    } else {
      cerr << "Bad Subalg Option\n";
      exit(1);
    }

    //ofs_constraints << multi_model;
    if(use_hw) {
      ofs_constraints << hw_model;
    } else {
      //ofs_constraints << timing_model;
      ofs_constraints << stage_model;
    }

    ofs_constraints.close();
     // Print the kinds of instructions
    ofstream ofs_kinds(_gams_work_dir+"/softbrain_kind.gams", ios::out);
    assert(ofs_kinds.good());
    _ssModel->printGamsKinds(ofs_kinds);
    ofs_kinds.close();
  
    _gams_files_setup=true;
  }
  
  // Print the controlling file
  ofstream ofs_ss_gams(_gams_work_dir+"/"+gams_file_name, ios::out);
  assert(ofs_ss_gams.good());
 
  double timeout = _reslim - (total_msec()/1000);

  if(heur_fix && heur_sched) { // early timeout only for MR'.RT scheduler
    timeout = std::min(timeout,300.0);
  }

  ofs_ss_gams << "option reslim=" << timeout << ";\n"
              << "option optcr="  <<  _optcr << ";\n"   
              << "option optca="  <<  _optca << ";\n";

  if(use_hw) {
    ofs_ss_gams << softbrain_gams_hw;
  } else {
    ofs_ss_gams << softbrain_gams;
  }
  ofs_ss_gams.close();
  
  ofstream ofs_mipstart(_gams_work_dir+"/mip_start.gams", ios::out);
  assert(ofs_mipstart.good());

  if(heur_sched && (_mipstart || heur_fix)) {
    print_mipstart(ofs_mipstart,heur_sched,ssDFG,true/* fix */);
  }
  ofs_mipstart.close();
  

  schedule->clearAll();

  cout << "Total Nodes: " << ssDFG->nodes().size() << "\n";
  
  int numInsts = ssDFG->inst_vec().size();
  cout << "Total Insts: " <<  numInsts << "\n";
  //assert(numInsts > 0);

  // Print the softbrain model   
  ofstream ofs_ss_model(_gams_work_dir + "/softbrain_model.gams", ios::out);
  assert(ofs_ss_model.good());
  gamsToNode.clear(); gamsToLink.clear();

  

  cout << _ssModel->subModel()->sizex() << " is the x size \n";

  _ssModel->subModel()->PrintGamsModel(ofs_ss_model,gamsToNode,gamsToLink,
                                       gamsToSwitch,gamsToPortN,1/*nconfigs*/);

  cout << gamsToNode.size() << " " << gamsToLink.size() << " " << gamsToSwitch.size() << "\n";

  ofs_ss_model.close();
  
  // ----------------- setup the dfg gams files ------------------------------
  ofstream ofs_ss_dfg(_gams_work_dir + "/softbrain_dfg.gams", ios::out);
  if(ofs_ss_dfg.fail()) {
    cerr << "could not open " + _gams_work_dir + "/softbrain_dfg.gams";
    return false;
  }
  gamsToDfgnode.clear(); 
  gamsToDfgedge.clear();
  gamsToPortV.clear();

  //Also populates these maps
  //--gamsToDfgnode
  //--gamsToDfgegde
  //--gamsToPortV
  ssDFG->printGams(ofs_ss_dfg,gamsToDfgnode,gamsToDfgedge,gamsToPortV);
  ssDFG->printPortCompatibilityWith(ofs_ss_dfg,_ssModel);
  ofs_ss_dfg.close();
  
  // ----------------- run gams! --------------------------------------------
  stringstream ss_cmd;
  ss_cmd << "gams " << gams_file_name << " wdir=" << _gams_work_dir;
  if(_showGams) {
    ss_cmd << " -lo=3";
  } else {
     ss_cmd << " -o=/dev/null -lo=2"; 
  }
  cout << ss_cmd.str().c_str() << "\n";
  checked_system(ss_cmd.str().c_str());

  // ----------------- parse output -----------------------------------------


  string line, edge_name, vertex_name, switch_name, link_name, out_link_name, ssnode_name,list_of_links,latency_str;
  ifstream gamsout(gams_out_file.c_str());
  enum {VtoN,EtoL,LtoL,EL,EDGE_DELAY,TIMING,PortMap,PASSTHROUGH,Parse_None}parse_stage;
  parse_stage=Parse_None;
  bool message_start=false, message_fus_ok=false, 
       message_ports_ok=false, message_complete=false;

  while(gamsout.good()) {  
    getline(gamsout,line);
    ModelParsing::trim_comments(line);
    ModelParsing::trim(line);

    if(line.empty()) {
      continue;
    }
    //if(ModelParsing::StartsWith(line,"#")) continue;
    if(line[0]=='[') {
      parse_stage = Parse_None; 
      if(ModelParsing::StartsWith(line,"[vertex-node-map]")) {
        parse_stage = VtoN; continue;
      } else if(ModelParsing::StartsWith(line,"[edge-link-map]")) {
        parse_stage = EtoL; continue;
      } else if(ModelParsing::StartsWith(line,"[switch-map]")) {
        parse_stage = LtoL; continue;
      } else if(ModelParsing::StartsWith(line,"[extra-lat]")) {
        parse_stage = EL; continue;
      } else if(ModelParsing::StartsWith(line,"[edge-delay]")) {
        parse_stage = EDGE_DELAY; continue;
      } else if(ModelParsing::StartsWith(line,"[timing]")) {
        parse_stage = TIMING; continue;
      } else if(ModelParsing::StartsWith(line,"[passthrough]")) {
        parse_stage = PASSTHROUGH; continue;
      } else if(ModelParsing::StartsWith(line,"[port-port-map]")) {
        parse_stage = PortMap; continue;
      } else if(ModelParsing::StartsWith(line,"[status_message_begin_scheduling]")) {
        message_start=true; continue;
      } else if(ModelParsing::StartsWith(line,"[status_message_fus_ok]")) {
        message_fus_ok=true; continue;
      } else if(ModelParsing::StartsWith(line,"[status_message_ports_ok]")) {
        message_ports_ok=true; continue;
      } else if(ModelParsing::StartsWith(line,"[status_message_complete]")) {
        message_complete=true; continue;
      } else if(ModelParsing::StartsWith(line,"[status_message]")) {
        // do nothing
      } 
    }

    if(parse_stage==PortMap) {
      stringstream ss(line);
      getline(ss, vertex_name, ':');
      ss >> std::ws;
      getline(ss, ssnode_name, ' ');

      ModelParsing::trim(vertex_name);
      ModelParsing::trim(ssnode_name);

      if(ssnode_name.empty()) {
        cout << "failed to parse line: \"" << line << "\"\n";
        assert(0);
      }

      SSDfgVec* pv = gamsToPortV[vertex_name];
      assert(pv);
      std::pair<bool,int> pn = gamsToPortN[ssnode_name];  

      unsigned size_of_vp;
      if(pn.first) {
       size_of_vp = _ssModel->subModel()->io_interf().in_vports[pn.second]->size();
      } else {
       size_of_vp = _ssModel->subModel()->io_interf().out_vports[pn.second]->size();
      }

      std::vector<bool> mask;
      //mask.resize(pv->locMap().size());
      mask.resize(size_of_vp);
      
      while(ss.good()) {
        string ind_str;
        getline(ss, ind_str, ' ');
        ModelParsing::trim(ind_str);
        if(ind_str.empty()) continue;
        unsigned ind = (int)(stof(ind_str))-1;
        
        //cout << vertex_name << " " << ssnode_name << " " << ind << " " << size_of_vp << "\n";
        assert(ind < size_of_vp && "went off end of vec");
        assert(mask[ind]==false && "I already assigned this place in the vec!");

        mask[ind]=true;
      }
      //cout << "\n";

      schedule->assign_vport(pv,pn,mask);

    } else if(parse_stage==PASSTHROUGH) {
      stringstream ss(line);
      
      while(ss.good()) {
        getline(ss,ssnode_name, ' ');
        ModelParsing::trim(ssnode_name);
        if(ssnode_name.empty()) continue;
        
        ssnode* ssnode  = gamsToNode[ssnode_name].first;
        if(ssnode==nullptr) {
          cerr << "null ssnode:\"" << ssnode_name << "\"\n";
        }
        schedule->add_passthrough_node(ssnode);

      }

    } else if(parse_stage==TIMING) {
      stringstream ss(line);
      getline(ss, vertex_name, ':');
      getline(ss, latency_str, '.');
      ModelParsing::trim(vertex_name);
      ModelParsing::trim(latency_str);
      SSDfgNode* dfgnode = gamsToDfgnode[vertex_name];

      int lat = stoi(latency_str);
      schedule->assign_lat(dfgnode,lat);

    } else if(parse_stage==EL) {
      stringstream ss(line);
      getline(ss, edge_name, ':');
      getline(ss, ssnode_name);
      //TODO: FINISH THIS IF EVER NEED EDGE -> LINK MAPPING

    } else if(parse_stage==EDGE_DELAY) {
      stringstream ss(line);
      getline(ss, edge_name, ':');

      ModelParsing::trim(edge_name);
      SSDfgEdge* dfgedge = gamsToDfgedge[edge_name];
      assert(dfgedge);

      string delay_str;
      getline(ss, delay_str);
      ModelParsing::trim(delay_str);
      if(delay_str.empty()) continue;
      unsigned delay = (unsigned)(stof(delay_str));

      //dfgedge->set_delay(delay);
      schedule->set_edge_delay(delay,dfgedge);

    } else if(parse_stage==VtoN) {
      stringstream ss(line);
      getline(ss, vertex_name, ':');
      getline(ss, ssnode_name);
      ModelParsing::trim(vertex_name);
      ModelParsing::trim(ssnode_name);
      
      if(ssnode_name.empty()) {
        return false;
      }

      SSDfgNode* dfgnode = gamsToDfgnode[vertex_name];
      ssnode* ssnode  = gamsToNode[ssnode_name].first;
      
      if(vertex_name.empty()) continue;
      
     
      schedule->assign_node(dfgnode, make_pair(0, ssnode));
      
        /*if(ssoutput* ssout = dynamic_cast<ssoutput*>(ssnode) ) {
           cout << dfgnode->name() << " new=" << schedule->getPortFor(dfgnode) << "\n";
        }*/
        
      //schedule
    } else if (parse_stage==LtoL) { //PARSE SWITCH MAP --------------------------
      stringstream ss(line);
      getline(ss, switch_name, ':');
      ModelParsing::trim(switch_name);
      if(switch_name.empty()) continue;

      ssswitch* sssw = gamsToSwitch[switch_name].first;
      if(sssw==nullptr) {
        cerr << "null sssw:\"" << switch_name << "\"\n";
      }

      while(ss.good()) {
        getline(ss, link_name, ' '); // not redundant, get rid of first space
        getline(ss, link_name, ' ');
        getline(ss, out_link_name, ',');

        ModelParsing::trim(link_name);
        ModelParsing::trim(out_link_name);

        if(link_name.empty()) continue;
        if(out_link_name.empty()) continue;
        sslink* slink = gamsToLink[link_name].first;
        sslink* slink_out = gamsToLink[out_link_name].first;
        assert(slink);
        assert(slink_out);
        schedule->assign_switch(sssw,slink,slink_out);

      }
      
    } else if (parse_stage==EtoL) {
//      if(_assignSwitch.size()!=0) {
//        continue;
//      }

      stringstream ss(line);
      getline(ss, edge_name, ':');
      //getline(ss, list_of_links);
      
      ModelParsing::trim(edge_name);
      if(edge_name.empty()) continue;
      
      SSDfgEdge* dfgedge = gamsToDfgedge[edge_name];
      SSDfgNode* dfgnode = dfgedge->def(); 
      if(dfgnode==nullptr) {
        cerr << "null dfgnode:\"" << vertex_name << "\"\n";
      }
     
      //TODO:FIXME check if I broke anything 
      while(ss.good()) {
        getline(ss, link_name, ' ');
        ModelParsing::trim(link_name);
        if(link_name.empty()) continue;
        sslink* slink = gamsToLink[link_name].first;
        
        if(slink==nullptr) {
          cerr << "null slink:\"" << link_name << "\"\n";
        }
        
        schedule->assign_edgelink(dfgedge, 0, slink);
        
        if(ssinput* ssin = dynamic_cast<ssinput*>(slink->orig())) {
          schedule->assign_node(dfgnode, make_pair(0, ssin));
        } else if(ssoutput* ssout = dynamic_cast<ssoutput*>(slink->dest())) {
          //find output for this output edge
          for(auto elem : dfgnode->uses()) {
            if(SSDfgOutput* dfg_out = dynamic_cast<SSDfgOutput*>(elem->use())) {
              schedule->assign_node(dfg_out, make_pair(0, ssout));
            }
          }
        }
        
      }
    }
    
  }
 
  if(!message_start) {
    cerr << "\n\nError: Scheduling Not Started -- Likely Error in Gams Code Gen\n\n";
    exit(1);
  } else if (!message_fus_ok) {
    cerr << "\n\nError: Combination of FUs requested are NOT satisfiable with given SSCONFIG.\n\n";
    exit(1);
  } else if (!message_ports_ok) {
    cerr << "\n\nError: Port specifications are NOT satisfiable with given SSCONFIG.\n\n";
    exit(1);
  }  else if (!message_complete) {
    return false; 
  }

  //if(_showGams) {
  //  //Print the I/Os
  //  std::cout << "in/out mapping:";

  //  SSDfg::const_input_iterator Ii,Ei;
  //  for(Ii=schedule->ssdfg()->input_begin(),Ei=schedule->ssdfg()->input_end();Ii!=Ei;++Ii) {
  //    SSDfgInput* in = *Ii;
  //    int p = schedule->getPortFor(in);
  //    cout << in->name() << " " << p << ", ";
  //  }

  //  SSDfg::const_output_iterator Io,Eo;
  //  for(Io=schedule->ssdfg()->output_begin(),Eo=schedule->ssdfg()->output_end();Io!=Eo;++Io) {
  //    SSDfgOutput* out = *Io;
  //    int p = schedule->getPortFor(out);
  //    cout << out->name() << " " << p << ", ";
  //  }
  //  std::cout << "\n";
  //}
  
  
  return true;
}

