#include "model.h"
#include <assert.h>
#include <iostream>
#include <fstream>

using namespace std;
using namespace SB_CONFIG;

int main(int argc, char** argv) {
  char *filename = argv[1]; //"softbrain8x8.sb_model";

  SbModel model(filename);
  
  ofstream graphvizfile("atest.dot", ios::out);
  if(graphvizfile.fail()) {
    cerr << "Could Not Open: " << filename << "\n";
    return 1;
  }
  
  model.subModel()->PrintGraphviz(graphvizfile);
  
  ofstream kindgamsfile("softbrain_kind.gams", ios::out);
  if(kindgamsfile.fail()) {
    cerr << "Could Not Open: " << filename << "\n";
    return 1;
  }
  
  model.printGamsKinds(kindgamsfile);
  
  ofstream sbgamsfile("softbrain_model.gams", ios::out);
  if(sbgamsfile.fail()) {
    cerr << "Could Not Open: " << filename << "\n";
    return 1;
  }
  
  std::unordered_map<std::string,std::pair<SB_CONFIG::sbnode*,int> > gamsToSbnode;  
  std::unordered_map<std::string,std::pair<SB_CONFIG::sblink*,int> > gamsToSblink;
  std::unordered_map<std::string,std::pair<SB_CONFIG::sbswitch*,int> > gamsToSbswitch;
  std::unordered_map<std::string,std::pair<bool,int> >  gamsToPortN;  
  model.subModel()->PrintGamsModel(sbgamsfile,gamsToSbnode,
                                   gamsToSblink,gamsToSbswitch,gamsToPortN,1);
 
  /*
  printf( "Digraph G { \n" ) ;

  //switches
  for (unsigned i = 0; i < model.sizex+1; ++i) {
    for (unsigned j = 0; j < model.sizey+1; ++j) {
      printf("S%04X%04X [ label = \" Switch[%d][%d] \" ];\n", i, j, i, j);
    }
  }

  assert(model.fu_array.size() == model.sizex);
  for (unsigned i = 0; i < model.sizex; ++i) {
    assert(model.fu_array[i].size() == model.sizey);
  }

  for (unsigned i = 0; i < model.sizex; ++i) {
    for (unsigned j = 0; j < model.sizey; ++j) {
      func_unit *unit = model.fu_array[i][j];
      //vertex
      printf( "F%04X%04X [ label = %s ];\n", i,j, unit->name.c_str());
      //edges
      for (unsigned i1 = i; i1 <= i+1; ++i1) {
	for (unsigned j1 = j; j1 <= j+1; ++j1) {
            printf("F%04X%04X -> S%04X%04X;\n", i,j, i1, j1);
	}
      }
    }
  }

  printf("}\n");
*/
    
}
