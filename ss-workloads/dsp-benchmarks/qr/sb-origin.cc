#include "qr.h"
#include <cmath>
#include <iostream>
#include <algorithm>
//#include "matvec.h"
#include "ss_insts.h"
#include "finalize.dfg.h"
#include "nmlz.dfg.h"
#include "norm.dfg.h"
#include "vcmm.dfg.h"

complex<float> buffer[1024];
complex<float> ws[1024];

int get_pad(int n, int v) {
  return (v - n % v) % v;
}

float complex_norm(const complex<float> &v) {
  return v.real() * v.real() + v.imag() * v.imag();
}

void qr(complex<float> *a, complex<float> *q, complex<float> *tau) {
  int N = _N_;
  complex<float> *w = buffer;
  complex<float> *v = buffer + N;

  for (int i = 0; i < N - 1; ++i) {
    int n = N - i;
    complex<float> normx(0);
    {
      SS_CONFIG(norm_config, norm_size);
      int pad = get_pad(n, 6);
      SS_DMA_READ(a + i * N + i, 8 * N, 8, n, P_norm_A); SS_CONST(P_norm_A, 0, pad);
      SS_CONST(P_norm_reset, 0, (n + pad) / 6 - 1);
      SS_CONST(P_norm_reset, 1, 1);
      SS_GARBAGE(P_norm_O, (n + pad) / 6 - 1);
      SS_DMA_WRITE(P_norm_O, 8, 8, 1, &normx);
      SS_WAIT_ALL();
      normx = sqrt(normx.real() + normx.imag());
    }
    //float normx = 0;
    //for (int j = i; j < N; ++j) {
    //  w[j - i] = a[j * N + i];
    //  normx += complex_norm(w[j - i]);
    //}
    //normx = sqrt(normx);
    w[0] = a[i * N + i];
    float norm0 = 1. / sqrt(complex_norm(w[0]));
    complex<float> s = -w[0] * norm0;
    a[i * N + i] = s * normx;
    complex<float> u1 = 1.0f / (w[0] - s * normx);
    w[0] = 1.0f;
    {
      SS_CONFIG(nmlz_config, nmlz_size);
      int pad = get_pad(n - 1, 4);
      SS_DMA_READ(a + (i + 1) * N + i, 8 * N, 8, n - 1, P_nmlz_A); SS_CONST(P_nmlz_A, 0, pad);
      SS_CONST(P_nmlz_B, *((uint64_t*) &u1), n - 1 + pad);
      //SS_DMA_WRITE(P_nmlz_AB, 8 * N, 8, n - 1, a + (i + 1) * N + i);
      SS_DMA_WRITE(P_nmlz_AB, 8, 8, n - 1, ws + i * N);
      SS_DMA_WRITE(P_nmlz_AB_, 8, 8, n - 1, w + 1);
      SS_GARBAGE(P_nmlz_AB, pad);
      SS_GARBAGE(P_nmlz_AB_, pad);
      SS_WAIT_ALL();
    }
    //for (int j = i + 1; j < N; ++j) {
    //  w[j - i] *= u1;
    //  a[j * N + i] = w[j - i];
    //}
    tau[i] = -std::conj(s) / u1 / normx;
    //householder done

    //SBvec_mul_mat(a + i * N + i + 1, n, n - 1, N, w, true, v);
    do {
      auto mat = a + i * N + i + 1;
      auto vec = w;
      auto res = v;
      auto mat_stride = N;
      auto mat_height = n;
      auto mat_width  = n - 1;
      int pad(get_pad((mat_width), 4));
      int A(P_vcmm_A);
      int B(P_vcmm_B);
      int C(P_vcmm_C);
      int O(P_vcmm_O);
      SS_CONFIG(vcmm_config, vcmm_size);
      SS_CONST(C, 0, (mat_width) + pad);
      for (int _i_(0); _i_ < (0) + (mat_height); ++_i_) {
        SS_DMA_READ((mat) + _i_ * (mat_stride), 8, 8, (mat_width), A);
        SS_CONST(A, 0, pad);
        SS_CONST(B, *((uint64_t*)(vec) + _i_), ((mat_width) + pad) / 4);
        if (_i_ != (mat_height) - 1) {
          SS_RECURRENCE(O, C, (mat_width) + pad);
        } else {
          SS_DMA_WRITE(O, 8, 8, (mat_width), (res));
          SS_GARBAGE(O, pad);
        }
      }
      SS_WAIT_ALL();
    } while (false);

    {
      int pad = (n - 1) & 1;
      SS_CONFIG(finalize_config, finalize_size);
      SS_CONST(P_finalize_TAU, *((uint64_t*)(tau + i)), n * (n - 1 + pad));
      for (int j = 0; j < n; ++j) {
        SS_DMA_READ(a + (j + i) * N + i + 1, 0, 8 * (n - 1), 1, P_finalize_C); SS_CONST(P_finalize_C, 0, pad);
        SS_CONST(P_finalize_B, *((uint64_t*)(w + j)), (n - 1 + pad) / 2);
        SS_DMA_READ(v, 0, 8 * (n - 1), 1, P_finalize_A); SS_CONST(P_finalize_A, 0, pad);
        SS_DMA_WRITE(P_finalize_O, 0, 8 * (n - 1), 1, a + (j + i) * N + i + 1); SS_GARBAGE(P_finalize_O, pad);
      }
      SS_WAIT_ALL();
    }
    //for (int j = i; j < N; ++j) {
    //  for (int k = i + 1; k < N; ++k) {
    //    a[j * N + k] -= tau[i] * w[j - i] * v[k - i - 1];
    //  }
    //}
  }

  q[N * N - 1] = 1.0f;
  for (int i = N - 2; i >= 0; --i) {
    int n = N - i;
    auto *w = ws + i * N;
    //for (int j = i + 1; j < N; ++j)
    // w[j - i - 1] = a[j * N + i];
    //SBvec_mul_mat(q + (i + 1) * N + i + 1, n - 1, n - 1, N, w, true, v);
    do {
      auto mat = q + i * N + i + 1;
      auto vec = w;
      auto res = v;
      auto mat_stride = N;
      auto mat_height = n - 1;
      auto mat_width  = n - 1;
      int pad(get_pad((mat_width), 4));
      int A(P_vcmm_A);
      int B(P_vcmm_B);
      int C(P_vcmm_C);
      int O(P_vcmm_O);
      SS_CONFIG(vcmm_config, vcmm_size);
      SS_CONST(C, 0, (mat_width) + pad);
      for (int _i_(0); _i_ < (0) + (mat_height); ++_i_) {
        SS_DMA_READ((mat) + _i_ * (mat_stride), 8, 8, (mat_width), A);
        SS_CONST(A, 0, pad);
        SS_CONST(B, *((uint64_t*)(vec) + _i_), ((mat_width) + pad) / 4);
        if (_i_ != (mat_height) - 1) {
          SS_RECURRENCE(O, C, (mat_width) + pad);
        } else {
          SS_DMA_WRITE(O, 8, 8, (mat_width), (res));
          SS_GARBAGE(O, pad);
        }
      }
      SS_WAIT_ALL();
    } while (false);

    complex<float> neg(-tau[i]);
    {
      SS_CONFIG(nmlz_config, nmlz_size);
      //nmlz1:
      int pad = get_pad(n - 1, 4);
      SS_DMA_READ(v, 0, 8 * (n - 1), 1, P_nmlz_A); SS_CONST(P_nmlz_A, 0, pad);
      SS_CONST(P_nmlz_B, *((uint64_t*)&neg), n - 1 + pad);
      SS_DMA_WRITE(P_nmlz_AB, 8, 8, n - 1, q + i * N + i + 1); SS_GARBAGE(P_nmlz_AB, pad);
      SS_GARBAGE(P_nmlz_AB_, n - 1 + pad);

      //nmlz2:
      SS_DMA_READ(a + (i + 1) * N + i, 8 * N, 8, n - 1, P_nmlz_A); SS_CONST(P_nmlz_A, 0, pad);
      SS_CONST(P_nmlz_B, *((uint64_t*)&neg), n - 1 + pad);
      SS_DMA_WRITE(P_nmlz_AB, 8 * N, 8, n - 1, q + (i + 1) * N + i); SS_GARBAGE(P_nmlz_AB, pad);
      SS_GARBAGE(P_nmlz_AB_, n - 1 + pad);

      SS_WAIT_ALL();
    }

    {
      int pad = (n - 1) & 1;
      SS_CONFIG(finalize_config, finalize_size);
      SS_CONST(P_finalize_TAU, *((uint64_t*)(tau + i)), (n - 1) * (n - 1 + pad));
      for (int j = 0; j < n - 1; ++j) {
        SS_DMA_READ(q + (j + i + 1) * N + i + 1, 0, 8 * (n - 1), 1, P_finalize_C); SS_CONST(P_finalize_C, 0, pad);
        SS_CONST(P_finalize_B, *((uint64_t*)(w + j)), (n - 1 + pad) / 2);
        SS_DMA_READ(v, 0, 8 * (n - 1), 1, P_finalize_A); SS_CONST(P_finalize_A, 0, pad);
        SS_DMA_WRITE(P_finalize_O, 0, 8 * (n - 1), 1, q + (j + i + 1) * N + i + 1); SS_GARBAGE(P_finalize_O, pad);
      }
      SS_WAIT_ALL();
    }

    q[i * N + i] = 1.0f - tau[i];
    //for (int j = i + 1; j < N; ++j) {
    //  // nmlz1: q[i * N + j] = -tau[i] * v[j - i - 1];
    //  // nmlz2: q[j * N + i] = -tau[i] * a[j * N + i];
    //  for (int k = i + 1; k < N; ++k) {
    //    q[j * N + k] -= tau[i] * w[j - i - 1] * v[k - i - 1];
    //  }
    //}
  }
}
