#include <complex>
#include <stdint.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "ss-config/fixed_point.h"
#include "ss_insts.h"

#include "compute.dfg.h"

using std::complex;

#define VEC 4

complex<float> _zero(0, 0);

void filter(int n, int m, complex<float> *a, complex<float> *b, complex<float> *c, complex<float> *) {
  SS_CONFIG(compute_config, compute_size);
  int len = n - m + 1;
  int pad = (VEC - (len & (VEC - 1))) & (VEC - 1);
  int mid = m >> 1;
  SS_DMA_SCRATCH_LOAD(a, 8, 8, n, 0);
  SS_WAIT_SCR_WR();
  //SS_REPEAT_PORT((len + pad) / VEC);
  //SS_DMA_READ(b, 8, 8, mid, P_compute_B);
  SS_CONST(P_compute_C, 0, len + pad);
  SS_RECURRENCE(P_compute_O, P_compute_C, mid * (len + pad));
  for (int i = 0; i < mid; ++i) {
    SS_CONST(P_compute_B, *((uint64_t*)(b + i)), (len + pad) / VEC);
    SS_SCRATCH_READ(i * 8, 8 * len, P_compute_AL);
    SS_CONST(P_compute_AL, 0, pad);
    SS_SCRATCH_READ((m - 1 - i) * 8, 8 * len, P_compute_AR);
    SS_CONST(P_compute_AR, 0, pad);
  }
  /*for (int j = 0; j < mid; ++j) {
    for (int i = 0; i < len; ++i) {
      complex<float> delta0(complex_mul(a[i + j], b[j]));
      complex<float> delta1(complex_mul(a[i + (m - j - 1)], b[j]));
      complex<float> delta(complex_add(delta0, delta1));
      c[i] = complex<float>(complex_add(c[i], delta));
    }
  }*/
  SS_SCRATCH_READ(mid * 8, 8 * len, P_compute_AL);
  //SS_DMA_READ(a + mid, 0, 8 * len, 1, P_compute_AL);
  SS_CONST(P_compute_AL, 0, pad);
  SS_CONST(P_compute_AR, 0, len + pad);
  SS_CONST(P_compute_B, *((uint64_t*) b + mid), (len + pad) / VEC);
  SS_DMA_WRITE(P_compute_O, 8, 8, len + pad, c);
  SS_WAIT_ALL();
  /*for (int i = 0; i < len; ++i) {
    complex<float> delta(complex_mul(a[i + mid + 1], b[mid + 1]));
    c[i] = complex<float>(complex_add(c[i], delta));
  }*/
}
