#include "testing.h"

int main(int argc, char* argv[]) {
  init();
  
  int len1=3;
  int len2=4;
  int const1=7;
  int const2=3;
  int iters=10;

  int volume = iters*(len1+len2);
  uint64_t pattern[volume];
  uint64_t pattern_check[volume];

  int ind =0;
  for(int i = 0; i < iters; ++i) {
    for(int e = 0; e < len1; ++e,++ind) {
      pattern_check[ind]=const1;
    }
    for(int e = 0; e < len2; ++e,++ind) {
      pattern_check[ind]=const2;
    }
  }

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_2D_CONST(P_none_in,const1,len1,const2,len2,iters);
  SS_DMA_WRITE(P_none_out,8,8,volume,&pattern[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],pattern,pattern_check,volume);
}
