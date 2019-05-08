#include <assert.h>
#include <complex>
#include <iostream>
#include <stdint.h>
#include <cmath>
#include <algorithm>
#include "compute.dfg.h"
#include "ss_insts.h"

using std::complex;

complex<float> one(1, 0);

void gemm(int n, int m, int p, complex<float> *a, complex<float> *b, complex<float> *c) {
  SS_CONFIG(compute_config, compute_size);

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      SS_DMA_READ(c + i * p, 0, 8 * p, 1, P_compute_C);
      SS_CONST(P_compute_A, *((uint64_t*) (a + i * m + j)), p / 4);
      SS_SCR_PORT_STREAM(8 * m, 0, 8 * p, 1, P_compute_B);
      SS_DMA_WRITE(P_compute_O, 8, 8, p, c + i * p);
      SS_WAIT_ALL();
    }
  }
  //SS_WAIT_ALL();

}
