#include "add.dfg.h"
#include "testing.h"

uint16_t answer[ASIZE];

int main(int argc, char* argv[]) {
  init();

  for(int i=0; i < ASIZE; ++i) {
    answer[i]=in[i]*2;
  }

  begin_roi();
  SS_CONFIG(add_config,add_size);
  SS_ADD_PORT(P_add_in2);
  SS_DMA_READ(&in[0],ABYTES,ABYTES,1,P_add_in1);
  SS_DMA_WRITE(P_add_out,8,8,AWORDS,&out[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<DTYPE>(argv[0],out,answer,ASIZE);
}
