#include <stdio.h>
#include "dot.h"
#include "../common/include/ss_insts.h"
#include "../common/include/sim_timing.h"
#include <inttypes.h>

#define ALEN 2048

uint16_t array1[ALEN]={1};
uint16_t array2[ALEN]={1};

uint64_t out = 0;

int main(int argc, char* argv[]) {
  for(int i = 0; i < ALEN; ++i) {
    array1[i]=1;
    array2[i]=1;
  }

  uint16_t sum0=0; 
  uint64_t sum1=0, sum2=0;

  //Version 0: (no softbrain)
  for(int i = 0; i < ALEN; ++i) {
    sum0 += array1[i] * array2[i];
  }


  //Version 1:
  begin_roi();
  SS_CONFIG(dot_config,dot_size);

  SS_CONST(P_dot_carry,0,1);

  SS_DMA_READ(array1, 8, 8, ALEN*sizeof(uint16_t)/8, P_dot_A);
  SS_DMA_READ(array2, 8, 8, ALEN*sizeof(uint16_t)/8, P_dot_B);

  SS_RECURRENCE(P_dot_R,P_dot_carry,ALEN/16-1);
  SS_DMA_WRITE(P_dot_R,8,8,1,&sum1);

  SS_WAIT_ALL();
  end_roi();

  //Version 2:  (has scratchpad)
  int scr_addr=0;

  SS_CONST(P_dot_carry,0,1);

  SS_DMA_SCRATCH_LOAD(array1,8,8,ALEN*sizeof(uint16_t)/8,scr_addr);
  SS_WAIT_SCR_WR();

  SS_SCRATCH_READ(scr_addr, ALEN*sizeof(uint16_t), P_dot_A);
  SS_DMA_READ(array2, 4*sizeof(uint16_t), 4*sizeof(uint16_t), ALEN/4, P_dot_B);

  SS_RECURRENCE(P_dot_R,P_dot_carry,ALEN/16-1);
  SS_DMA_WRITE(P_dot_R,0,8,1,&sum2);

  SS_WAIT_ALL();

  printf("dot product (original): %d\n",sum0);
  printf("dot product (sb version 1): %d\n",sum1);
  printf("dot product (sb version 2): %d\n",sum2);

  sb_stats();
}
