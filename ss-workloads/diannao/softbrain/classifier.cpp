#include <iostream>
#include "dnn.hpp"
#include "softbrain.hpp"
#include <inttypes.h>

#include "red32to1sig.dfg.h"
#include "../../common/include/ss_insts.h"

using namespace std;

// Problem Size
//#define Nn 100  // Number of Output Layers
//#define Ni 200  // Number of Input  Layers

#ifndef Nn
#define Nn 128  // Number of Output Layers
#define Ni 224  // Number of Input  Layers
#endif

#ifndef Tii
// Tiling Sizes
#define Tnn 32  
#define Tii 32
//#define Tn 5
//#define Ti 25
#define Tn 16
#define Ti 16
#endif

//Arrays:
VTYPE synapse[Nn][Ni] __attribute__((aligned(64)));
VTYPE neuron_i[Ni] __attribute__((aligned(64)));
VTYPE neuron_n[Nn] __attribute__((aligned(64))),    neuron_n2[Nn] __attribute__((aligned(64)));


int classifier_layer(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni], VTYPE (&neuron_n)[Nn]) {
  int total_calc=0;
  for (int n = 0; n < Nn; n++) {
    VTYPE temp=0;
    for (int i = 0; i < Ni; i++) {
      temp += synapse[n][i] * neuron_i[i];
    }
    neuron_n[n] = sigmoid(temp);
  }
  return total_calc;
}

// CGRA Pipe
#define PIPEWIDTH 8 // adders at mouth of 1 CGRA pipe
#define PIPEDEPTH 32 // approx. depth of CGRA pipeline
int classifier_layer_sb(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni], VTYPE (&neuron_n)[Nn], bool profiling = false) {
  // Fits in scratchpad? (should be true for our benches)
  //if(Ni > SCRATCHSIZE){
  //  cout << "Error: inputs do not fit in scratch for classifier layer" << endl;
  //  return -1;
  //}     

  // Handle class3, Nn = 32  -- not sure what this does
  int pipedepth = PIPEDEPTH < Nn ? PIPEDEPTH : Nn;
  //if(Nn < PIPEDEPTH){
  //  pipedepth = Nn;
  //}

  // Stream in CGRA config (do this somewhere else?) 
  if (profiling)
    begin_roi();
  SS_CONFIG(red32to1sig_config, red32to1sig_size);

  // Stream in inputs to scratch
  SS_DMA_SCRATCH_LOAD(&neuron_i, sizeof(VTYPE)*4, sizeof(VTYPE)*4, Ni/4, 0); 
  SS_WAIT_SCR_WR();

  int Tail = Ni % (PIPEDEPTH * 4);
  int Ni_ = Ni - Tail;
  for(int n = 0; n < Nn; n += pipedepth){
    SS_CONST(P_red32to1sig_acc, 0, pipedepth); 

    for(int i = 0; i < Ni_; i+= PIPEWIDTH*4){
      // Enable sigmoid on final itr
      //if(i + PIPEWIDTH*4 < Ni){   
        SS_CONST(P_red32to1sig_pred, 0, pipedepth); 
        SS_RECURRENCE(P_red32to1sig_out, P_red32to1sig_acc, pipedepth);
      //} else {
        //SS_CONST(P_red32to1sig_pred, 1, pipedepth); 
      //}
      
      SS_DMA_READ(&synapse[n][i],  sizeof(VTYPE)*Ni, 4*sizeof(VTYPE)*PIPEWIDTH, pipedepth, P_red32to1sig_S); //Read Synapses 
      SS_SCR_PORT_STREAM(i*sizeof(VTYPE), 0, 4*sizeof(VTYPE)*PIPEWIDTH, pipedepth, P_red32to1sig_N); //Read Neurons
    }

    if (Ni_ != Ni) {
      SS_CONST(P_red32to1sig_pred, 1, pipedepth); 
      SS_DMA_READ(&synapse[n][Ni_],  sizeof(VTYPE)*Ni, 4*sizeof(VTYPE)*PIPEWIDTH, pipedepth, P_red32to1sig_S); //Read Synapses 
      SS_SCR_PORT_STREAM(Ni_*sizeof(VTYPE), 0, 4*sizeof(VTYPE)*PIPEWIDTH, pipedepth, P_red32to1sig_N); //Read Neurons
    }

    // write completed outputs out to memory
    SS_DMA_WRITE_SHF16(P_red32to1sig_out, 4*sizeof(VTYPE), 4*sizeof(VTYPE), pipedepth/4, &neuron_n[n]); 
  } 

  SS_WAIT_ALL();
  if (profiling)
    end_roi();
  return 0;
}


void fill_classifier(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni], 
    VTYPE (&neuron_n)[Nn],   VTYPE (&neuron_n2)[Nn]) {
  for(int n = 0; n < Nn; ++n) {
    for(int i = 0; i < Ni; ++i) {
      synapse[n][i] = rand()%(1+n/4); //n*Ni+i;
    }
  }
  for(int i = 0; i < Ni; ++i) {
    neuron_i[i] = rand()%16; //i;
  }
  for(int n = 0; n < Nn; ++n) { neuron_n[n] = 0; //i;
    neuron_n2[n] = 0; //i;
  }
}

int classifier_layer_blocked(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni], 
                              VTYPE (&neuron_n)[Nn]) {
  int total_calc=0;
  VTYPE sum[Nn]={0};
  for (int nnn = 0; nnn < Nn; nnn += Tnn) { // tiling for output neurons;
    for (int iii = 0; iii < Ni; iii += Tii) { // tiling for input neurons;
      for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {
        for (int ii = iii; ii < iii + Tii; ii += Ti) {
          //total_calc++;

          // — Original code —
          for (int n = nn; n < nn + Tn; n++) {
            VTYPE sum_sc=0;
            for (int i = ii; i < ii + Ti; i++) {
              sum_sc += (synapse[n][i] * neuron_i[i]);
              //sum_sc += synapse[n][i] * i;
            }
            sum[n]+=sum_sc;
          }
        }
      }
    }
    for (int nn = nnn; nn < nnn + Tnn; nn++) {
      neuron_n[nn] = sigmoid(sum[nn]);
    }
  }
  return total_calc;
}

int main(int argc, char** argv) {
  fill_classifier(synapse,neuron_i,neuron_n,neuron_n2);

  if(argc==3) {
  
  } else if(argc==2) {
    //begin_roi();
    #ifdef SB
    int calc = classifier_layer_sb(synapse,neuron_i,neuron_n);  
    #else
    int calc = classifier_layer_blocked(synapse,neuron_i,neuron_n);  
    #endif
    //end_roi();

    if(calc > 0) {
      cout << "calc: " << calc << "\n";
    }
    //cout << "Perf Run Complete\n";
  } else {
    //int calc  = classifier_layer(synapse,neuron_i,neuron_n);

    classifier_layer_sb(synapse,neuron_i,neuron_n2,false);
    //begin_roi();
    #ifdef SB
    int calc2 = classifier_layer_sb(synapse,neuron_i,neuron_n2,true);
    #else
    int calc2 = classifier_layer_blocked(synapse,neuron_i,neuron_n2);  
    #endif
    //end_roi();
    sb_stats();
 
    //cout << "C1: " << calc << " C2: " << calc2 << "\n";
  
    //compare(neuron_n,neuron_n2,Nn);
  
    cout << "mults: " << Nn*Ni <<  " sigmoids: " << Nn << "\n";
  }
}

