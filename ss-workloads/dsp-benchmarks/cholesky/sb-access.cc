#include <iostream>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <complex>
#include "cholesky.h"
#include "compute.dfg.h"
#include "writeback.dfg.h"
#include "ss_insts.h"
#include "matvec.h"

using std::complex;

void cholesky(complex<float> *a, complex<float> *L) {

  int N = _N_;
  complex<float> div = std::sqrt(*a);
  {
    int i = 0;
    complex<float> *b = a + i * (N + 1);
    L[i * (N + 1)] = div;
    {
      complex<float> *bp = b + 1;
      float norm = 1 / complex_norm(div);
      div = std::conj(div * norm);
      SS_CONFIG(writeback_config, writeback_size);
      SS_DMA_READ(bp, 8, 8, N - i - 1, P_writeback_BP);
      SS_CONST(P_writeback_DIV, *((uint64_t *) &div), N - i - 1);
      SS_DMA_WRITE(P_writeback_RES, N * 8, 8, N - i - 1, L + (i + 1) * N + i);
      /*for (int j = i + 1; j < N; ++j) {
        //L[j * N + i] = b[j - i] / div;
        L[j * N + i] = complex<float>(
            (bp->real * div.real + bp->imag * div.imag) * norm,
            (bp->imag * div.real - bp->real * div.imag) * norm
        );
        ++bp;
      }*/
    }
    {
      complex<float> *bj = b + 1, *bk, v = a[i * (N + 1)];
      float norm = 1 / complex_norm(v);
      union {
        float f[2];
        uint64_t v;
      } ri_v = {v.real() * norm, v.imag() * -norm};
      SS_WAIT_ALL();
      SS_CONFIG(compute_config, compute_size);

      int total = N - i - 1;
      SS_DMA_READ_STRETCH(a + (i + 1) * N + (i + 1), 8 * (N + 1), 8 * total, -8, total, P_compute_Z);
      SS_DMA_READ_STRETCH(bj, 8, 8 * total, -8, total, P_compute_B);
      SS_CONST(P_compute_V, ri_v.v, (1 + total) * total / 2);
      SS_CONFIG_PORT(total, -1);
      SS_DMA_READ(bj, 8, 8, total, P_compute_A);
      SS_DMA_WRITE(P_compute_O, 0, 8, 1, &div);
      SS_SCR_WRITE(P_compute_O, (1 + total) * (total) * 4 - 8, 0);

      SS_WAIT_ALL();
    }
  }

  for (int i = 1; i < N; ++i) {
    int total = N - i - 1;

    complex<float> val = 1.0f / div;
    div = std::sqrt(div);

    complex<float> *b = a + i * (N + 1);
    L[i * (N + 1)] = div;
    {
      complex<float> *bp = b + 1;
      float norm = 1 / complex_norm(div);
      div = std::conj(div * norm);
      SS_CONFIG(writeback_config, writeback_size);
      SS_SCRATCH_READ(0, 8 * total, P_writeback_BP);
      SS_CONST(P_writeback_DIV, *((uint64_t *) &div), total);
      SS_DMA_WRITE(P_writeback_RES, N * 8, 8, total, L + (i + 1) * N + i);
      /*for (int j = i + 1; j < N; ++j) {
        //L[j * N + i] = b[j - i] / div;
        L[j * N + i] = complex<float>(
            (bp->real * div.real + bp->imag * div.imag) * norm,
            (bp->imag * div.real - bp->real * div.imag) * norm
        );
        ++bp;
      }*/
    }
    if (total) {
      complex<float> *bj = b + 1, *bk, v = a[i * (N + 1)];
      float norm = 1 / complex_norm(v);
      union {
        float f[2];
        uint64_t v;
      } ri_v = {v.real() * norm, v.imag() * -norm};
      SS_WAIT_ALL();

      SS_CONFIG(compute_config, compute_size);

      SS_SCRATCH_READ(8 * total, 4 * total * (total + 1), P_compute_Z);
      SS_SCR_PORT_STREAM_STRETCH(0, 8, 8 * total, -8, total, P_compute_B);
      SS_CONST(P_compute_V, *((uint64_t*) &val), (1 + total) * total / 2);
      SS_CONFIG_PORT(total, -1);
      SS_SCR_PORT_STREAM(0, 8, 8, total, P_compute_A);
      SS_DMA_WRITE(P_compute_O, 0, 8, 1, &div);
      SS_SCR_WRITE(P_compute_O, 4 * total * (total + 1) - 8, 0);

      SS_WAIT_ALL();
    }
  }
}

