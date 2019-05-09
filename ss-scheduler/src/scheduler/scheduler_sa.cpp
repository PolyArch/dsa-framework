#include "scheduler_sa.h"

using namespace SS_CONFIG;
using namespace std;

#include <unordered_map>
#include <fstream>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define DEBUG_SCHED (false)

//Major things left to try that might improve the algorithm:
//1. Overprovisioning while routing/placing 
//    (maybe also the including actual simulated annealing part)
//2. Prioritizing nodes based on what their presumed effect on scheduling
//   quality might be.  This may be difficult to gauge though.

void SchedulerSimulatedAnnealing::initialize(SSDfg* ssDFG, Schedule*& sched) {
  //Sort the input ports once
  SS_CONFIG::SubModel* subModel = _ssModel->subModel();
  ssio_interface& si =  subModel->io_interf();
  _sd_in.clear();
  si.sort_in_vports(_sd_in);
  si.sort_out_vports(_sd_out);

  ssDFG->sort_vec_in();
  ssDFG->sort_vec_out();

  sched = new Schedule(getSSModel(),ssDFG); //just a dummy one
}

std::pair<int, int> SchedulerSimulatedAnnealing::obj( Schedule*& sched, 
    int& lat, int& latmis, int& ovr, int& agg_ovr, int& max_util) {  
    int num_left = sched->num_left(); 
    bool succeed_sched = (num_left==0);

    sched->get_overprov(ovr,agg_ovr,max_util);

    //bool succeed_ovr = (ovr ==0);

    //bool succeed_timing = false;
    if (succeed_sched) { 
      //succeed_timing = 
      sched->fixLatency(lat,latmis);
    } else {
      latmis = 1000;
      lat = 1000;
    }

  int violation = sched->violation();

  int obj = agg_ovr*100000 + ovr * 50000 + latmis*10000+violation*100+lat + max_util;

//        fprintf(stdout, "objective rt:%d, left: %3d, " 
//                "lat: %3d, vio %d, mis: %d, ovr: %d, util: %d, "
//                "obj:%d, ins: %d/%d, outs: %d/%d,"
//                " insts: %d, links:%d, edge-links:%d  %s%s\n", 
//                _route_times,
//                sched->num_left(), lat, 
//                sched->violation(), latmis, ovr, max_util, obj,
//                sched->num_inputs_mapped(),  sched->ssdfg()->num_inputs(),
//                sched->num_outputs_mapped(), sched->ssdfg()->num_outputs(),
//                sched->num_insts_mapped(),  
//                sched->num_links_mapped(),
//                sched->num_edge_links_mapped(),
//                succeed_sched ? ", all mapped" : "",
//                succeed_timing ? ", mismatch == 0" : "");

//    sched->printGraphviz("hi.gv");

  return make_pair( succeed_sched  -num_left, -obj);
}

bool SchedulerSimulatedAnnealing::schedule(SSDfg* ssDFG, Schedule*& sched) {  
  initialize(ssDFG,sched);

  static int _srand_no=1; 

  int max_iters_no_improvement = 100000000;
  srand(++_srand_no);

  Schedule* cur_sched = new Schedule(getSSModel(),ssDFG);
  std::pair<int, int> best_score = make_pair(0, 0);
  bool best_succeeded = false;
  bool best_mapped = false;
  std::set<SSDfgOutput*> best_dummies;

  int last_improvement_iter = 0;

  _best_lat=MAX_ROUTE;
  _best_violation=MAX_ROUTE;

  int presize = ssDFG->inst_vec().size();
  int postsize = presize;

  // An attempt to remap SSDfg
  SS_CONFIG::SubModel* subModel = _ssModel->subModel();
  int hw_num_fu = subModel->sizex()*subModel->sizey();

  int remapNeeded = false; //ssDFG->remappingNeeded(); //setup remap structres
  int iter = 0;
  while (iter < _max_iters) {
    if( (total_msec() > _reslim * 1000) || _should_stop ) {
      break;
    }

    bool print_stat=false;
    if( (iter & (256-1)) == 0) {
      print_stat=true;
    }

    if( (iter & (4096-1)) == 0) {
      //Every so often, lets give up and start over from scratch
      delete cur_sched;
      cur_sched = new Schedule(getSSModel(),ssDFG);
     
      if(remapNeeded) { //remap every so often to try new possible dummy positions
        ssDFG->remap(hw_num_fu);
        postsize = ssDFG->inst_vec().size();
        cur_sched->allocate_space();
      }
    }

    // every so often you can reset?
    if(iter - last_improvement_iter > 1023) {
      *cur_sched = *sched; //shallow copy of sched should work?
    }


    bool succeed_sched = schedule_internal(ssDFG, cur_sched);

    int lat=INT_MAX,latmis=INT_MAX,agg_ovr=INT_MAX,ovr=INT_MAX,max_util=INT_MAX;
    std::pair<int,int> score = obj(cur_sched,lat,latmis,ovr,agg_ovr,max_util);

    int succeed_timing = (latmis ==0) && (ovr ==0);

    if (verbose && ((score > best_score) || print_stat)) {
      stringstream ss;
      ss << "viz/iter/" << iter << ".gv";
      cur_sched->printGraphviz(ss.str().c_str());

      for(int i = 0; i < ssDFG->num_vec_input(); ++i) {
        cout << cur_sched->vecPortOf(ssDFG->vec_in(i)).second << " ";
      }

      fprintf(stdout, "Iter: %4d, time:%0.2f, rt:%d, left: %3d, " 
              "lat: %3d, vio %d, mis: %d, ovr: %d, agg_ovr: %d, util: %d, "
              "obj:%d, ins: %d/%d, outs: %d/%d,"
              " insts: %d/%d,%d, links:%d, edge-links:%d  %s%s", 
              iter, total_msec()/1000.f, _route_times,
              cur_sched->num_left(), lat, 
              cur_sched->violation(), latmis, ovr, agg_ovr, 
              max_util, -score.second,
              cur_sched->num_inputs_mapped(),  (int) ssDFG->inputs().size(),
              cur_sched->num_outputs_mapped(), (int) ssDFG->outputs().size(),
              cur_sched->num_insts_mapped(),  presize, postsize,
              cur_sched->num_links_mapped(),
              cur_sched->num_edge_links_mapped(),
              succeed_sched ? ", all mapped" : "",
              succeed_timing ? ", mismatch == 0" : "");
      if(score > best_score) {
        cout << "\n";
      } else {
        cout.flush();
        cout << "\r";
      }
    }


    if (score > best_score) {
      sched->printGraphviz("viz/cur-best.gv");

      if(remapNeeded) {
        ssDFG->printGraphviz("viz/remap.dot");
        best_dummies = ssDFG->getDummiesOutputs();
      }

      best_score = score;
      *sched = *cur_sched; //shallow copy of sched should work?
      if (succeed_timing && !best_succeeded) {
        max_iters_no_improvement = std::max(1000, iter * 2);
      }

      best_mapped = succeed_sched;
      best_succeeded = succeed_timing;
      last_improvement_iter = iter;
    }

    iter++;

    if (((iter - last_improvement_iter) > max_iters_no_improvement) 
        && best_succeeded) {
      break;
    }
    if(sched->violation() ==0 && iter > _max_iters_zero_vio) {
      break;
    }
  }
  if(verbose) {
    cout << "Breaking at Iter " << iter << "\n";
  }

  //Fix back up any dummies
  if(remapNeeded) {
    ssDFG->rememberDummies(best_dummies);
  }

  if(cur_sched) {
    delete cur_sched;
  }
  return best_mapped;
}

void SchedulerSimulatedAnnealing::findFirstIndex(vector<pair<int,int>>& sd, 
    ssio_interface& si, unsigned int numIO, unsigned int& index, bool is_input){
  for (; index < sd.size(); index++) {
    pair<int,int> j = sd[index];
    int vn = j.first; 
    auto* vdesc = (!is_input ? si.getDesc_O(vn) : si.getDesc_I(vn));
    //Check if the vetcor port is 1. big enough
    if (numIO <= vdesc->size()) { break;}
  }
}

bool SchedulerSimulatedAnnealing::genRandomIndexBW(
        pair<bool, int>& vport_id,
        vector<int>& vport_desc,
        vector<pair<int,int>>& sd,
        ssio_interface& si,
        unsigned int size,
        unsigned int index,
        Schedule*& sched,
        bool is_input) {
  unsigned int k;
  pair<int, int> p;
  int vport_num;
  unsigned int rep = 0;
  bool success=false;

  while (!success && rep++ < size*2) {
    k = rand() % (size - index) + index;
    p = sd[k];
    vport_num = p.first;
    vport_id = std::make_pair(is_input,vport_num);
    
    success = sched->vportOf(vport_id) == nullptr;

    vport_desc = (!is_input ? 
        si.getDesc_O(vport_num) : si.getDesc_I(vport_num))->port_vec();
  }

  return success;
}

bool SchedulerSimulatedAnnealing::schedule_input( SSDfgVecInput*  vec_in, SSDfg* ssDFG, Schedule* sched) {

  SS_CONFIG::SubModel *subModel = _ssModel->subModel();
  ssio_interface &si = subModel->io_interf();
  int n_vertex = (int) vec_in->inputs().size();
  int n_vertex_physical = n_vertex;

  if (vec_in->is_temporal()) {
    n_vertex_physical = 1;  // temporal vectors are always scheduled to one node
  }

  unsigned int index = 0;
  int num_found = 0;
  //use physical number of vertices to decide a port
  findFirstIndex(_sd_in, si, n_vertex_physical, index, true /*input*/);
  unsigned int attempt = 0;

  vector<int> order; //temp variable used for randomly iterating

  CandidateRouting r1, r2; //do this so that function scope de-allocates these
  CandidateRouting *bestRouting = &r1, *candRouting = &r2;

  vector<bool> bestMask;
  pair<int, int> bestScore = std::make_pair(INT_MIN, INT_MIN);
  pair<bool, int> bestVportid;
  std::vector<ssnode *> bestInputs;

  while (num_found < 8 &&  (attempt++ < _sd_in.size())) {
    pair<bool, int> vport_id;
    vector<int> vport_desc;

    //TODO: put this code in schedule_output as well
    bool found=false;
    while(!found) {
      if (!genRandomIndexBW(vport_id, vport_desc, _sd_in, si, _sd_in.size(), index, sched, true /*input*/)) {
        unmap_one_input(ssDFG, sched);
      } else {
        found = vec_in->is_temporal() || si.in_vports[vport_id.second]->size() >= vec_in->inputs().size();
      }
    }


    candRouting->clear();  
    std::vector<ssnode*> possInputs;

    for (unsigned m = 0; m < vport_desc.size(); m++) {
      int cgra_port_num = vport_desc[m];
      ssinput *in = subModel->inputs()[cgra_port_num];
      if (sched->dfgNodeOf(in) == nullptr) {
        possInputs.push_back(in);
      }
    }

    if ((int) possInputs.size() < n_vertex_physical) continue;

    vector<ssnode *> candInputs;
    rand_node_choose_k(n_vertex_physical, possInputs, candInputs);

    bool ports_okay_to_use = true;
    random_order(n_vertex, order);
    int num_links_used = 0;
    for (int i : order) {
      //In a temporal region, just select the 0th input
      int cand_index = vec_in->is_temporal() ? 0 : i;
      ssnode *node = candInputs[cand_index];
      SSDfgNode *vertex = vec_in->inputs()[i];

      pair<int, int> n_score = scheduleHere(sched, vertex, make_pair(0, node), *candRouting);
      if (n_score >= fscore) {
        ports_okay_to_use = false;
        break;
      }

      num_links_used += n_score.second;
    }

    if (!ports_okay_to_use) continue;
    num_found++;

    apply_routing(sched, candRouting);
    for (int i = 0; i < n_vertex; ++i) {
      int cand_index = vec_in->is_temporal() ? 0 : i;
      ssnode *node = candInputs[cand_index];
      sched->assign_node(vec_in->inputs()[i], make_pair(0, node));
    }

    int lat = INT_MAX, latmis = INT_MAX, ovr = INT_MAX, 
        agg_ovr = INT_MAX, max_util = INT_MAX;
    pair<int, int> candScore = obj(sched, lat, latmis, ovr, agg_ovr, max_util);
    candScore.second -= num_links_used;

    //TODO: does this help at all?
    int hw_port_size = vport_desc.size();
    int extra = hw_port_size - vec_in->inputs().size();
    candScore.second-=extra*10;

    sched->unassign_input_vec(vec_in);

    if (candScore > bestScore) {
      //cout << candScore.first << " " << candScore.second <<"\n";
      //cout << "lat: " << lat << " latmis: " << latmis 
      //    << " ovr " << ovr << " num links used " << num_links_used << "\n";
      bestScore = candScore;
      bestInputs = candInputs;
      bestVportid = vport_id;
      std::swap(bestRouting, candRouting);
      bestMask.clear();
      bestMask.resize(vport_desc.size());
      int num_cgra_ports = 0; //for debugging
      for (int m = 0; m < (int) vport_desc.size(); m++) {
        int cgra_port_num = vport_desc[m];
        for (int i = 0; i < (int) candInputs.size(); ++i) {
          if (static_cast<ssinput *>(candInputs[i])->port() == cgra_port_num) {
            assert(bestMask[m] == false);
            bestMask[m] = true;
            num_cgra_ports++;
            break;
          }
        }
      }
      //assert(n_vertex == num_cgra_ports);
    }
  }
  //cout << " -- \n";

  if (num_found > 0) {
    for (int i = 0; i < n_vertex; ++i) {
      int cand_index = vec_in->is_temporal() ? 0 : i;
      ssnode *node = bestInputs[cand_index];
      sched->assign_node(vec_in->inputs()[i], make_pair(0, node));
    }
    sched->assign_vport(vec_in, bestVportid, bestMask);
    apply_routing(sched, bestRouting); //Commit the routing
  }
  return num_found > 0;
}

bool SchedulerSimulatedAnnealing::schedule_output( SSDfgVecOutput*  vec_out, 
    SSDfg* ssDFG, Schedule* sched) {

  SS_CONFIG::SubModel* subModel = _ssModel->subModel();
  ssio_interface& si =  subModel->io_interf();
  int n_vertex = vec_out->outputs().size();
  int n_vertex_physical = n_vertex;

  if(vec_out->is_temporal()) {
    n_vertex_physical=1;  // temporal vectors are always scheduled to one node
  }

  unsigned int index = 0;
  int num_found=0;
  findFirstIndex(_sd_out, si, n_vertex_physical, index, false /*output*/);
  unsigned int attempt = 0;

  vector<int> order; //temp variable used for randomly iterating

  CandidateRouting r1,r2;
  CandidateRouting* bestRouting = &r1, * candRouting = &r2;  

  vector<bool> bestMask;
  pair<int,int> bestScore = std::make_pair(INT_MIN,INT_MIN);
  pair<bool, int> bestVportid;
  std::vector<ssnode*> bestOutputs;


  while (num_found < 10 &&  (attempt++ < _sd_out.size())) {
    pair<bool, int> vport_id;
    vector<int> vport_desc;
    if(!genRandomIndexBW(vport_id, vport_desc, _sd_out, si, _sd_out.size(), index, sched, false /*output*/)) {
      return false;
    }

    candRouting->clear();  
    std::vector<ssnode*> possOutputs;

    for (unsigned m=0; m < vport_desc.size(); m++) {
      int cgra_port_num = vport_desc[m];
      ssoutput* out = subModel->outputs()[cgra_port_num];
      if (sched->dfgNodeOf(out) == nullptr) {
        possOutputs.push_back(out);
      }
    }

    if((int)possOutputs.size() < n_vertex_physical) continue;

    vector<ssnode*> candOutputs;
    rand_node_choose_k(n_vertex_physical, possOutputs, candOutputs);

    bool ports_okay_to_use=true;
    random_order(n_vertex,order);
    int num_links_used=0;
    for(int i : order) {
      int cand_index = vec_out->is_temporal() ? 0 : i;
      ssnode* node = candOutputs[cand_index];
      SSDfgNode* vertex = vec_out->outputs()[i];

      pair<int,int> n_score = scheduleHere(sched, vertex, make_pair(0, node), *candRouting);
      if(n_score>=fscore) {
        ports_okay_to_use=false;
        break;
      }

      num_links_used+=n_score.second;
    }

    if (!ports_okay_to_use) continue;
    num_found++;

    apply_routing(sched, candRouting);
    for(int i = 0; i < n_vertex; ++i) {
       int cand_index = vec_out->is_temporal() ? 0 : i;
       ssnode* node = candOutputs[cand_index];
       sched->assign_node(vec_out->outputs()[i], make_pair(0, node));
    }
    
    int lat=INT_MAX,latmis=INT_MAX,ovr=INT_MAX,agg_ovr=INT_MAX,max_util=INT_MAX;
    pair<int,int> candScore = obj(sched,lat,latmis,ovr,agg_ovr,max_util);
    candScore.second-=num_links_used;
    sched->unassign_output_vec(vec_out);

    if(candScore > bestScore) {
      //cout << candScore.first << " " << candScore.second <<"\n";
      //cout << "lat: " << lat << " latmis: " << latmis 
      //    << " ovr " << ovr << " num links used " << num_links_used << "\n";
      bestScore=candScore;
      bestOutputs=candOutputs;
      bestVportid=vport_id;
      std::swap(bestRouting,candRouting);
      bestMask.clear();
      bestMask.resize(vport_desc.size());
      int num_cgra_ports=0; //for debugging
      for (int m=0; m < (int)vport_desc.size(); m++) {
        int cgra_port_num = vport_desc[m];
        for(int i = 0; i < (int)candOutputs.size(); ++i) {
          if(static_cast<ssoutput*>(candOutputs[i])->port()==cgra_port_num) {
            bestMask[m]=true;
            num_cgra_ports++;
            break;
          }
        }
      }
      //assert(n_vertex == num_cgra_ports);
    }
  }
  //cout << " -- \n";
  if(num_found > 0) {
    for(int i = 0; i < n_vertex; ++i) { 
       int cand_index = vec_out->is_temporal() ? 0 : i;
       ssnode* node = bestOutputs[cand_index];
       sched->assign_node(vec_out->outputs()[i], make_pair(0, node));
    }

    sched->assign_vport(vec_out,bestVportid,bestMask);
    apply_routing(sched, bestRouting); //Commit the routing
  }
  return num_found>0;
}

bool SchedulerSimulatedAnnealing::map_one_input(SSDfg* ssDFG, Schedule* sched) {
  if(DEBUG_SCHED) cout << "map_one_input (" 
                       << sched->num_inputs_mapped() << " mapped)\n";

  int n = ssDFG->num_vec_input();

  int p = rand_bt(0,n);
  for(int i = 0; i < n; ++i) {
    p = (p + 1) % n;

    SSDfgVecInput *vec_in = ssDFG->vec_in(p);
    if (sched->vecMapped(vec_in)) continue;
    bool res = schedule_input(vec_in, ssDFG, sched);
    return res;
  }
  return false;
}

bool SchedulerSimulatedAnnealing::map_one_output(SSDfg* ssDFG, Schedule* sched) {
  if (DEBUG_SCHED)
    cout << "map_one_output ("
         << sched->num_outputs_mapped() << " mapped)\n";

  int n = ssDFG->num_vec_output();

  int p = rand_bt(0, n);
  for (int i = 0; i < n; ++i) {
    p = (p + 1) % n;

    SSDfgVecOutput *vec_out = ssDFG->vec_out(p);
    if (sched->vecMapped(vec_out)) continue;
    bool success = schedule_output(vec_out, ssDFG, sched);
    return success;
  }
  return false;
}

bool SchedulerSimulatedAnnealing::map_one_inst(SSDfg* ssDFG, Schedule* sched) {
  if (DEBUG_SCHED)
    cout << "map_one_inst ("
         << sched->num_insts_mapped() << " mapped)\n";

  const std::vector<SSDfgInst *> &inst_vec = ssDFG->inst_vec();
  size_t n = inst_vec.size();

  size_t p = rand_bt(0, n);
  for (size_t i = 0; i < n; ++i) {
    if (++p == n) { p = 0; }
    SSDfgInst *inst = inst_vec[p];
    if (sched->is_scheduled(inst)) continue;

    //cout << "try to map this inst: " << inst->name() << "\n";

    bool success = scheduleNode(sched, inst);
    //if(!success) {
    //  cout << "success with " << inst->name() << "\n";
    //} else {
    //  cout << "failed with " << inst->name() << "\n";
    //}
    return success;
  }
  return false;
}

bool SchedulerSimulatedAnnealing::map_io_to_completion(SSDfg* ssDFG, Schedule* sched) {

  while (!(sched->inputs_complete() && sched->outputs_complete())) {
    int r = rand_bt(0, 2); //upper limit defines ratio of input/output scheduling
    switch (r) {
      case 0: {
        if (sched->inputs_complete()) break;
        bool success = map_one_input(ssDFG, sched);
        if (!success) return false;
        break;
      }
      case 1: {
        if (sched->outputs_complete()) break;
        bool success = map_one_output(ssDFG, sched);
        if (!success) return false;
        break;
      }
      default: {
        assert(0);
        break;
      }
    }
  }
  //cout << "Schedule Complete, mapped: " << sched->num_mapped() 
  //                          << "left:" << sched->num_left();

  return true;
}


bool SchedulerSimulatedAnnealing::map_to_completion(SSDfg* ssDFG, Schedule* sched) {

  if(DEBUG_SCHED) cout << "Map to completion! " << sched->num_mapped() << "\n";
  while(!sched->isComplete()) {
    int r = rand_bt(0,16); //upper limit defines ratio of input/output scheduling
    switch(r) {
      case 0: {
        if(sched->inputs_complete()) break;
        bool success = map_one_input(ssDFG,sched);
        //cout << "I";
        if(!success) {
          //cout << "IF";
          return false;
        }
        break;
      }
      case 1: {
        if(sched->outputs_complete()) break;
        bool success = map_one_output(ssDFG,sched);
        //cout << "O";
        if(!success) { 
          //cout << "OF";
          return false;
        }
        break;
      }      
      default: {
        if(sched->insts_complete()) break;
        bool success = map_one_inst(ssDFG,sched);
        //cout << "N";
        if(!success) { 
          //cout << "NF";
          return false;
        }
        break;
      }      
    }
  }
  //cout << "Schedule Complete, mapped: " << sched->num_mapped() 
  //                          << "left:" << sched->num_left();
  
  return true;
}

void SchedulerSimulatedAnnealing::unmap_one_input(SSDfg* ssDFG, Schedule* sched) {
  int n = ssDFG->num_vec_input();
  if (DEBUG_SCHED)
    cout << "unmap_one_input ("
         << sched->num_inputs_mapped() << " left)\n";

  int p = rand_bt(0,n);
  while(true) {
    for (int i = 0; i < n; ++i) {
      if (++p == n) { p = 0; }

      SSDfgVecInput *vec_in = ssDFG->vec_in(p);
      if (sched->vecMapped(vec_in)) {
        //TODO: do this for outputs too, or just eliminate
        int hw_port_size = _ssModel->subModel()->io_interf().in_vports[sched->vecPortOf(vec_in).second]->size();
        int extra = hw_port_size - vec_in->inputs().size();
        if (!vec_in->is_temporal())
          assert(extra >= 0 && extra < 32); //don't expect this large of inputs
        int r = rand_bt(0, extra * extra + hw_port_size);
        //cout << "extra: " << extra << " for vport " << vec_in->name() << "\n";
        if (r < extra * extra + 1) {
          sched->unassign_input_vec(vec_in);
          return;
        }
      }
    }
  }
  assert(0);
}

void SchedulerSimulatedAnnealing::unmap_one_output(SSDfg* ssDFG, Schedule* sched) {
  int n = ssDFG->num_vec_output();
  if(DEBUG_SCHED) cout << "unmap_one_output (" 
                       << sched->num_outputs_mapped() << " left)\n";

  int p = rand_bt(0,n);
  for(int i = 0; i < n; ++i) {
    if(++p ==n) {p=0;}
    SSDfgVecOutput* vec_out = ssDFG->vec_out(p);
    if(sched->vecMapped(vec_out)) {
      //cout << "O" << vec_out->name();
      sched->unassign_output_vec(vec_out);
      return;
    }
  }
  assert(0);
}

void SchedulerSimulatedAnnealing::unmap_one_inst(SSDfg* ssDFG, Schedule* sched) {
  const std::vector<SSDfgInst*>& inst_vec = ssDFG->inst_vec();
  int n = inst_vec.size();
  if(DEBUG_SCHED) cout << "unmap_one_inst(" 
                       << sched->num_insts_mapped() << " left)\n";

  int p = rand_bt(0,n);
  for(int i = 0; i < n; ++i) {
    if(++p ==n) {p=0;}
    SSDfgInst* inst = inst_vec[p];
    if(sched->is_scheduled(inst)) {
      //cout << "V" << inst->name();
      sched->unassign_dfgnode(inst);

      //Error Checking : TODO: make function or remove these two loops
      SSDfgInst* n = inst;
      for(auto source_dfgedge : n->in_edges()) {
        if(sched->link_count(source_dfgedge)!=0) {
           cerr << "Edge: " << source_dfgedge->name() << " is already routed!\n"; 
           assert(0);
        }
      }

      for(auto use_dfgedge : n->uses()) {
        if(sched->link_count(use_dfgedge)!=0) {
           cerr << "Edge: " << use_dfgedge->name() << " is already routed!\n"; 
           assert(0);
        }
      }

      return;
    }
  }
  assert(0);
}

void SchedulerSimulatedAnnealing::unmap_some(SSDfg* ssDFG, Schedule* sched) {
  //for(SSDfgEdge* e : ssDFG->edges()) {
  //  if(sched->edge_links(e).size() > 8) {
  //    cout << e->name() << ":" <<  sched->edge_links(e).size() << " : ";
  //    for(auto* l : sched->edge_links(e)) {
  //      cout << l->name() << " ";
  //    }
  //    cout << "\n";
  //  }
  //}
  //cout << "\n";

  int num_to_unmap=1;
  int r = rand_bt(0,1000); //upper limit defines ratio of input/output scheduling
  if(r<5) { 
    num_to_unmap=10;
    //cout << "Z";
  } else if(r < 10) {
    num_to_unmap = 5;
    //cout << "X";
  } else if(r < 100) {
    //cout << "*";
    //clear all nodes with overprovisioning
    //SS_CONFIG::SubModel* subModel = _ssModel->subModel();
    //for(ssnode* n : subModel->node_list()) {
    //  if(sched->thingsAssigned(n) > 1) { //TODO FIX THIS NEEDS TO BE UPDATED WITH A SLOT
    //    auto vertices = sched->dfgNodesOf(n); //deliberately copy
    //    for(SSDfgNode* v : vertices) {
    //      //cout << v->name() << "\n";
    //      //cout << v->group_id() << "\n";
    //      if(!v->is_temporal()) {
    //        sched->unassign_dfgnode(v);
    //      }
    //    }
    //  }
    //}

    //for(auto I = ssDFG->nodes_begin(), E=ssDFG->nodes_end();I!=E;++I) {
    //  SSDfgNode* v = *I;
    //  

    //}
  } //else {
    //cout << "u";
  //}

  while(num_to_unmap && sched->num_mapped()) {
    int r = rand_bt(0,8); //upper limit defines ratio of input/output scheduling
    switch (r) {
      case 0:
        if (sched->num_inputs_mapped() != 0) {
          unmap_one_input(ssDFG, sched);
          num_to_unmap--;
        }
        break;
      case 1:
        if (sched->num_outputs_mapped() != 0) {
          unmap_one_output(ssDFG, sched);
          num_to_unmap--;
        }
        break;
      default:
        if (sched->num_insts_mapped() != 0) {
          unmap_one_inst(ssDFG, sched);
          num_to_unmap--;
        }
    }
  }

}



bool SchedulerSimulatedAnnealing::schedule_internal(SSDfg* ssDFG, Schedule*& sched) {
  int max_retries = 100;

  for(int t = 0; t < max_retries; ++t) {
    unmap_some(ssDFG,sched);

    if(_fake_it) {
      //Just map the i/o ports
      map_io_to_completion(ssDFG,sched);
      if(sched->inputs_complete() && sched->outputs_complete()) {
        return true;
      }
    } else {
      //THE REAL THING
      map_to_completion(ssDFG,sched);
      if(sched->isComplete()) {
        //cout << "remppaed after " << t << " tries" << "\n";
        return true;
      } 
    }
  } 
  //cout << "failed to remap\n";
  return false;
}

bool SchedulerSimulatedAnnealing::scheduleNode(Schedule* sched, SSDfgNode* dfgnode) {
  std::vector<std::pair<int, ssnode*>> spots;

  CandidateRouting route1, route2;
  CandidateRouting *bestRouting = &route1, *curRouting = &route2;


  if (auto *dfginst = dynamic_cast<SSDfgInst *>(dfgnode)) {
    spots = fill_inst_spots(sched, dfginst); //all possible candidates based on FU capability
  } else if (auto *dfg_in = dynamic_cast<SSDfgInput *>(dfgnode)) {
    spots = fill_input_spots(sched, dfg_in);
  } else if (auto *dfg_out = dynamic_cast<SSDfgOutput *>(dfgnode)) {
    spots = fill_output_spots(sched, dfg_out);
  }

  //populate a scheduling score for each of canidate ssspot
  if ( (rand() % 1024) == 1) { //Pick a totally random spot
    bool succ = false;
    unsigned int attempt = 0;
    while (!succ && attempt < spots.size()) {
      size_t r2 = rand() % spots.size();

      std::pair<int, int> n_score = scheduleHere(sched, dfgnode, spots[r2], *curRouting);
      bool failed = (n_score == fscore);

      if (!failed) {
        apply_routing(sched, dfgnode, spots[r2], curRouting);
        return true;
      } else {
        attempt++;
      }
    }
    return false; // couldn't find one

  } else {
    std::pair<int, int> bestScore = make_pair(INT_MIN, INT_MIN);
    std::pair<int, ssnode *> bestspot(-1, nullptr);
    int num_found = 0;

    for (unsigned i = 0; i < spots.size(); i++) {

      curRouting->clear();
      std::pair<int, int> n_score = scheduleHere(sched, dfgnode, spots[i], *curRouting);

      bool failed = (n_score == fscore);
      int routes = n_score.second;

      if (!failed) {
        num_found++;
        apply_routing(sched, dfgnode, spots[i], curRouting);
        int lat = INT_MAX, latmis = INT_MAX, ovr = INT_MAX, 
            agg_ovr = INT_MAX, max_util = INT_MAX;
        std::pair<int, int> curScore = obj(sched, lat, latmis, ovr, agg_ovr, max_util);
        curScore.second -= routes;
        sched->unassign_dfgnode(dfgnode); //rip it up!

        if (curScore > bestScore) {
          bestScore = curScore;
          bestspot = spots[i];
          std::swap(bestRouting, curRouting);
        }
      }

    }

    if (num_found > 0) {
      apply_routing(sched, dfgnode, bestspot, bestRouting);
    }

    return (num_found > 0);
  }
}

std::pair<int,int> SchedulerSimulatedAnnealing::scheduleHere(Schedule* sched,
    SSDfgNode* n, pair<int, SS_CONFIG::ssnode*> here, CandidateRouting& candRouting) {

  pair<int, int> score = make_pair(0, 0);

  //TODO: make work for decomp-CGRA
  for (auto source_dfgedge : n->in_edges()) {
    SSDfgNode *source_dfgnode = source_dfgedge->def();     //could be input node also

    if (sched->link_count(source_dfgedge) != 0) {
      cerr << "Edge: " << source_dfgedge->name() << " is already routed!\n";
      assert(0);
    }

    //cout << " try " << source_dfgedge->name() << " " 
    //     << source_dfgedge << " " << here->name() << " " 
    //     << score.first << "," << score.second << "\n";

    //route edge if source dfgnode is scheduled
    if (sched->is_scheduled(source_dfgnode)) {
      auto source_loc = sched->location_of(source_dfgnode); //scheduled location


      //route using source node, ssnode
      pair<int, int> tempScore = route(sched, source_dfgedge, source_loc, here, candRouting);
      if (tempScore == fscore) {
        //cout << "failed to route def\n";
        return fscore;
      }

      score = score + tempScore;
    }
  }

  for (auto use_dfgedge : n->uses()) {
    SSDfgNode *use_dfgnode = use_dfgedge->use();

    if (sched->link_count(use_dfgedge) != 0) {
      cerr << "Edge: " << use_dfgedge->name() << " is already routed!\n";
      assert(0);
    }

    //cout << " try " <<  n->name() << " " << here->name() << " " << score.first << " " << score.second << "\n";

    //route edge if source dfgnode is scheduled
    if (sched->is_scheduled(use_dfgnode)) {
      //ssnode *use_loc = sched->locationOf(use_dfgnode);
      auto use_loc = sched->location_of(use_dfgnode);

      pair<int, int> delta = route(sched, use_dfgedge, here, use_loc, candRouting);
      if (delta == fscore) {
        //cout << "failed to route use\n";
        return fscore;
      }
      score = score + delta;
    }
  }

  return score;
}

int SchedulerSimulatedAnnealing::routing_cost(SSDfgEdge* edge, int from_slot, int next_slot, sslink *link,
      Schedule* sched, CandidateRouting& candRouting, const pair<int, ssnode*> &dest) {

  SSDfgNode *def_dfgnode = edge->def();
  SSDfgNode *use_dfgnode = edge->use();

  bool is_in = def_dfgnode->type() == SSDfgNode::V_INPUT;
  bool is_out = use_dfgnode->type() == SSDfgNode::V_OUTPUT;
  bool is_io = is_in || is_out;

  bool is_temporal = def_dfgnode->is_temporal();
  bool is_temporal_inst = is_temporal && !is_io;
  bool is_temporal_in = is_temporal && is_in;
  bool is_temporal_out = is_temporal && is_out;

  int internet_dis = abs(from_slot - next_slot);
  internet_dis = min(internet_dis, 8 - internet_dis);

  //For now, links only route on their own network
  //ie. temporal_io and non-temporal route on dedicated network
  if ((!is_temporal_inst && link->max_util() > 1) || (is_temporal_inst && link->max_util() <= 1)) {
    return -1;
  }

  ssnode *next = link->dest();

  //check if connection is closed..
  //0: free
  //1: empty
  //2: already there
  int t_cost;
  if (is_temporal_in) {
    t_cost = sched->temporal_cost_in(link, dynamic_cast<SSDfgInput *>(def_dfgnode)->input_vec());
  } else if (is_temporal_out) {
    t_cost = sched->temporal_cost_out(make_pair(from_slot, link), def_dfgnode,
                                      dynamic_cast<SSDfgOutput *>(use_dfgnode)->output_vec());
  } else { //NORMAL CASE!
    t_cost = sched->temporal_cost(make_pair(from_slot, link), def_dfgnode);
    for (auto elem : candRouting.routing[make_pair(from_slot, link)]) {
      if (elem->def() == def_dfgnode) {
        t_cost = 0;
        break;
      }
    }
  }

  if (t_cost != 0) { //empty

    if (candRouting.routing.count(make_pair(from_slot, link))) {
      for (auto elem : candRouting.routing[make_pair(from_slot, link)]) {
        auto *cur_node = elem->def();
        if (cur_node == def_dfgnode) {
          t_cost = 0;
          break;
        }
        //kind of horribly ugly
        if (is_temporal_in) {
          bool flag = false;
          if (auto input_vec = dynamic_cast<SSDfgInput *>(def_dfgnode)) {
            flag |= sched->input_matching_vector(cur_node, input_vec->input_vec());
          }
          if (auto output_vec = dynamic_cast<SSDfgOutput *>(use_dfgnode)) {
            flag |= sched->output_matching_vector(cur_node, output_vec->output_vec());
          }
          if (flag) {
            t_cost = 0;
            break;
          }
        }
        if (is_temporal_out) {

        }
      }
      //TODO: FIXME OH NOOOOO
      //if (t_cost==1) t_cost=1+edges.size(); //empty before
      //t_cost+=edges.size();
    }
  }

  if (t_cost >= 2) { //square law avoidance of existing routes
    if (!is_temporal_inst) {
      return (t_cost) * (t_cost) * 10;
    }
  }
  bool is_dest = (next == dest.second && next_slot == dest.first);

  ssfu *fu = dynamic_cast<ssfu *>(next);
  if (fu && sched->dfgNodeOf(fu) && !is_dest)
    return -1;  //stop if run into fu
  if (t_cost == 0) {
    return 0 + internet_dis; //free link because schedule or candidate routing already maps
  } else {
    if (fu && sched->isPassthrough(fu))
      return -1; //someone else's pass through
    bool passthrough = (fu && !is_dest);
    return t_cost + passthrough * 1000 + internet_dis;
  }
}


pair<int,int> SchedulerSimulatedAnnealing::route(Schedule* sched, SSDfgEdge* dfgedge,
        pair<int, ssnode*> source, pair<int, ssnode*> dest, CandidateRouting& candRouting) {

  if (source.second == dest.second) {
    sslink *link = source.second->get_cycle_link();
    assert(link);
    if (source.first < dest.first) {
      for (int i = source.first + 1; i <= dest.first; ++i)
        candRouting.routing[make_pair(i, link)].insert(dfgedge);
    } else if (source.first > dest.first) {
      for (int i = source.first - 1; i >= dest.first; --i)
        candRouting.routing[make_pair(i, link)].insert(dfgedge);
    } else {
      //I am not sure if this will happen
      candRouting.routing[make_pair(source.first, link)].insert(dfgedge);
    }
    auto &prop = candRouting.edge_prop[dfgedge];
    prop.num_links = 0;
    prop.num_passthroughs = 0;
    return std::make_pair(0, 2 + abs(source.first - dest.first));
  }

  pair<int, int> score = route_minimize_distance(sched, dfgedge, source, dest, candRouting);
  return score;
}



