#ifndef __INST_MODEL_H__
#define __INST_MODEL_H__

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace SS_CONFIG {

// Instruction Class
// Stores attributes like it's name, latency, etc...
class ConfigInst {
    public:
        std::string name()               { return _name; }
        void setName(std::string& name) { _name=name; }
        
        int bitwidth()              { return _bitwidth; }
        void setBitwidth(int b)     { _bitwidth=b; }

        int latency()               { return _latency; }
        void setLatency(int lat)    { _latency=lat; }

        int  throughput()           { return _throughput; }
        void setThroughput(int thr) { _throughput=thr; }

        int numOps()                     { return _num_ops; }
        void setNumOperands(int n_ops)    { _num_ops=n_ops; }

        
    private:
        std::string _name;
        int _latency;
        int _throughput; //technically 1/throughput in cycles
        int _num_ops;
        int _bitwidth;
};

class InstModel {
    public:
        InstModel(char* filename);          //read the file and populate the instructions
        //DyInst* GetDyInstByName(std::string& name);
        
        void printCFiles(char* header, char* cpp);
        
    private:
        std::vector<ConfigInst*> _instList;
};






}

#endif
