#include "qr.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "matvec.h"
#include "rdfg.dfg.h"
#include "qdfg.dfg.h"

#define N _N_

complex<float> _one(1, 0);
uint64_t _one_in_bit = *((uint64_t*) &_one);

void qr(complex<float> *a, complex<float> *q, complex<float> *tau) {
  SS_CONFIG(rdfg_config, rdfg_size);

  const int r_trans_spad = 0;
  int w_spad = 8192;

  SS_FILL_MODE(NO_FILL);
  for (int i = 0; i < N - 1; ++i) {
    int n = N - i;
    int n2 = n / 2 + n % 2;
    int n12 = (n - 1) / 2 + (n - 1) % 2;

    {
      int pad = (n - 1) & 1;
      SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_rdfg_M); SS_CONST(P_rdfg_M, 0, pad);
      SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_rdfg_V); SS_CONST(P_rdfg_V, 0, pad);
      SS_CONST(P_rdfg_Coef, _one_in_bit, n12);
      SS_CONST(P_rdfg_reset, 2, n12 - 1);
      SS_CONST(P_rdfg_reset, 1, 1);
    }

    SS_RECURRENCE(P_rdfg_O, P_rdfg_NORM, 1);
    SS_SCRATCH_READ(r_trans_spad, 8, P_rdfg_HEAD);

    SS_DMA_WRITE(P_rdfg_ALPHA, 8, 8, 1, a + i * N + i); //alpha


    uint64_t tau0;
    SS_RECV(P_rdfg_U1INV, tau0);
    SS_CONST(P_rdfg_M, tau0, (n - 1) * 2);

    SS_DMA_WRITE(P_rdfg_TAU0, 8, 8, 1, tau + i);

    SS_CONST(P_rdfg_Coef, _one_in_bit, n - 1);
    SS_CONST(P_rdfg_reset, 1, n - 1);
    for (int j = 0; j < n - 1; ++j) {
      SS_SCR_PORT_STREAM(r_trans_spad + 8 * j, 8, 8, 1, P_rdfg_V);
      SS_CONST(P_rdfg_V, 0, 1);
    }

    SS_REPEAT_PORT((n - 1) * n2);
    SS_RECURRENCE(P_rdfg_TAU1, P_rdfg_Coef, 1); //tau
    //SS_GARBAGE(P_rdfg_TAU1, 1); // for debugging
    
    SS_CONST_SCR(w_spad, _one_in_bit, 1);
    SS_SCR_WRITE(P_rdfg_O, 8 * (n - 1), w_spad + 8);

    SS_WAIT_SCR_WR();
    //SS_WAIT_ALL();

    SS_REPEAT_PORT(n2);
    SS_RECURRENCE(P_rdfg_O, P_rdfg_A, n - 1);
    SS_2D_CONST(P_rdfg_Signal, 1, 1, 0, n2 - 1, n - 1);
    SS_DMA_WRITE(P_rdfg_FIN, 8 * N, 8, n - 1, a + (i + 1) * N + i);
    {
      int pad = n & 1;
      SS_2D_CONST(P_rdfg_reset, 2, n2 - 1, 1, 1, n - 1);
      for (int j = 0; j < n - 1; ++j) {
        SS_SCR_PORT_STREAM(w_spad, 0, 8 * n, 1, P_rdfg_M); SS_CONST(P_rdfg_M, 0, pad);
        SS_SCR_PORT_STREAM(r_trans_spad + j * 8 * n, 0, 8 * n, 1, P_rdfg_V); SS_CONST(P_rdfg_V, 0, pad);
        SS_SCR_PORT_STREAM(w_spad, 0, 8 * n, 1, P_rdfg_B); SS_CONST(P_rdfg_B, 0, pad);
        SS_SCR_PORT_STREAM(r_trans_spad + j * 8 * n, 0, 8 * n, 1, P_rdfg_C); SS_CONST(P_rdfg_C, 0, pad);
        SS_SCR_WRITE(P_rdfg_R, 8 * (n - 1), r_trans_spad); SS_GARBAGE(P_rdfg_R, pad);
      }
    }

    SS_WAIT_SCR_WR();

    w_spad -= (n - 1) * 8;
  }
  SS_WAIT_ALL();

  w_spad += 8;
  SS_CONFIG(qdfg_config, qdfg_size);
  for (int i = N - 2; i >= 0; --i) {
    int n = N - i;
    int n2 = n / 2 + n % 2;
    int n12 = (n - 1) / 2 + (n - 1) % 2;
    SS_SCR_PORT_STREAM(w_spad, 8, 8, n, P_qdfg_D);
    SS_CONST(P_qdfg_E, *((uint64_t*)(tau + i)), n);
    SS_DMA_WRITE(P_qdfg_FIN, 0, 8 * n, 1, q + i * N + i);

    SS_CONST(P_qdfg_Coef, *((uint64_t*)(tau + i)), n12 * (n - 1));
    SS_2D_CONST(P_qdfg_reset, 2, n12 - 1, 1, 1, n - 1);
    SS_REPEAT_PORT(n2);
    SS_RECURRENCE(P_qdfg_O, P_qdfg_A, n - 1);
    {
      int pad1 = (n - 1) & 1;
      int pad0 = n & 1;
      for (int j = 0; j < n - 1; ++j) {
        SS_SCR_PORT_STREAM(w_spad + 8, 0, 8 * (n - 1), 1, P_qdfg_M); SS_CONST(P_qdfg_M, 0, pad1);
        SS_DMA_READ(q + (i + 1 + j) * N + i + 1, 8 * N, 8 * (n - 1), 1, P_qdfg_V); SS_CONST(P_qdfg_V, 0, pad1);
        SS_SCR_PORT_STREAM(w_spad, 0, 8 * n, 1, P_qdfg_B); SS_CONST(P_qdfg_B, 0, pad0);
        SS_DMA_READ(q + (j + i + 1) * N + i, 8 * N, 8 * n, 1, P_qdfg_C); SS_CONST(P_qdfg_C, 0, pad0);
        SS_DMA_WRITE(P_qdfg_Q, 8 * N, 8 * n, 1, q + (i + 1) * N + i); SS_GARBAGE(P_qdfg_Q, pad0);
      }
    }

    w_spad += i * 8;
    SS_WAIT_ALL();
  }
  SS_WAIT_ALL();
}

#undef N

