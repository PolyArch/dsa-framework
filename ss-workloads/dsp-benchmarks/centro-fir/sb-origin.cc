#include <complex>
#include <stdint.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "ss-config/fixed_point.h"
#include "ss_insts.h"
#include "matvec.h"

#include "compute.dfg.h"

using std::complex;

#define VEC 4

complex<float> _zero(0, 0);

void filter(int n, int m, complex<float> *a, complex<float> *b, complex<float> *c, complex<float> *) {

  SS_CONFIG(compute_config, compute_size);

  int half = m / 2;
  const int block = 128;
  for (int io = 0; io < n - m + 1; io += block) {
    int len = std::min(n - m + 1 - io, block);
    int pad = get_pad(len, VEC);
    SS_CONST(P_compute_C, 0, len + pad);
    SS_RECURRENCE(P_compute_O, P_compute_C, half * (len + pad));
    for (int j = 0; j < half; ++j) {
      SS_CONST(P_compute_B, *((uint64_t*)(b + j)), (len + pad) / VEC);
      SS_SCRATCH_READ((j + io) * 8, 8 * (len + pad), P_compute_AL);
      SS_SCRATCH_READ((io + m - 1 - j) * 8, 8 * (len + pad), P_compute_AR);
    }
    SS_SCRATCH_READ((half + io) * 8, 8 * len, P_compute_AL);
    SS_CONST(P_compute_AL, 0, pad);
    SS_CONST(P_compute_AR, 0, len + pad);
    SS_CONST(P_compute_B, *((uint64_t*) b + half), (len + pad) / VEC);
    SS_DMA_WRITE(P_compute_O, 0, 8 * (len + pad), 1, c + io);
    //for (int ii = 0; ii < len; ++ii) {
    //  int i = io + ii;
    //  for (int j = 0, jj = m - 1; j < m / 2; ++j, --jj) {
    //    c[i] += (a[i + j] + a[i + jj]) * b[j];
    //  }
    //  c[i] += a[i + m / 2] * b[m / 2];
    //}
  }
  SS_WAIT_ALL();
}
