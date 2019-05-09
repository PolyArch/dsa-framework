#include "testing.h"
#include "add1_vec.dfg.h"

#define N 5
#define M 1
#define TOTAL ((N) * (M))

uint64_t input[TOTAL];
uint64_t output[TOTAL];
uint64_t answer[TOTAL];


int main(int argc, char* argv[]) {
  init();
  for (int i = 0; i < TOTAL; ++i) {
    input[i] = i;
    answer[i] = i + 1;
  }

  SS_FILL_MODE(STRIDE_DISCARD_FILL);

  begin_roi();
  SS_CONFIG(add1_vec_config,add1_vec_size);

  SS_DMA_READ(input,           M * 8, M * 8, N, P_add1_vec_in);
  SS_DMA_WRITE(P_add1_vec_out, M * 8, M * 8, N, output);


  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0], answer, output, TOTAL);
}
