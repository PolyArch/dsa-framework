#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  SS_CONFIG(none_config, none_size); // to avoid seg faults when no dfg
  begin_roi();
  SS_DMA_SCRATCH_LOAD(in,0,8,1,0);
  SS_WAIT_ALL();
  end_roi();
}
