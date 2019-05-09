#include <complex>
#include "limited.dfg.h"
#include "ss_insts.h"

using std::complex;

#define N _N_

void solver(complex<float> *a, complex<float> *v) {
  SS_CONFIG(limited_config, limited_size);

  complex<float> tmp[N * 2];

  for (int i = 0; i < N; ++i) {
    SS_CONST(P_limited_X, *(uint64_t*)(v + i), 1);
    SS_CONST(P_limited_Y, *(uint64_t*)(a + i * N + i), 1);
    SS_RECV(P_limited_RES0, v[i]);
    SS_GARBAGE(P_limited_RES1, 1);
    int pad = 4 - (N - i - 1) % 4;
    SS_DMA_READ(a + i * N + i + 1, 0, 8 * (N - i - 1), 1, P_limited_A); SS_CONST(P_limited_A, 0, pad);
    SS_DMA_READ(v + i + 1, 0, 8 * (N - i - 1), 1, P_limited_VV); SS_CONST(P_limited_VV, 0, pad);
    SS_CONST(P_limited_V, *(uint64_t*)(v + i), (N - i - 1 + pad) / 4);
    SS_DMA_WRITE(P_limited_O, 0, 8 * (N - i - 1), 1, v + i + 1);
    SS_GARBAGE(P_limited_O, pad);
    SS_WAIT_ALL();
  }
  //SS_GARBAGE(P_multi_Y_X_REC, 1);
  SS_WAIT_ALL();
}

#undef N
