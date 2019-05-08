#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_DMA_SCRATCH_LOAD(in,ABYTES,ABYTES,1,0);
  SS_WAIT_SCR_WR();
  //SS_WAIT_ALL();
  SS_SCRATCH_DMA_STORE(0,8,8,AWORDS,&out[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<DTYPE>(argv[0],out,in,ASIZE);
}
