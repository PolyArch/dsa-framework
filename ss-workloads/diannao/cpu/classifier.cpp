#include <iostream>
#include "dnn.hpp"

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

void fill_classifier(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni]) {
  for(int n = 0; n < Nn; ++n) {
    for(int i = 0; i < Ni; ++i) {
      synapse[n][i] = n*Ni+i;
    }
  }
  for(int i = 0; i < Ni; ++i) {
    neuron_i[i] = i;
  }
}

int classifier_layer_blocked(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni], 
                              VTYPE (&neuron_n)[Nn]) {
  int total_calc=0;
  VTYPE sum[Nn]={0};
  for (int nnn = 0; nnn < Nn; nnn += Tnn) { // tiling for output neurons;
    for (int iii = 0; iii < Ni; iii += Tii) { // tiling for input neurons;
      for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {
/*        for (int n = nn; n < nn + Tn; n++) {
          cout << "i-n" << n << " " << nn+Tn << "\n";
          sum[n] = 0;
        }*/
        for (int ii = iii; ii < iii + Tii; ii += Ti) {
          //total_calc++;

          // — Original code —
          for (int n = nn; n < nn + Tn; n++) {
            VTYPE sum_sc=0;
            for (int i = ii; i < ii + Ti; i++) {
              sum_sc += (synapse[n][i] * neuron_i[i])>>1;
              //sum_sc += synapse[n][i] * i;
            }
            sum[n]+=sum_sc>>1;
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

int classifier_layer(VTYPE (&synapse)[Nn][Ni], VTYPE (&neuron_i)[Ni], VTYPE (&neuron_n)[Nn]) {
  int total_calc=0;
  // — Original code —
  for (int n = 0; n < Nn; n++) {
    VTYPE temp=0;
    for (int i = 0; i < Ni; i++) {
      temp += (synapse[n][i] * neuron_i[i])>>1;
    }
    neuron_n[n] = sigmoid(temp);
//    total_calc++;
  }
  return total_calc;
}

int main(int argc, char** argv) {
  fill_classifier(synapse,neuron_i);

  begin_roi();
  if(argc==3) {
  
 // } else if(argc==2 && string(argv[1])=="perf") {
  } else if(argc==2) {
    int calc = classifier_layer_blocked(synapse,neuron_i,neuron_n);
    if(calc > 0) {
      cout << "calc: " << calc << "\n";
    }
    //cout << "Perf Run Complete\n";
  } else {
    int calc  = classifier_layer_blocked(synapse,neuron_i,neuron_n);
    int calc2 = classifier_layer(synapse,neuron_i,neuron_n2);
  
    cout << "C1: " << calc << " C2: " << calc2 << "\n";
  
    compare(neuron_n,neuron_n2,Nn);
  
    cout << "mults: " << Nn*Ni <<  " sigmoids: " << Nn << "\n";
  }
  end_roi();
}

