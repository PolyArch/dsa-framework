#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  //don't do one iteration
  int N_BYTES = ABYTES - sizeof(uint64_t);
  int N_WORDS = AWORDS - 1;

  begin_roi();
  if(N_BYTES > 0) {
    SS_CONFIG(none_config,none_size);
    SS_DMA_READ(&in[2],N_BYTES,N_BYTES,1,P_none_in);
    SS_DMA_WRITE(P_none_out,8,8,N_WORDS,&out[2]);
    SS_WAIT_ALL();
  }

  out[0]=in[0];
  out[1]=in[1];
  out[ASIZE-2]=in[ASIZE-2];
  out[ASIZE-1]=in[ASIZE-1];

  end_roi();

  compare<DTYPE>(argv[0],out,in,ASIZE);
}
