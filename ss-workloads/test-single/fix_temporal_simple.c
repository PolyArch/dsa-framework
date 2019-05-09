#include "testing.h"
#include "temporal.dfg.h"

uint64_t input[64];
uint64_t output[128];
uint64_t answer[128];

int main(int argc, char* argv[]) {
  init();

  for(int i = 0; i < 64; ++i) {
    input[i] = i;
    answer[i] = i+1;
    answer[i+64] = i+16;
  }

  SS_CONFIG(temporal_config,temporal_size);

  begin_roi();
  SS_DMA_READ(&input[0],8,8,  64, P_temporal_DI);
  SS_DMA_READ(&input[0],8,8,  64, P_temporal_TI);
  SS_DMA_WRITE(P_temporal_DO,8,8, 64,&output[0]);
  SS_DMA_WRITE(P_temporal_TO,8,8, 64,&output[64]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],answer,output,128);
}
