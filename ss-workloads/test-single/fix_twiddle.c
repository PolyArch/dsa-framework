#include "testing.h"
#include "twiddle.dfg.h"


uint64_t out64[4];
uint64_t answer64[4];

int main(int argc, char* argv[]) {
  init();

  SS_CONFIG(twiddle_config,twiddle_size);

  SS_CONST(P_twiddle_in,0x0003000200010000,1);
  SS_DMA_WRITE(P_twiddle_out_normal,8,8,1,&out64[0]);
  SS_DMA_WRITE(P_twiddle_out_reverse,8,8,1,&out64[1]);
  SS_DMA_WRITE(P_twiddle_out_red_add1,8,8,1,&out64[2]);
  SS_DMA_WRITE(P_twiddle_out_upper,8,8,1,&out64[3]);
  SS_WAIT_ALL();

  answer64[0]=0x0003000200010000;
  answer64[1]=0x0000000100020003;
  answer64[2]=0x0000000000000007;
  answer64[3]=0x0000000000030002;

  compare<uint64_t>(argv[0],out64,answer64,4);
}
