#include <iostream>
#include <string>
#include "dnn.hpp"

using namespace std;

#define AVG 1

//Problem Size
#ifndef Ny //if Ny is undefined, then assume nothing is defined
  #define Ny 32
  #define Nx 32
  
  #define Kx 4
  #define Ky 4
  //#define Ni 100  //Input Layers == Ouptut Layers
  #define Ni 128
#endif

//slide increment
#ifndef Sy
  #define Sx 1
  #define Sy 1
#endif

#ifndef Tii //Tiling Sizes:
  #define Tii 64
  #define Ti  16
  #define Ty  16
  #define Tx  16
#endif

#define NYPAD (Ny+Ky)
#define NXPAD (Nx+Kx)

#define NYSCL (Ny/Sy)
#define NXSCL (Nx/Sx)


//Arrays:
//VTYPE  neuron_i[NYPAD][NXPAD][Ni];
//VTYPE  neuron_n[NYSCL][NXSCL][Ni];
//VTYPE neuron_n2[NYSCL][NXSCL][Ni];

VTYPE  (*neuron_i)[NYPAD][NXPAD][Ni];
VTYPE  (*neuron_n)[NYSCL][NXSCL][Ni];
VTYPE (*neuron_n2)[NYSCL][NXSCL][Ni];

void fill_pooling(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  int total=0;
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = total+1;
      }
    }
  }
}

int pooling_layer_blocked(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                           VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  int c=0;

  VTYPE value[Ni]={0};
  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int iii = 0; iii < Ni; iii += Tii) {
        // — Original code — (excluding ii loop)
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) {
          int xout = xx/Sx;
          for (int x = xx; x < xx + Tx; x += Sx) {
    
            for (int ii = iii; ii < iii + Tii; ii += Ti) {
              for (int i = ii; i < ii + Ti; i++) {
                value[i] = 0;
              }
    
              for (int ky = 0; ky < Ky; ky++) {
                for (int kx = 0; kx < Kx; kx++) {
                  //c++;
                  for (int i = ii; i < ii + Ti; i++) {
                    #ifdef AVG
                    value[i] += neuron_i[ky + y][kx + x][i];
                    #else
                    value[i] = max(value[i], neuron_i[ky + y][kx + x][i]);
                    #endif
                  }
                }
              }

              for (int i = ii; i < ii + Ti; i++) {
                #ifdef AVG
                neuron_n[yout][xout][i] = value[i] / (Kx * Ky);
                #else
                neuron_n[yout][xout][i] = value[i];
                #endif
              }
            }
            xout++;
          }
          yout++;
        }
      }
    }
  }
  return c;
}

void pooling_layer(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                   VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  VTYPE value[Ni]={0};
  // — Original code —
  int yout = 0;
  for (int y = 0; y < Ny; y += Sy) {
    int xout = 0;
    for (int x = 0; x < Nx; x += Sx) {
      for (int i = 0; i < Ni; i++) {
        value[i]=0;
      }

      for (int ky = 0; ky < Ky; ky++) {
        for (int kx = 0; kx < Kx; kx++) {
          for (int i = 0; i < Ni; i++) {
            #ifdef AVG
            value[i] += neuron_i[ky + y][kx + x][i];
            #else
            value[i] = max(value[i], neuron_i[ky + y][kx + x][i]);
            #endif
          }
        }
      }

      for (int i = 0; i < Ni; i++) {
        #ifdef AVG
        neuron_n[yout][xout][i] = value[i] / (Kx * Ky);
        #else
        neuron_n[yout][xout][i] = value[i];
        #endif
      }
      xout++;
    }
    yout++;
  }
}

int main(int argc, char** argv) {

  neuron_i  = (VTYPE (*)[NYPAD][NXPAD][Ni])malloc(NYPAD*NXPAD*Ni*sizeof(VTYPE));
  neuron_n  = (VTYPE (*)[NYSCL][NXSCL][Ni])malloc(NYSCL*NXSCL*Ni*sizeof(VTYPE));
  neuron_n2 = (VTYPE (*)[NYSCL][NXSCL][Ni])malloc(NYSCL*NXSCL*Ni*sizeof(VTYPE));

  fill_pooling(*neuron_i);

  begin_roi();
  if(argc==3) {

  //cout << "Did nothing\n";

//  } else if(argc==2 && string(argv[1])=="perf") {
  } else if(argc==2) {

    pooling_layer_blocked(*neuron_i,*neuron_n);
    cout << "Perf Run Complete\n";
  } else {
    int calc = pooling_layer_blocked(*neuron_i,*neuron_n);
    pooling_layer(*neuron_i,*neuron_n2);
    
    if(calc > 0) {
      cout << "calc: " << calc << "\n";
    }

    compare((VTYPE*)*neuron_n,(VTYPE*)*neuron_n2,NYSCL*NXSCL*Ni);
    cout << "adds: " << NYSCL*NXSCL*Ni*Ky*Kx <<  "\n";
    cout << "argc:" << argc << "\n";
//  cout << "mult-block:  " << calc.first   << " sigmoid-block: " << calc.second  << "\n";
//  cout << "mult-orig:  "  << calc2.first  << " sigmoid-orig:  " << calc2.second << "\n";
//
//  int n_outputs= Ny/Sy * Nx/Sx * Nn;
//  cout << "mult-correct: " << n_outputs*Ni*Kx*Ky
//       << " sigmoid-correct: "  << n_outputs << "\n";
  }
  end_roi();
}

