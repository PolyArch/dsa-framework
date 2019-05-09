#include <complex>
#include <stdint.h>
#include <cmath>
#include <algorithm>
#include "ss-config/fixed_point.h"
#include "compute.dfg.h"
#include "ss_insts.h"

using std::complex;

#define PI 3.14159265358979303

void gemm(int n, int m, int p, complex<float> *a, complex<float> *b, complex<float> *c) {
  SS_CONFIG(compute_config, compute_size);
  SS_DMA_SCRATCH_LOAD(b, 0, 8 * m * p, 1, 0);
  SS_WAIT_SCR_WR();
  for (int i = 0; i < n; ++i) {
    SS_CONST(P_compute_C, 0, p);
    SS_RECURRENCE(P_compute_O, P_compute_C, p * (m - 1));
    SS_SCR_PORT_STREAM(0    , 8 * p, 8 * p, m, P_compute_B);
    SS_DMA_WRITE(P_compute_O, 0, 8 * p, 1, c + i * p);
    SS_REPEAT_PORT(p / 4);
    SS_DMA_READ(a + i * m, 0, 8 * m, 1, P_compute_A);
    /*for (int k = 0; k < m; k += 2) {
      SS_DMA_READ(a + i * m + k, 0, 8, p / 4, P_compute_A);
    }*/
  }
  SS_WAIT_ALL();
}
