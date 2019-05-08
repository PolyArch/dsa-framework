#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  SS_CONTEXT_OFFSET(0x0003,ABYTES/2);
  SS_CONFIG(none_config,none_size);

  begin_roi();
  SS_DMA_READ(&in[0],ABYTES/2,ABYTES/2,1,P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,AWORDS/2,&out[0]);
  SS_WAIT_ALL();
  end_roi();
 
  compare<DTYPE>(argv[0],out, in,ASIZE);
}
