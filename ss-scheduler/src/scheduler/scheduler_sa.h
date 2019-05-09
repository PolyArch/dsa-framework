#ifndef __SS__SCHEDULE_SIMULATEDANNEALING_H__
#define __SS__SCHEDULE_SIMULATEDANNEALING_H__

#include "scheduler.h"

class SchedulerSimulatedAnnealing : public HeuristicScheduler {
public:
  void initialize(SSDfg *, Schedule *&);

  SchedulerSimulatedAnnealing(SS_CONFIG::SSModel *ssModel) :
          HeuristicScheduler(ssModel) {}

  bool schedule(SSDfg *, Schedule *&);

  std::pair<int, int> route(Schedule *sched, SSDfgEdge *dfgnode,
                            std::pair<int, SS_CONFIG::ssnode *> source, std::pair<int, SS_CONFIG::ssnode *> dest,
                            CandidateRouting &);

  int routing_cost(SSDfgEdge *, int, int, sslink *, Schedule *, CandidateRouting &, const std::pair<int, ssnode*> &);

  void set_fake_it() { _fake_it = true; }

  bool schedule_internal(SSDfg *ssDFG, Schedule *&sched);

protected:
  std::pair<int, int> obj(Schedule*& sched, int& lat, 
      int& lat_mis, int& ovr, int& agg_ovr, int& max_util); 

  bool schedule_input(SSDfgVecInput *vec, SSDfg *ssDFG, Schedule *sched);

  bool schedule_output(SSDfgVecOutput *vec, SSDfg *ssDFG, Schedule *sched);

  bool scheduleNode(Schedule *sched, SSDfgNode *dfgnode);

  std::pair<int, int> scheduleHere(Schedule *, SSDfgNode *, std::pair<int, SS_CONFIG::ssnode *>, CandidateRouting &);

  void findFirstIndex(std::vector<std::pair<int, int>> &sd, ssio_interface &si,
                      unsigned int numIO, unsigned int &index, bool is_input);

  bool genRandomIndexBW(std::pair<bool, int> &vport_id, std::vector<int> &vport_desc,
                        std::vector<std::pair<int, int>> &sd, ssio_interface &si, 
                        unsigned int size, unsigned int index,
                        Schedule *&sched, bool s);

  bool timingIsStillGood(Schedule *sched);

  bool map_to_completion(SSDfg *ssDFG, Schedule *sched);

  bool map_io_to_completion(SSDfg *ssDFG, Schedule *sched);

  bool map_one_input(SSDfg *ssDFG, Schedule *sched);

  bool map_one_inst(SSDfg *ssDFG, Schedule *sched);

  bool map_one_output(SSDfg *ssDFG, Schedule *sched);

  void unmap_one_input(SSDfg *ssDFG, Schedule *sched);

  void unmap_one_inst(SSDfg *ssDFG, Schedule *sched);

  void unmap_one_output(SSDfg *ssDFG, Schedule *sched);

  void unmap_some(SSDfg *ssDFG, Schedule *sched);

  std::vector<std::pair<int, int>> _sd_in;  //port, length pair
  std::vector<std::pair<int, int>> _sd_out; //port, length pair

  int _max_iters_zero_vio = 1000000000;
  bool _integrate_timing = true;
  int _best_latmis, _best_lat, _best_violation;
  bool _strict_timing = true;

  bool _fake_it = false;

};

#endif
