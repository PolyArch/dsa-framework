#include "testing.h"
#include "multi.dfg.h"

uint64_t input[64];
uint64_t output[64];
uint64_t answer[64];


int main(int argc, char* argv[]) {
  init();

  for(int i = 0; i < 64; ++i) {
    input[i] = i;
    answer[i] = i+3;
  }

  begin_roi();
  SS_CONFIG(multi_config,multi_size);
  SS_DMA_READ(&input[0],8,8,  64, P_multi_inA);
  SS_RECURRENCE(P_multi_outA,P_multi_inB,64);

  //SS_DMA_READ(&input[0],8,8,  64, P_multi_inB);

  SS_DMA_WRITE(P_multi_outB,8,8, 64,&output[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],answer,output,64);
}
