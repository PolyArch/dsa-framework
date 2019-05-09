#ifndef __SS_FU_MODEL_H__
#define __SS_FU_MODEL_H__

#include <string>
#include <vector>
#include <set>
#include <map>
#include <assert.h>

#include "ssinst.h"

namespace SS_CONFIG {

class func_unit_def {
public:
    func_unit_def(std::string name_in) {
        _name = name_in;
        add_cap(ss_inst_t::SS_Copy); // All FUs can copy!
    }

    std::string name() {return _name;}
    
    void add_cap(ss_inst_t ss_inst) { _cap.insert(ss_inst); }
    void set_encoding(ss_inst_t ss_inst, unsigned i) { 
      if(i==0) {
        assert(0 && "Encoding for Instruction cannot be zero.  Zero is reserved for Blank");
      }
      if(i==1) {
        assert(0 && "Encoding for Instruction cannot be 1.  1 is reserved for Copy");
      }
      _cap2encoding[ss_inst]=i; 
      _encoding2cap[i]=ss_inst;
    }
    
    bool is_cap(ss_inst_t inst) { return _cap.count(inst)>0; }
    unsigned encoding_of(ss_inst_t inst) { 
      if(inst == SS_Copy) {
        return 1;
      } else {
        return _cap2encoding[inst]; 
      }
    }
    
    ss_inst_t inst_of_encoding(unsigned i) {
      if(i==1) {
        return SS_Copy;
      }
      assert(_encoding2cap.count(i));
      return _encoding2cap[i];
    }
    
private:    
    std::string _name;
    std::set<ss_inst_t> _cap;
    std::map<ss_inst_t, unsigned> _cap2encoding;
    std::map<unsigned, ss_inst_t> _encoding2cap;

    friend class FuModel;
};

class FuModel {
    public:
        FuModel(std::istream& istream);
        func_unit_def* GetFUDef(char*);
        func_unit_def* GetFUDef(std::string& fu_string);
       
    private:
        void AddCapabilities(func_unit_def& fu, std::string& cap_string);
        
        std::vector<func_unit_def> func_defs;        
};

}

#endif
