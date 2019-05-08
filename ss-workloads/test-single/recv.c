#include "testing.h"

int main(int argc, char* argv[]) {
  init();

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_DMA_READ(&in[0],8,8,AWORDS,P_none_in);

  uint64_t* out64 = (uint64_t*) out;

  for(int i = 0; i < AWORDS; ++i) {
    uint64_t val;
    SS_RECV(P_none_out,val);
    out64[i] = val;
  }
  
  SS_WAIT_ALL();
  end_roi();

  compare<DTYPE>(argv[0],out,in,1);
}
