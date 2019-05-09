#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_DMA_READ(&in[0],8,8,1,P_none_in);
  SS_RECURRENCE(P_none_out,P_none_in,AWORDS);
  SS_DMA_WRITE(P_none_out,8,8,1,&out[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<DTYPE>(argv[0],out,in,1);
}
