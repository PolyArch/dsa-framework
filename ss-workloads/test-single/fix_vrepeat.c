#include "testing.h"
#include "add1.dfg.h"

int main(int argc, char* argv[]) {
  init();
 
  // uint64_t repeat[AWORDS];
  uint8_t repeat[AWORDS];
  uint64_t data_array[AWORDS];
  uint64_t out[AWORDS];
  uint64_t known[AWORDS];

  for(int i=0; i<AWORDS; ++i) {
    repeat[i]=1;
    data_array[i]=i*2;
    out[i]=i*2; known[i]=i*2;
  }

  SS_CONFIG(none_config,none_size);

  begin_roi();
  SS_DMA_READ(&repeat[0],1,1,AWORDS,P_IND_1);
  SS_VREPEAT_PORT(P_IND_1);
  SS_DMA_READ(&data_array[0],8,8,AWORDS,P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,AWORDS,&out[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],out,known,AWORDS);
  
}
