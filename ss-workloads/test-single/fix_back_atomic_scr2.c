#include <stdio.h>
#include "add_offset.dfg.h"
#include <inttypes.h>
#include <stdlib.h>
#include "testing.h"

#define N 256

uint64_t output[N];

int main() {
  init();

  uint64_t ind[N/2];
  for(uint64_t i=0; i<N/2; ++i) {
    ind[i] = i*2;
  }
  uint64_t x[N];
  for(uint64_t i=0; i<N; ++i) {
    x[i] = i;
  }

  SS_CONFIG(add_offset_config,add_offset_size);
  
  //SS_CONST_SCR(0, 0, (64*2+2));
  //SS_WAIT_SCR_WR();
 
  begin_roi();
  for(int i = 0; i < 1; ++i) {
    // begin_roi();

    //SS_DMA_READ(&ind[0], 8, 8, N/2, P_IND_1);
    //SS_CONFIG_INDIRECT1(T64, T64, 1, 1);
    //SS_INDIRECT(P_IND_1, &x[0], N/2, P_add_offset_A);
    SS_DMA_READ(&x[0], 8, 8, N, P_add_offset_A);

    SS_CONST(P_add_offset_const, 1, N/2);

    SS_ATOMIC_SCR_OP(P_add_offset_C, P_add_offset_D, 0, N, 0);
    //SS_WAIT_SCR_WR();
    //SS_DMA_WRITE(P_add_offset_C,8,8,N,&output[0]);
    //SS_DMA_WRITE(P_add_offset_D,8,8,N,&output[0]);

    //SS_WAIT_SCR_ATOMIC();

    // end_roi();

  }

    SS_WAIT_ALL();

    end_roi();
    sb_stats();


  //SS_WAIT_ALL();
}
