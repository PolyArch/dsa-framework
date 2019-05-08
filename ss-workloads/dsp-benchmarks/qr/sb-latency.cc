#include "qr.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "matvec.h"
#include "l_rdfg.dfg.h"
#include "l_qdfg.dfg.h"

#define N _N_

complex<float> _one(1, 0);
complex<float> ws[1024];
uint64_t _one_in_bit = *((uint64_t*) &_one);

void qr(complex<float> *a, complex<float> *q, complex<float> *tau) {
  SS_CONTEXT((1 << 0) | (1 << 2) | (1 << 4) | (1 << 6));
  SS_CONFIG(l_rdfg_config, l_rdfg_size);
  SS_CONTEXT((1 << 1) | (1 << 3) | (1 << 5) | (1 << 7));
  SS_CONFIG(l_qdfg_config, l_qdfg_size);

  const int r_trans_spad = 0;
  complex<float> *w = ws;
  int w_spad = 8192;

  int acc = 0;
  {
    int n = N;
    SS_CONTEXT(1 << acc);
  }

  for (int i = 0; i < N - 1; ++i) {
    int n = N - i;
    int n2 = n / 2 + n % 2;
    int n12 = (n - 1) / 2 + (n - 1) % 2;

    SS_FILL_MODE(STRIDE_ZERO_FILL);
    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_l_rdfg_M);
    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_l_rdfg_V);
    SS_CONST(P_l_rdfg_Coef, _one_in_bit, n12);
    SS_CONST(P_l_rdfg_reset, 2, n12 - 1);
    SS_CONST(P_l_rdfg_reset, 1, 1);

    SS_RECURRENCE(P_l_rdfg_O, P_l_rdfg_NORM, 1);
    SS_SCRATCH_READ(r_trans_spad, 8, P_l_rdfg_HEAD);

    SS_DMA_WRITE(P_l_rdfg_ALPHA, 8, 8, 1, a + i * N + i); //alpha


    SS_REPEAT_PORT(n - 1);
    SS_RECURRENCE(P_l_rdfg_U1INV, P_l_rdfg_M, 1); //normalize
    SS_DMA_WRITE(P_l_rdfg_TAU0, 8, 8, 1, tau + i);
    SS_CONST(P_l_rdfg_Coef, _one_in_bit, n - 1);
    SS_SCR_PORT_STREAM(r_trans_spad + 8, 8, 8, n - 1, P_l_rdfg_V);
    SS_CONST(P_l_rdfg_reset, 1, n - 1);

    SS_REPEAT_PORT((n - 1) * n2);
    SS_RECURRENCE(P_l_rdfg_TAU1, P_l_rdfg_Coef, 1); //tau
    //SS_GARBAGE(P_l_rdfg_TAU1, 1); // for debugging
    
    //SS_CONST_SCR(w_spad, _one_in_bit, 1);
    //SS_SCR_WRITE(P_l_rdfg_O, 8 * (n - 1), w_spad + 8);
    w[0] = 1.0;
    SS_DMA_WRITE(P_l_rdfg_O, 0, 8 * (n - 1), 1, w);

    SS_WAIT_MEM_WR();

    //SS_SCR_PORT_STREAM(w_spad, 0, 8 * n, n - 1, P_l_rdfg_M);
    SS_DMA_READ(w, 0, 8 * n, n - 1, P_l_rdfg_M);
    if (i) {
      SS_SCR_PORT_STREAM(r_trans_spad, 8 * n, 8 * n, 1, P_l_rdfg_V);
      SS_STRIDE(8 * n, 8 * n);
      SS_RECURRENCE_PAD(P_l_rdfg_Out0, P_l_rdfg_V, n * (n - 2));
      SS_GARBAGE(P_l_rdfg_Out0, n);
      SS_FILL_MODE(STRIDE_ZERO_FILL);
    } else {
      SS_SCR_PORT_STREAM(r_trans_spad, 8 * n, 8 * n, n - 1, P_l_rdfg_V);
    }
    SS_2D_CONST(P_l_rdfg_reset, 2, n2 - 1, 1, 1, n - 1);


    SS_REPEAT_PORT(n2);
    SS_RECURRENCE(P_l_rdfg_O, P_l_rdfg_A, n - 1);

    SS_FILL_MODE(STRIDE_DISCARD_FILL);
    SS_2D_CONST(P_l_rdfg_Signal, 1, 1, 0, n2 - 1, n - 1);
    //SS_SCR_PORT_STREAM(w_spad, 0, 8 * n, n - 1, P_l_rdfg_B);
    SS_DMA_READ(w, 0, 8 * n, n - 1, P_l_rdfg_B);
    //if (i) {
    //  SS_SCR_PORT_STREAM(r_trans_spad, 8 * n, 8 * n, 1, P_l_rdfg_C);
    //  SS_STRIDE(8 * n, 8 * n);
    //  SS_RECURRENCE(P_l_rdfg_Out1, P_l_rdfg_C, n * (n - 2));
    //  //SS_GARBAGE(P_l_rdfg_Out1, n);
    //} else {
      SS_SCR_PORT_STREAM(r_trans_spad, 8 * n, 8 * n, n - 1, P_l_rdfg_C);
    //}

    SS_DMA_WRITE(P_l_rdfg_FIN, 8, 8, n - 1, a + i * N + i + 1);
    //SS_GARBAGE(P_l_rdfg_R, (n - 1) * (n % 2));

    if (n - 1 == 1) {
      SS_DMA_WRITE(P_l_rdfg_R, 0, 8, 1, a + N * N - 1);
      SS_CONTEXT(1 << (acc + 1));
        //if (i == 1) {
        SS_FILL_MODE(STRIDE_ZERO_FILL);
        SS_STRIDE(8 * n, 8 * n);
        SS_RECURRENCE_PAD(P_l_qdfg__Out0, P_l_qdfg_V, n * N);
        SS_DMA_READ(w, 0, 8 * n, N, P_l_qdfg_M);
        SS_CONST(P_l_qdfg_Coef, *((uint64_t*)(tau + i)), ((n - 1) / 2 + 1) * N);
        SS_2D_CONST(P_l_qdfg_reset, 2, (n - 1) / 2, 1, 1, N);
        SS_REPEAT_PORT((n - 1) / 2 + 1);
        SS_RECURRENCE(P_l_qdfg_O, P_l_qdfg_A, N);
        SS_FILL_MODE(STRIDE_DISCARD_FILL);
        SS_STRIDE(8 * n, 8 * n);
        SS_RECURRENCE_PAD(P_l_qdfg__Out1, P_l_qdfg_C, n * N);
        SS_STRIDE(8 * n, 8 * n);
        SS_DMA_READ(w, 0, 8 * n, N, P_l_qdfg_B);
        SS_CONST(P_l_qdfg_Signal, 1, N);
        SS_DMA_WRITE(P_l_qdfg_Q, 8 * N, 16, N, q + i);
        //SS_XFER_RIGHT(P_l_qdfg_Q, P_l_rdfg__In, (n - 1) * N);
        //SS_GARBAGE(P_l_qdfg_Q, (n - 1) * N);
        //}
    } else {
      SS_STRIDE(8 * (n - 1), 8 * (n - 1));
      SS_XFER_RIGHT_PAD(P_l_rdfg_R, P_l_qdfg_In, (n - 1) * (n - 1));
      //SS_SCR_WRITE(P_l_rdfg_R, 8 * (n - 1) * (n - 1), r_trans_spad);

      ++acc;
      SS_CONTEXT(1 << acc);
      SS_STRIDE(8 * (n - 1), 8 * (n - 1));
      SS_XFER_RIGHT(P_l_qdfg_Out, P_l_rdfg_In, (n - 1) * (n - 1));

      if (!i) {
        SS_FILL_MODE(NO_FILL);
        SS_2D_CONST(P_l_qdfg_V, _one_in_bit, 1, 0, N, N - 1);
        SS_CONST(P_l_qdfg_V, _one_in_bit, 1);
        SS_DMA_READ(w, 0, 8 * n, N, P_l_qdfg_M);
        SS_2D_CONST(P_l_qdfg_reset, 2, n / 2 - 1, 1, 1, N);
        SS_CONST(P_l_qdfg_Coef, *((uint64_t*)(tau + i)), n / 2 * N);
        SS_REPEAT_PORT(n / 2);
        SS_RECURRENCE(P_l_qdfg_O, P_l_qdfg_A, n);
        SS_DMA_READ(w, 0, 8 * n, N, P_l_qdfg_B);
        SS_2D_CONST(P_l_qdfg_C, _one_in_bit, 1, 0, N, N - 1);
        SS_CONST(P_l_qdfg_C, _one_in_bit, 1);
        SS_2D_CONST(P_l_qdfg_Signal, 0, 1, 1, n / 2 - 1, N);
        SS_DMA_WRITE(P_l_qdfg_FIN, 8 * N, 8, N, q + i);
        //SS_GARBAGE(P_l_qdfg_Q, (n - 1) * N);
        SS_XFER_RIGHT(P_l_qdfg_Q, P_l_rdfg__In, (n - 1) * N);
        SS_FILL_MODE(STRIDE_DISCARD_FILL);
      } else {
        //if (i == 1) {
        SS_FILL_MODE(STRIDE_ZERO_FILL);
        SS_STRIDE(8 * n, 8 * n);
        SS_RECURRENCE_PAD(P_l_qdfg__Out0, P_l_qdfg_V, n * N);
        SS_DMA_READ(w, 0, 8 * n, N, P_l_qdfg_M);
        SS_CONST(P_l_qdfg_Coef, *((uint64_t*)(tau + i)), ((n - 1) / 2 + 1) * N);
        SS_2D_CONST(P_l_qdfg_reset, 2, (n - 1) / 2, 1, 1, N);
        SS_REPEAT_PORT((n - 1) / 2 + 1);
        SS_RECURRENCE(P_l_qdfg_O, P_l_qdfg_A, N);
        SS_FILL_MODE(STRIDE_DISCARD_FILL);
        SS_STRIDE(8 * n, 8 * n);
        SS_RECURRENCE_PAD(P_l_qdfg__Out1, P_l_qdfg_C, n * N);
        SS_STRIDE(8 * n, 8 * n);
        SS_DMA_READ(w, 0, 8 * n, N, P_l_qdfg_B);
        SS_2D_CONST(P_l_qdfg_Signal, 0, 1, 1, (n - 1) / 2, N);
        SS_DMA_WRITE(P_l_qdfg_FIN, 8 * N, 8, N, q + i);
        SS_XFER_RIGHT(P_l_qdfg_Q, P_l_rdfg__In, (n - 1) * N);
        //SS_GARBAGE(P_l_qdfg_Q, (n - 1) * N);
        //}
      }

      acc = (acc + 1) % 8;
      SS_CONTEXT(1 << acc);
      SS_SCR_WRITE(P_l_rdfg_Out0, 8 * (n - 1), r_trans_spad);
      SS_GARBAGE(P_l_rdfg_Out1, n - 1);
      SS_WAIT_SCR_WR();
      //if (!i) {
      SS_XFER_RIGHT(P_l_rdfg__Out, P_l_qdfg__In, (n - 1) * N);
      //}

      SS_SCR_WRITE(P_l_rdfg_Out1, 8 * (n - 2) * (n - 1), r_trans_spad + 8 * (n - 1));
      //SS_GARBAGE(P_l_rdfg_Out1, (n - 2) * (n - 1));
      //SS_GARBAGE(P_l_rdfg_Out0, (n - 2) * (n - 1));

    }

    w_spad -= (n - 1) * 8;
    w += (n - 1);
  }
  SS_CONTEXT(255);
  SS_WAIT_ALL();

}

#undef N

