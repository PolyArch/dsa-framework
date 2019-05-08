#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_DMA_READ(&in[0],ABYTES,ABYTES,1,P_none_in);
  SS_SCR_WRITE(P_none_out,ABYTES,0);
  SS_WAIT_SCR_WR();
  //SS_WAIT_ALL();
  SS_SCRATCH_READ(0, ABYTES, P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,AWORDS,&out[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<DTYPE>(argv[0],out,in,ASIZE);
}
