#include "testing.h"
#include <stdio.h>
#include "merge.dfg.h"
#include <inttypes.h>
#include <stdlib.h>

#define N 12

int main(int argc, char* argv[]) {
  uint64_t x[N+1] = {0, 1, 4, 7,   12, 16, 22, 23,  25, 16, 34, 42, SENTINAL};
  uint64_t y[N+1] = {3, 5, 8, 11,  13, 14, 15, 17,  18, 19, 20, 21, SENTINAL};
  uint64_t z_answer[N*2];
  uint64_t z_output[N*2];

  SS_CONFIG(merge_config,merge_size);

  int xp=0, yp=0;
  for(uint64_t i=0; i<2*N; ++i) {
    if(x[xp] < y[yp]) {
      z_answer[i] = x[xp++];
    } else {
      z_answer[i] = y[yp++];
    }
  }

  begin_roi(); 
  SS_DMA_READ(&x[0], 8, 8, N+1, P_merge_in1);
  SS_DMA_READ(&y[0], 8, 8, N+1, P_merge_in2);

  SS_DMA_WRITE(P_merge_out, 8, 8, N*2, &z_output[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],z_answer,z_output,2*N);
}
