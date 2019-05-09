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

  for (int i = 0; i < _N_; ++i) {
    complex<float> div = std::sqrt(a[i * _N_ + i]);

    complex<float> *b = a + i * (_N_ + 1);
    L[i * (_N_ + 1)] = div;
    {
      complex<float> *bp = b + 1;
      float norm = 1 / complex_norm(div);
      div = std::conj(div * norm);
      SS_CONFIG(writeback_config, writeback_size);
      SS_DMA_READ(bp, 8, 8, _N_ - i - 1, P_writeback_BP);
      SS_CONST(P_writeback_DIV, *((uint64_t *) &div), _N_ - i - 1);
      SS_DMA_WRITE(P_writeback_RES, _N_ * 8, 8, _N_ - i - 1, L + (i + 1) * _N_ + i);
      /*for (int j = i + 1; j < _N_; ++j) {
        //L[j * _N_ + i] = b[j - i] / div;
        L[j * _N_ + i] = complex<float>(
            (bp->real * div.real + bp->imag * div.imag) * norm,
            (bp->imag * div.real - bp->real * div.imag) * norm
        );
        ++bp;
      }*/
    }
    {
      complex<float> *bj = b + 1, *bk, v = a[i * (_N_ + 1)];
      float norm = 1 / complex_norm(v);
      union {
        float f[2];
        uint64_t v;
      } ri_v = {v.real() * norm, v.imag() * -norm};
      SS_WAIT_ALL();
      SS_CONFIG(compute_config, compute_size);


      for (int j = i + 1; j < _N_; ++j) {
        int times = _N_ - j;
        uint64_t ri_bj = *((uint64_t *) bj);

        SS_CONST(P_compute_A, ri_bj, times);
        SS_DMA_READ(a + j * (_N_ + 1), 0, 8 * times, 1, P_compute_Z);
        SS_DMA_READ(bj, 0, 8 * times, 1, P_compute_B);
        SS_CONST(P_compute_V, ri_v.v, times);
        SS_DMA_WRITE(P_compute_O, 0, 8 * times, 1, a + j * (_N_ + 1));

        ++bj;
      }
      SS_WAIT_ALL();
    }
  }
}

