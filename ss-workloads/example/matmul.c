#include <stdio.h>
#include "dot_red.h"
#include "../common/include/ss_insts.h"
#include "../common/include/sim_timing.h"
#include <inttypes.h>

#define ALEN 64

uint16_t array[ALEN][ALEN];
uint16_t vec[ALEN];

uint16_t out1[ALEN];
uint16_t out2[ALEN];

int main(int argc, char* argv[]) {

  //Version 1: Fill in Data
r for(int i = 0; i < ALEN; ++i) {
    for(int j = 0; j < ALEN; ++j) {
      array[i][j] = (i+j)%16;
    }
    vec[i]=i;
  }


  //Version 1: Original
  for(int i = 0; i < ALEN; ++i) {
    uint16_t acc=0;
    for(int j = 0; j < ALEN; ++j) {
      acc+=array[i][j]*vec[j]; 
    }
    out1[i]=acc;
  }

  //Version 3: (uses accumulator)
  SS_CONFIG(dot_red_config,dot_red_size); 
  SS_WAIT_ALL();

  begin_roi();

  SS_2D_CONST(P_dot_red_reset, 2,ALEN/16-1, 1,1, ALEN);
  SS_DMA_READ(array, 8, 8,                      ALEN*ALEN*sizeof(uint16_t)/8, P_dot_red_A);
  SS_DMA_READ(vec,   0, ALEN*sizeof(uint16_t),  ALEN,                         P_dot_red_B);
  SS_DMA_WRITE_SHF16(P_dot_red_R,8,8,ALEN/4,&out2);
  SS_WAIT_ALL();

  end_roi();

  sb_stats();

  for(int i = 0; i < ALEN; ++i) {
    printf("%d: %d %d\n",i,out1[i],out2[i]);
  }
}
