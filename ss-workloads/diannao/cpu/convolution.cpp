#include <iostream>
#include <string>
#include "dnn.hpp"

using namespace std;

#ifndef SHARED
#define SHARED 1
#endif

//Lets just make sure all problem specific stuff gets defined before we get here!
//#ifndef Ny
//  //Problem Size
//  #define Ny 32
//  #define Ny 32
//  
//  #define Kx 4
//  #define Ky 4
//  //#define Ni 108
//  //#define Nn 200
//  
//  #define Ni 112
//  #define Nn 224
//#endif
//

//slide increment
#ifndef Sy
  #define Sy 1
  #define Sx 1
#endif

#ifndef Tnn
  //Tiling Sizes
  #define Tnn 32
  //#define Tn  25
  //#define Ti  16
  #define Tn  16
  #define Ti  16
  
  #define Ty  8
  #define Tx  8
#endif

#define NYPAD (Ny+Ky)
#define NXPAD (Nx+Kx)

#define NYSCL (Ny/Sy)
#define NXSCL (Nx/Sx)


//Arrays:
#if SHARED == 1
#define SYNAPSE_SIZE (1L*Ky*Kx*Nn*Ni)
#else
#define SYNAPSE_SIZE (1L*NYSCL*NXSCL*Ky*Kx*Nn*Ni)
#endif

#if SHARED == 1
VTYPE (*synapse)[Ky][Kx][Nn][Ni];
#else
VTYPE (*synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni];
#endif

//VTYPE neuron_i[NYPAD][NXPAD][Ni];
//VTYPE neuron_n[NYSCL][NXSCL][Nn]={0},    neuron_n2[NYSCL][NXSCL][Nn]={0};

VTYPE  (*neuron_i)[NYPAD][NXPAD][Ni];
VTYPE  (*neuron_n)[NYSCL][NXSCL][Nn];
VTYPE (*neuron_n2)[NYSCL][NXSCL][Nn];


void fill_convolution_shared_simple(VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
                                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  for(int yy = 0; yy < Ky; ++yy) {
    for(int xx = 0; xx < Kx; ++xx) {
      for(int nn = 0; nn < Nn; ++nn) {
        for(int ni = 0; ni < Ni; ++ni) {
          synapse[yy][xx][nn][ni] = 2;
  } } } }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = 1;
  }  }  }

}

void fill_convolution_private(VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
                                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  for(int yout = 0; yout < NYSCL; ++yout) {
    for(int xout = 0; xout < NXSCL; ++xout) {
      for(int yy = 0; yy < Ky; ++yy) {
        for(int xx = 0; xx < Kx; ++xx) {
          for(int nn = 0; nn < Nn; ++nn) {
            for(int ni = 0; ni < Ni; ++ni) {
              synapse[xout][yout][yy][xx][nn][ni] = 2;
  } } } } } }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = 1;
  }  }  }

}


void fill_convolution_shared(VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
                             VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  int total1=0,total2=0;
  for(int yy = 0; yy < Ky; ++yy) {
    for(int xx = 0; xx < Kx; ++xx) {
      for(int nn = 0; nn < Nn; ++nn) {
        for(int ni = 0; ni < Ni; ++ni) {
          synapse[yy][xx][nn][ni] = total1;
          total1+=1;
        }
      }
    }
  }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = total2;
        total2+=2;
      }
    }
  }
}

std::pair<int,int> convolution_layer_blocked(
#if SHARED == 1
                              VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                              VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                              VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                              VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Ny; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        // — Original code — (excluding nn, ii loops)
        int yout = yy/Sy;//0;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx; //0;
          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;

            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {
              for (int n = nn; n < nn + Tn; n++) {
                sum[n] = 0;
              }
              // sliding window;
              for (int ky = 0; ky < Ky; ky++) {
                for (int kx = 0; kx < Kx; kx++) {

                  int ii = 0;
                  VTYPE sum_sc;

                  for (; ii < Ni -Ti+1; ii += Ti) { //don't do past the end
                    //c1++;
                    for (int n = nn; n < nn + Tn; n++) {
                      sum_sc=0;
                      for (int i = ii; i < ii + Ti; i++) {
                        #if SHARED == 1 // version with shared kernels
                        VTYPE sv = synapse[ky][kx][n][i];
                        VTYPE nv = neuron_i[ky + y][kx + x][i];
                        #else // version with private kernels
                        VTYPE sv = synapse[yout][xout][ky][kx][n][i];
                        VTYPE nv = neuron_i[ky + y][kx + x][i];
                        #endif
                        sum_sc+=(sv*nv)>>1;
                      }
                      sum[n]+=sum_sc;
                    }
                  }
                }
              }
              //sigmoid
              for (int n = nn; n < nn + Tn; n++) {
                neuron_n[yout][xout][n] = sigmoid(sum[n]);
                //c2++;
              }
            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  return make_pair(c1,c2);
}

std::pair<int,int>  convolution_layer(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  // — Original code — (excluding nn, ii loops)
  int yout = 0;
  for (int y = 0; y < Ny; y += Sy) { // tiling for y;
    int xout = 0;
    for (int x = 0; x < Ny; x += Sx) { // tiling for x;
      for (int nn = 0; nn < Nn; nn += Tn) {
        for (int n = nn; n < nn + Tn; n++) {
          sum[n]=0;
        }

        // sliding window;
        for (int ky = 0; ky < Ky; ky++)
          for (int kx = 0; kx < Kx; kx++)
            for (int n = nn; n < nn + Tn; n++)
              for (int i = 0; i < Ni; i++) {
                #if SHARED == 1 // version with shared kernels
                VTYPE sv = synapse[ky][kx][n][i];
                VTYPE nv = neuron_i[ky + y][kx + x][i];
                #else // version with private kernels
                VTYPE sv = synapse[yout][xout][ky][kx][n][i];
                VTYPE nv = neuron_i[ky + y][kx + x][i];
                #endif
                sum[n]+=(sv*nv)>>1;
              }
        //sigmoid
        for (int n = nn; n < nn + Tn; n++) {
          neuron_n[yout][xout][n] = sigmoid(sum[n]);
          c2++;
        }
      }
      xout++; 
    }
    yout++;
  }
  return make_pair(c1,c2);
}

int main(const int argc, const char** argv) {

  #if SHARED == 1
  synapse = (VTYPE (*)[Ky][Kx][Nn][Ni]) malloc(SYNAPSE_SIZE*sizeof(VTYPE));
  #else
  synapse = (VTYPE (*)[NYSCL][NXSCL][Ky][Kx][Nn][Ni]) malloc(SYNAPSE_SIZE*sizeof(VTYPE));
  #endif

  neuron_i  = (VTYPE (*)[NYPAD][NXPAD][Ni])malloc(NYPAD*NXPAD*Ni*sizeof(VTYPE));
  neuron_n  = (VTYPE (*)[NYSCL][NXSCL][Nn])malloc(NYSCL*NXSCL*Nn*sizeof(VTYPE));
  neuron_n2 = (VTYPE (*)[NYSCL][NXSCL][Nn])malloc(NYSCL*NXSCL*Nn*sizeof(VTYPE));

  #if SHARED == 1
  fill_convolution_shared_simple(*synapse,*neuron_i);
  #else
  fill_convolution_private(*synapse,*neuron_i);
  #endif

  begin_roi();
  if(argc==3) {

//  } else if(argc==2 && string(argv[1])=="perf") {
  } else if(argc==2) {
    auto calc  = convolution_layer_blocked(*synapse,*neuron_i,*neuron_n);
    //cout << "Perf Run Complete\n";
  } else {
  cout << "argc: " << argc << "\n";

    auto calc  = convolution_layer_blocked(*synapse,*neuron_i,*neuron_n);
    auto calc2 = convolution_layer(*synapse,*neuron_i,*neuron_n2);
    if(calc.first!=0) {
      cout << "blocks=" << calc.first << "\n";
    }
    compare((VTYPE*)*neuron_n,(VTYPE*)*neuron_n2,NYSCL*NXSCL*Nn);
    int n_outputs= Ny/Sy * Ny/Sx * Nn;
    cout << "mults: " << n_outputs*Ni*Kx*Ky << " sigmoids: "  << n_outputs << "\n";
    cout << "argc: " << argc << "\n";
  }
  end_roi();
 
  //cout << "mult-block:  " << calc.first   << " sigmoid-block: " << calc.second  << "\n";
  //cout << "mult-orig:  "  << calc2.first  << " sigmoid-orig:  " << calc2.second << "\n";
}

