#include "testing.h"

#define MAX_REPEAT 4

DTYPE answer[ASIZE*MAX_REPEAT];
DTYPE out_big[ASIZE*MAX_REPEAT];


int main(int argc, char* argv[]) {
  init();

  uint64_t* answer64 = (uint64_t*) &answer[0];
  uint64_t* out64    = (uint64_t*) &out_big[0];
  uint64_t* in64     = (uint64_t*) &in[0];

  for(int repeat=1; repeat <= MAX_REPEAT; ++repeat) {
    for(int i=0; i < AWORDS; ++i) {
      for(int j=0; j < repeat; ++j) {
        answer64[i*repeat+j]=in64[i];
      }
    }
  
    begin_roi();
    SS_CONFIG(none_config,none_size);
    SS_REPEAT_PORT(repeat);
    SS_DMA_READ(&in[0],ABYTES,ABYTES,1,P_none_in);
    SS_DMA_WRITE(P_none_out,8*repeat,8*repeat,AWORDS,&out_big[0]);
    SS_WAIT_ALL();
    end_roi();
 
    compare<DTYPE>(argv[0],out_big,answer,ASIZE*repeat);
  }
}
