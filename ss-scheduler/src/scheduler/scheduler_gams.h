#ifndef __SS__SCHEDULE_GAMS_H__
#define __SS__SCHEDULE_GAMS_H__

#include <iostream>
#include <fstream>

#include "scheduler.h"

class GamsScheduler : public Scheduler {
public:
  GamsScheduler(SS_CONFIG::SSModel* ssModel) :
    Scheduler(ssModel),
    _gams_files_setup(false), _gams_work_dir("gams"),
    _showGams(true), _mipstart(false), _sll(false) { }

  virtual bool schedule(SSDfg* ssDFG, Schedule*& schedule);
  virtual bool schedule_internal(SSDfg* ssDFG, Schedule*& schedule);

  void showGams(bool show) {
    _showGams=show; 
  }
  void setMipstart(bool mipstart) {_mipstart=mipstart;}

  void setSll(bool sll) {_sll=sll;}
  void print_mipstart(std::ofstream& ofs,  Schedule* sched, SSDfg* ssDFG, 
                      bool fix);

  protected:

  bool _gams_files_setup;
  std::string _gams_work_dir;
  bool _showGams, _mipstart;
  bool _sll;

  std::unordered_map<std::string, std::pair<SS_CONFIG::ssnode*,int> >  gamsToNode;
  std::unordered_map<std::string, std::pair<SS_CONFIG::sslink*,int> > gamsToLink;
  std::unordered_map<std::string, std::pair<SS_CONFIG::ssswitch*,int>>  gamsToSwitch;


  std::unordered_map<std::string, SSDfgNode*> gamsToDfgnode;
  std::unordered_map<std::string, SSDfgEdge*> gamsToDfgedge;
  std::unordered_map<std::string, std::pair<bool,int> >  gamsToPortN;  
  std::unordered_map<std::string, SSDfgVec*>  gamsToPortV;  

};

#endif
