#include "testing.h"

void test_new() {
  init();
  SS_CONFIG(none_config,none_size);
  SS_DMA_RD_INNER(in, ASIZE * sizeof(DTYPE), P_none_in);
  SS_DMA_WR_INNER(out, ASIZE * sizeof(DTYPE), P_none_out);
  SS_WAIT_ALL();
}

void test_old() {
  SS_CONFIG(none_config,none_size);
  SS_DMA_READ(&in[0],ABYTES,ABYTES,1,P_none_in);
  // SS_DMA_READ(&in[0],8,8,ABYTES,P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,AWORDS,&out[0]);
  SS_WAIT_ALL();
}

int main(int argc, char* argv[]) {

  test_new();
  compare<DTYPE>(argv[0], out, in, ASIZE);
  test_old();
  compare<DTYPE>(argv[0], out, in, ASIZE);

  return 0;
}
