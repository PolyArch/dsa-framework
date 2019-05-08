#include <stdio.h>
#include "red.dfg.h"
#include "../common/include/ss_insts.h"
#include "../common/include/sim_timing.h"
#include <inttypes.h>

#define ALEN 65536

uint16_t array[ALEN]={};

uint16_t accum16[4]={0,0,0,0};

int main(int argc, char* argv[]) {

  uint16_t sum0=0; 
  uint64_t sum1=0;

  printf("\nPOINTERS: array:%p accum16:%p sum0:%p\n", array, accum16, &sum0);


  for(int i = 0; i < ALEN; ++i) {
    array[i]=i&1;
  }


  //begin_roi();
  //Version 0: (no softbrain)
  for(int i = 0; i < ALEN; ++i) {
    sum0 += array[i];
  }
  //end_roi(); 



  //Version 3: (uses accumulator)
  SS_CONFIG(red_config,red_size);

  SS_WAIT_ALL();


  for(int i = 0; i < ALEN; ++i) {
    sum1 += array[i];
  }



  begin_roi();

  SS_CONST(P_red_reset,0, ALEN/32-1);
  //SS_CONST(P_red_A,1, ALEN/4);
  //SS_DMA_READ(array, 8, 8, (argc&0x100)*ALEN*sizeof(uint16_t)/8, P_red_A);
  SS_DMA_READ(array, 8, 8, ALEN*sizeof(uint16_t)/8, P_red_A);

  SS_CONST(P_red_reset,1, 1);
  SS_GARBAGE(P_red_O, ALEN/32-1);
  SS_DMA_WRITE(P_red_O,8,8,1,&accum16);

  SS_WAIT_ALL();


  end_roi();

  sb_stats();

  sum1 = accum16[0] + accum16[1] + accum16[2] + accum16[3];

  printf("dot product (original): %d\n",sum0);
  printf("dot product (sb version 1): %d\n",sum1);

}
