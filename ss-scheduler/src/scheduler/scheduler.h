#ifndef __SS_SCHEDULER_H__
#define __SS_SCHEDULER_H__

#include "ssdfg.h"
#include "model.h"
#include "schedule.h"

#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <chrono>
#include <random>
#include <stdlib.h>
#include <memory>
#include <boost/functional.hpp>

#define MAX_ROUTE 100000000

using usec = std::chrono::microseconds;
using get_time = std::chrono::steady_clock;


template <typename T,typename U>                           
std::pair<T,U> operator+(const std::pair<T,U> & l,
                         const std::pair<T,U> & r) {   
    return {l.first+r.first,l.second+r.second};
}        

template <typename T,typename U>                           
std::pair<T,U> operator-(const std::pair<T,U> & l,
                         const std::pair<T,U> & r) {   
    return {l.first-r.first,l.second-r.second};
}        

class CandidateRouting {
public:
  struct EdgeProp {
    int num_links = 0;
    int num_passthroughs = 0;
    std::unordered_set<sslink *> links;
  };


  void take_union(CandidateRouting &r) {
    for (auto &link_iter : r.routing) {
      for (SSDfgEdge *edge : link_iter.second) {
        routing[link_iter.first].insert(edge);
      }
    }
    for (auto &edge_iter : r.edge_prop) {
      SSDfgEdge *edge = edge_iter.first;
      assert(edge_prop.count(edge) == 0);
      edge_prop[edge] = edge_iter.second;
    }
  }

  std::unordered_map<std::pair<int, SS_CONFIG::sslink*>,
    std::unordered_set<SSDfgEdge*>, boost::hash<std::pair<int, SS_CONFIG::sslink*>>> routing;

  std::unordered_map<SSDfgEdge *, EdgeProp> edge_prop;

  void fill_lat(Schedule *sched,
                int &min_node_lat, int &max_node_lat, bool print = false) {
    min_node_lat = 0; //need minimax, so that's why this is odd
    max_node_lat = MAX_ROUTE;

    if (edge_prop.empty())
      return;

    SSDfgNode *n = (*edge_prop.begin()).first->use();
    bool output = dynamic_cast<SSDfgOutput *>(n);

    for (auto edge : edge_prop) {
      SSDfgEdge *source_dfgedge = edge.first;
      auto i = edge_prop[source_dfgedge];
      int num_links = i.num_links;
      int num_passthroughs = i.num_passthroughs;

      auto p = sched->lat_bounds(source_dfgedge->def());

      int min_inc_lat = p.first + num_links;
      int max_inc_lat = p.second + num_links +
                        sched->ssModel()->maxEdgeDelay() * ((!output) + num_passthroughs);

      if (print) {
        std::cout << "  links: " << num_links << " pts: " << num_passthroughs << "\n";
        std::cout << "  b low: " << p.first << " pts: " << p.second << "\n";
        std::cout << "  max_extra:" << sched->ssModel()->maxEdgeDelay() * ((!output) + num_passthroughs) << "\n";
      }

      if (min_inc_lat > min_node_lat) min_node_lat = min_inc_lat;
      if (max_inc_lat < max_node_lat) max_node_lat = max_inc_lat;
    }
    if (print) {
      std::cout << "  min_inc_lat" << min_node_lat << " " << max_node_lat << "\n";
    }

  }


  void clear() {
    routing.clear();
    edge_prop.clear();
  }
};


class Scheduler {
public:
  Scheduler(SS_CONFIG::SSModel *ssModel) : _ssModel(ssModel),
                                           _optcr(0.1f), _optca(0.0f), _reslim(100000.0f) {}

  bool check_res(SSDfg *ssDFG, SSModel *ssmodel);

  virtual bool schedule(SSDfg *ssDFG, Schedule *&schedule) = 0;

  bool verbose;
  bool suppress_timing_print = false;

  void set_max_iters(int i) { _max_iters = i; }

  std::string str_subalg;

  std::string AUX(int x) {
    return (x == -1 ? "-" : std::to_string(x));
  }

  double total_msec() {
    auto end = get_time::now();
    auto diff = end - _start;
    return ((double) std::chrono::duration_cast<usec>(diff).count()) / 1000.0;
  }

  virtual bool schedule_timed(SSDfg *ssDFG, Schedule *&sched) {
    _start = get_time::now();

    bool succeed_sched = schedule(ssDFG, sched);

    if (verbose && !suppress_timing_print) {
      printf("sched_time: %0.4f seconds\n", total_msec() / 1000.0);
    }

    return succeed_sched;
  }

  void setGap(float relative, float absolute = 1.0f) {
    _optcr = relative;
    _optca = absolute;
  }

  void setTimeout(float timeout) { _reslim = timeout; }

  //virtual void unroute(Schedule* sched, SSDfgEdge* dfgnode,
  //                     SS_CONFIG::ssnode* source);

  bool running() {return !_should_stop;}
  void stop() {_should_stop=true;}

protected:
  SS_CONFIG::SSModel *getSSModel() { return _ssModel; }

  SS_CONFIG::SSModel *_ssModel;

  int _max_iters = 20000;
  bool _should_stop = false;

  float _optcr, _optca, _reslim;
  std::chrono::time_point<std::chrono::steady_clock> _start;

  std::shared_ptr<Schedule*> best, current;
};




class HeuristicScheduler : public Scheduler {
public:

  HeuristicScheduler(SS_CONFIG::SSModel *ssModel) : Scheduler(ssModel),
                                                    fscore(std::make_pair(MAX_ROUTE, MAX_ROUTE)) {}

  virtual bool scheduleNode(Schedule *, SSDfgNode *) = 0;

  virtual std::pair<int, int> scheduleHere(Schedule *, SSDfgNode *, std::pair<int, SS_CONFIG::ssnode *>,
                                           CandidateRouting &) = 0;

  virtual std::pair<int, int> route(Schedule *sched, SSDfgEdge *dfgnode,
                                    std::pair<int, SS_CONFIG::ssnode *> source,
                                    std::pair<int, SS_CONFIG::ssnode *> dest,
                                    CandidateRouting &) = 0;

  virtual int
  routing_cost(SSDfgEdge *, int, int, sslink *, Schedule *, CandidateRouting &, const std::pair<int, ssnode *> &);

  std::pair<int, int> route_minimize_distance(Schedule *sched, SSDfgEdge *dfgnode,
                                              std::pair<int, SS_CONFIG::ssnode *> source,
                                              std::pair<int, SS_CONFIG::ssnode *> dest,
                                              CandidateRouting &);

protected:
  bool assignVectorInputs(SSDfg *, Schedule *);

  bool assignVectorOutputs(SSDfg *, Schedule *);

  void apply_routing(Schedule *, CandidateRouting *);

  void apply_routing(Schedule *, SSDfgNode *, std::pair<int, SS_CONFIG::ssnode *>, CandidateRouting *);

  std::vector<std::pair<int, ssnode *>> fill_input_spots(Schedule *, SSDfgInput *);

  std::vector<std::pair<int, ssnode *>> fill_output_spots(Schedule *, SSDfgOutput *);

  // Find all the candidate spots for the given instruction.
  std::vector<std::pair<int, SS_CONFIG::ssnode *>> fill_inst_spots(Schedule *, SSDfgInst *);

  const std::pair<int, int> fscore;

  void random_order(int n, std::vector<int> &order);

  std::vector<bool> rand_node_choose_k(int k,
                                       std::vector<ssnode *> &input_nodes,
                                       std::vector<ssnode *> &output_nodes);

  void rand_n_choose_k(int n, int m, std::vector<int> &indices);

  int rand_bt(int s, int e) {
    return rand() % (e - s) + s;
  }

  int _route_times = 0;
};

#endif
