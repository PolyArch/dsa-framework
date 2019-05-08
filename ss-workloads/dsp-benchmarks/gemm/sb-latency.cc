#include <complex>
#include <stdint.h>
#include <cmath>
#include <algorithm>
#include "compute.dfg.h"
#include "ss_insts.h"

using std::complex;

complex<float> one(1, 0);

void gemm(int n, int m, int p, complex<float> *a, complex<float> *b, complex<float> *c) {
  SS_CONTEXT(255);
  SS_CONFIG(compute_config, compute_size);

  int resudo = n & 7;
  int _n = n + (resudo != 0) * (8 - resudo);
  for (int io = 0; io < _n; io += 8) {
    SS_CONTEXT_I(255);
    SS_CONST(P_compute_C, 0, p);
    SS_RECURRENCE(P_compute_O, P_compute_C, p * (m - 1));
    SS_SCR_PORT_STREAM(0    , 0, 8 * m * p, 1, P_compute_B);

    SS_STRIDE(8, 8);

    complex<float> *_c = c + io * p;
    complex<float> *_a = a + io * m;

    SS_CONTEXT_OFFSET(255, p);
    SS_DMA_WRITE_SIMP(P_compute_O, p, _c);
    SS_CONTEXT_OFFSET(255, m)
    SS_REPEAT_PORT(p / 4);
    SS_DMA_READ_SIMP(_a, m, P_compute_A);
    SS_CONTEXT_OFFSET(255, 0);
  }

  /*if (resudo) {

    SS_CONTEXT_I(15);
    SS_CONST(P_compute_C, 0, p / 2);
    SS_RECURRENCE(P_compute_O, P_compute_C, (p / 2) * (m / 2 - 1));
    SS_SCR_PORT_STREAM(0    , 8 * p, 4 * p, m / 2, P_compute_BE);
    SS_SCR_PORT_STREAM(p * 4, 8 * p, 4 * p, m / 2, P_compute_BO);

    SS_STRIDE(8, 8);

    complex<float> *_c = c + io * p;
    complex<float> *_a = a + io * m;

    SS_CONTEXT_OFFSET(15, p);
    SS_DMA_WRITE_SIMP(P_compute_O, p / 2, _c);
    SS_CONTEXT_OFFSET(15, m)
    SS_REPEAT_PORT(p / 4);
    SS_DMA_READ_SIMP(_a, m / 2, P_compute_A);
    SS_CONTEXT_OFFSET(15, 0);

  }*/

  SS_CONTEXT(255);
  SS_WAIT_ALL();
}
