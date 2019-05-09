#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_WAIT_ALL();
  SS_SCRATCH_READ(0, ABYTES, P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,AWORDS,&out[0]);
  SS_WAIT_ALL();
  end_roi();
}
