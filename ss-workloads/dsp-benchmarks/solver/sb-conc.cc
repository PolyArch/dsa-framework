#include <complex>
#include "multi.dfg.h"
#include "ss_insts.h"

using std::complex;

#define N _N_

void solver(complex<float> *a, complex<float> *v) {
  SS_CONFIG(multi_config, multi_size);

  SS_DMA_READ(a, 8 * (N + 1), 8, N, P_multi_X);
  SS_CONST(P_multi_Y, *((uint64_t*) v), 1);

  SS_REPEAT_PORT((N - 2) / 4 + 1);
  SS_CONFIG_PORT_EXPLICIT((N - 1) * 2, -2);
  SS_RECURRENCE(P_multi_Y_X_REC, P_multi_V, N - 1);
  SS_GARBAGE(P_multi_Y_X_REC, 1);
  SS_DMA_WRITE(P_multi_Y_X_OUT, 8, 8, N, v);

  /*SS_FILL_MODE(STRIDE_ZERO_FILL);
  SS_DMA_READ_STRETCH(a + 1, 8 * (N + 1), 8 * (N - 1), -8, N - 1, P_multi_A);
  //SS_DMA_READ(v, 0, 8 * (N - 1), 1, P_multi_VV);
  SS_FILL_MODE(NO_FILL);*/

  int last_pad;
  {
    int pad = (4 - (N - 1) % 4) % 4;
    SS_DMA_READ(a + 1, 0, 8 * (N - 1), 1, P_multi_A); SS_CONST(P_multi_A, 0, pad);
    SS_DMA_READ(v, 0, 8 * (N - 1), 1, P_multi_VV); SS_CONST(P_multi_VV, 0, pad);
    last_pad = pad;
  }

  for (int i = 1; i < N; ++i) {
    int pad = (last_pad + 1) % 4;

    {
      int pad = (4 - (N - i - 1) % 4) % 4;
      SS_DMA_READ(a + i * N + i + 1, 0, 8 * (N - i - 1), 1, P_multi_A); SS_CONST(P_multi_A, 0, pad);
    }

    //SS_REPEAT_PORT((N - i - 2) / 4 + 1);
    //if (i != N - 1) {
    //  SS_RECURRENCE(P_multi_Y_X_REC, P_multi_V, 1);
    //} else {
    //  SS_GARBAGE(P_multi_Y_X_REC, 1);
    //}

    SS_RECURRENCE(P_multi_O, P_multi_Y, 1);
    SS_RECURRENCE(P_multi_O, P_multi_VV, (N - i - 1));
    SS_CONST(P_multi_VV, 0, pad);
    SS_GARBAGE(P_multi_O, last_pad);

    last_pad = pad;
  }
  //SS_GARBAGE(P_multi_Y_X_REC, 1);
  SS_WAIT_ALL();
}

#undef N
