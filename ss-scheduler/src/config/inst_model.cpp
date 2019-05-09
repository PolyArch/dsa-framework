#include "inst_model.h"

#include <string.h>
#include <stdlib.h>
#include "model_parsing.h"

using namespace SS_CONFIG;
using namespace std;

//constructor based on input stream
InstModel::InstModel(char* filename) {
    
    ifstream ifs(filename, ios::in);
    
    if(ifs.fail()) {
        cerr << "Could Not Open: " << filename << "\n";
        return;
    }
    
    char line[512];
    while(ifs.good())
    {
        //string line;
        ifs.getline(line,512);
        
        string str_line=string(line);
        
        ModelParsing::trim(str_line);
        
        //Empty line or the first line
        if(str_line[0]=='#' || str_line.empty()) continue;
        
        ConfigInst* inst = new ConfigInst();
        
        char* token;
        token = strtok (line," ");
        string str_name(token);
        inst->setName(str_name);
        
        token = strtok (nullptr," ");
        inst->setBitwidth(atoi(token));
        
        token = strtok (nullptr, " ");
        inst->setNumOperands(atoi(token));

        token = strtok (nullptr, " ");
        inst->setLatency(atoi(token));

        token = strtok (nullptr, " ");
        inst->setThroughput(atoi(token));

        _instList.push_back(inst);
    }
    
    
}

void InstModel::printCFiles(char* header_file, char* cpp_file) {
    
  // -------------------------print header file -----------------------------
    ofstream ofs(header_file, ios::out);
    ofs <<
    "//This file generated from inst_model.cpp -- Do not edit.  Do not commit to repo.\n"
    "#ifndef __SS_INST_H__\n"
    "#define __SS_INST_H__\n"
    "\n"
    "#include <string>\n"
    "#include <string.h>\n"
    "#include <cstring>\n"
    "#include <assert.h>\n"
    "#include <iostream>\n"
    "#include <vector>\n"
    "#include <complex>\n"
    "#include <algorithm>\n"
    "#include <xmmintrin.h>\n"
    "#include <math.h>\n"
    "#include \"fixed_point.h\"\n"
    "\n"
    "using std::complex;\n"
    "\n"
    "namespace SS_CONFIG {\n"
    "\n"

    "float    as_float(std::uint32_t ui);\n"
    "uint32_t as_uint32(float f);\n"
    "\n"
    "double    as_double(std::uint64_t ui);\n"
    "uint64_t as_uint64(double f);\n"
    "\n"

    "std::complex<float> as_float_complex(uint64_t ui);\n"
    "uint64_t as_uint64(const std::complex<float> &val);\n"
    "\n"


    "enum ss_inst_t {\n"
    "SS_NONE=0,\n"
    "SS_ERR,\n";
    
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "SS_" << _instList[i]->name() << ", \n";
    };
    
    ofs << "SS_NUM_TYPES\n};\n";

    ofs << "\n";
    ofs << "extern int num_ops[" << _instList.size()+2 << "];\n";
    ofs << "extern int bitwidth[" << _instList.size()+2 << "];\n";


    ofs << 
    "\n"
    "ss_inst_t inst_from_string(const char* str);\n"
    "const char* name_of_inst(ss_inst_t inst);\n"
    "int inst_lat(ss_inst_t inst);\n"
    "int inst_thr(ss_inst_t inst);\n";

    //Generate an execute function for all bitwidths
    int    bitwidths[4] = {64,         32,         16,          8};
    string types[4]     = {"uint64_t", "uint32_t", "uint16_t",  "uint8_t"};
    string suffixes[4]  = {"64",       "32",       "16",        "8"};

    for(int i = 0; i < 4; ++i) {
      string dtype = types[i];
      string suffix = suffixes[i];
    
      ofs << dtype << " execute" << suffix << "(ss_inst_t inst, " 
          << "std::vector<" << dtype << ">& ops, std::vector<" << dtype << ">& reg, "
          << "uint64_t& discard, std::vector<bool>& back_array);\n";
    }

    ofs <<
    "\n"
    "};\n\n"
    "#endif\n";
    
    ofs.close();
    
    // -------------------------print cpp file --------------------------------
    
    ofs.open(cpp_file, ios::out);
    
    // inst_from_string
    ofs << 
    "//This file generated from inst_model.cpp -- Do not edit.  Do not commit to repo.\n"
    "#include \"" << header_file << "\"\n\n"

    "float SS_CONFIG::as_float(std::uint32_t ui) {\n"
    "  float f;\n"
    "  std::memcpy(&f, &ui, sizeof(float));\n"
    "  return f;\n"
    "}\n"
    "\n"

    "uint32_t SS_CONFIG::as_uint32(float f) {\n"
    "  uint32_t ui;\n"
    "  std::memcpy(&ui, &f, sizeof(float));\n"
    "  return ui;\n"
    "}\n"
    "\n"

    "double SS_CONFIG::as_double(std::uint64_t ui) {\n"
    "  double f;\n"
    "  std::memcpy(&f, &ui, sizeof(double));\n"
    "  return f;\n"
    "}\n"
    "\n"

    "uint64_t SS_CONFIG::as_uint64(double f) {\n"
    "  uint64_t ui;\n"
    "  std::memcpy(&ui, &f, sizeof(double));\n"
    "  return ui;\n"
    "}\n"
    "\n"

    "std::complex<float> SS_CONFIG::as_float_complex(uint64_t val) {\n"
    "  std::complex<float> res;\n"
    "  std::memcpy(&res, &val, sizeof(val));\n"
    "  return res;\n"
    "}\n"
    "\n"

    "uint64_t SS_CONFIG::as_uint64(const std::complex<float> &val) {\n"
    "  uint64_t res;\n"
    "  std::memcpy(&res, &val, sizeof(val));\n"
    "  return res;\n"
    "}\n"
    "\n"

    "using namespace SS_CONFIG;\n\n"
    "ss_inst_t SS_CONFIG::inst_from_string(const char* str) {\n"
    "  if(strcmp(str,\"NONE\")==0) return SS_NONE;\n";
    
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "  else if(strcmp(str,\"" << _instList[i]->name() << "\")==0) return SS_" << _instList[i]->name() << ";\n";
    }
    ofs << "  else { fprintf(stderr,\"Config Library does not understand string\\\"%s\\\"\\n\",str); assert(0); return SS_ERR;}\n\n";
    
    ofs << "}\n\n";
    
       
    // Properties of Instructions
    
    // name_of_inst
    ofs << 
    "const char* SS_CONFIG::name_of_inst(ss_inst_t inst) {\n"
    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SS_" << _instList[i]->name() << ": return \"" << _instList[i]->name() << "\";\n";
    }
    ofs << "case SS_NONE: return \"NONE\";\n";
    ofs << "case SS_ERR:  assert(0); return \"ERR\";\n";
    ofs << "case SS_NUM_TYPES:  assert(0); return \"ERR\";\n";
    ofs << "    default: assert(0); return \"DEFAULT\";\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";

    
    //FUNCTION: inst_lat 
    ofs <<
    "int SS_CONFIG::inst_lat(ss_inst_t inst) {\n"
    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SS_" << _instList[i]->name() << ": return " << _instList[i]->latency() << ";\n";
    }
    ofs << "    default: return 1;\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";

    //FUNCTION: inst_thr 
    ofs <<
    "int SS_CONFIG::inst_thr(ss_inst_t inst) {\n"
    "  switch(inst) {\n";
    for(unsigned i = 0; i < _instList.size(); ++i) {
        ofs << "    case " << "SS_" << _instList[i]->name() << ": return " << _instList[i]->throughput() << ";\n";
    }
    ofs << "    default: return 1;\n";
    ofs << "  }\n\n";
    ofs << "}\n\n";

    // num_ops_array
    ofs << "int SS_CONFIG::num_ops[" << _instList.size()+2 << "]={0, 0\n";
    ofs << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    for(unsigned i = 0; i < _instList.size(); ++i) {
      ofs << ", " << _instList[i]->numOps();
      if(i%16==15) {
        ofs << "\n";
        ofs << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
      }
    }
    ofs << "};\n\n";

    // bitwidth_array
    ofs << "int SS_CONFIG::bitwidth[" << _instList.size()+2 << "]={0, 0\n";
    ofs << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    for(unsigned i = 0; i < _instList.size(); ++i) {
      ofs << ", " << _instList[i]->bitwidth();
      if(i%16==15) {
        ofs << "\n";
        ofs << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
      }
    }
    ofs << "};\n\n";


    //FUNCTION: execute()
    for(int i = 0; i < 4; ++i) {
      int bitwidth = bitwidths[i];
      string dtype = types[i];
      string suffix = suffixes[i];

      ofs << dtype << " " << "SS_CONFIG::execute" << suffix << "(ss_inst_t inst, " 
          << "std::vector<" << dtype << ">& ops, std::vector<" << dtype << ">& reg, "
          << "uint64_t& discard, std::vector<bool>& back_array) {\n";

     //somwhere below is an implementation of pass through, is it though? (tony, 2018)

      ofs <<  dtype << "& accum = reg[0]; \n"
      "  assert(ops.size() <= 4); \n" 
      "  assert(ops.size() <=  (unsigned)(num_ops[inst]+1)); \n" 
      "  if((ops.size() > (unsigned)num_ops[inst]) && (ops[ops.size()] == 0)) { \n" 
      "    return ops[0];\n"
      "  }\n"

      "  switch(inst) {\n";
      for(unsigned i = 0; i < _instList.size(); ++i) {
          ConfigInst* inst = _instList[i];

          if(inst->bitwidth() != bitwidth) continue; // TODO: later implement autovectorization
         
          ofs << "    case " << "SS_" << inst->name() << ": {";
          string inst_code_name = "insts" + suffix + "/" + inst->name() + ".h";
          ifstream f(inst_code_name.c_str());

          if (f.good()) {
            std::string line;
            ofs << "\n";
            while (std::getline(f, line)) {
              ofs << "      " << line << "\n";
            }
            ofs << "    };\n";
          } else {
            ofs << "assert(0 && \"Instruction Not Implemented, add it to insts folder\");";
            ofs << "};\n";
          }
      }
      ofs << "    default: assert(0 && \"Instruction not defined (for this bitwidth)\"); return 1;\n";
      ofs << "  }\n\n";
      ofs << "}\n\n";   
    }
    ofs.close();
}

int main(int argc, char** argv)
{
    if(argc!=4) {
        std::cout << "Usage:\n inst_model [input file] [header file] [cpp file]\n";
        return 1;
    }

    InstModel* instModel = new InstModel(argv[1]);
    instModel->printCFiles(argv[2],argv[3]);
    return 0;
}

