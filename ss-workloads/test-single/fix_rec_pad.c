#include <cassert>
#include "testing.h"
#include <stdio.h>
#include "none.dfg.h"
#include "add1_vec_o2.dfg.h"
#include <inttypes.h>
#include <stdlib.h>

uint64_t a[ASIZE];
uint64_t aa[ASIZE];
uint64_t resA[ASIZE];
uint64_t resB[ASIZE];

void test_zero_fill() {
  for (int i = 0; i < ASIZE; ++i)
    a[i] = i;

  SS_FILL_MODE(NO_FILL);
  SS_CONFIG(add1_vec_o2_config, add1_vec_o2_size);
  SS_DMA_READ(a, 32, 32, ASIZE / 4, P_add1_vec_o2_in);

  SS_GARBAGE(P_add1_vec_o2_outB, ASIZE / 2);

  SS_FILL_MODE(STRIDE_ZERO_FILL);
  SS_SET_ITER(2);
  SS_RECURRENCE_PAD(P_add1_vec_o2_outA, P_add1_vec_o2_in, ASIZE / 2);
  SS_DMA_WRITE(P_add1_vec_o2_outA, 8, 8, ASIZE / 2, resA);
  SS_DMA_WRITE(P_add1_vec_o2_outB, 8, 8, ASIZE / 2, resB);
  SS_WAIT_ALL();

  for (int i = 0; i < ASIZE / 4; i += 2) {
    assert(resA[i] == (i + 1) * 2);
    assert(resA[i + 1] == (i + 1) * 2 + 1);
  }
  for (int i = 0; i < ASIZE / 4; ++i) {
    assert(resB[i] == 1);
  }
}

void test_discard_fill() {
  for (int i = 0; i < ASIZE; ++i)
    a[i] = i;

  SS_FILL_MODE(NO_FILL);
  SS_CONFIG(add1_vec_o2_config, add1_vec_o2_size);
  SS_DMA_READ(a, 32, 32, ASIZE / 4, P_add1_vec_o2_in);

  SS_GARBAGE(P_add1_vec_o2_outB, ASIZE / 2);

  SS_FILL_MODE(STRIDE_DISCARD_FILL);
  SS_SET_ITER(2);
  SS_RECURRENCE_PAD(P_add1_vec_o2_outA, P_add1_vec_o2_in, ASIZE / 2);
  SS_DMA_WRITE(P_add1_vec_o2_outA, 8, 8, ASIZE / 2, resA);
  //SS_DMA_WRITE(P_add1_vec_o2_outB, 8, 8, ASIZE / 2, resB);
  SS_WAIT_ALL();

  for (int i = 0; i < ASIZE / 4; i += 2) {
    assert(resA[i] == (i + 1) * 2);
    assert(resA[i + 1] == (i + 1) * 2 + 1);
  }
}

int main(int argc, char* argv[]) {
  test_discard_fill();
  puts("========================================");
  test_zero_fill();
  return 0;
}
