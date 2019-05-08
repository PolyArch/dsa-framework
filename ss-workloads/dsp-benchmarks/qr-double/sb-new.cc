#include "qr.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "matvec.h"
#include "hhr.dfg.h"
#include "fused.dfg.h"

complex<float> _one(1, 0);
complex<float> buffer[1024];

void qr(complex<float> *a, complex<float> *q, complex<float> *tau) {
  int N = _N_;
  const int w_spad = 8;
  const int r_trans_spad = N * 8;
  const int q_spad = N * 8;

  SS_CONTEXT(2);
  SS_CONFIG(fused_config, fused_size);
  SS_DMA_SCRATCH_LOAD(&_one, 8, 8, 1, 0);

  SS_CONTEXT(1);
  SS_CONFIG(hhr_config, hhr_size);
  SS_DMA_SCRATCH_LOAD(&_one, 8, 8, 1, 0);
  SS_DMA_READ(a + N, 8 * N, 8, N - 1, P_hhr_A);
  SS_DMA_READ(a + N, 8 * N, 8, N - 1, P_hhr_B);
  SS_CONST(P_hhr_Coef, 1065353216, N - 1);
  SS_CONST(P_hhr_reset, 2, N - 2);
  SS_CONST(P_hhr_reset, 1, 1);
  //SS_REPEAT_PORT(4);
  SS_RECURRENCE(P_hhr_O, P_hhr_NORM, 1);
  //SS_REPEAT_PORT(4);
  SS_DMA_READ(a, 8, 8, 1, P_hhr_HEAD);
  //SS_CONST(P_hhr_Inst, 0, 1);
  //SS_CONST(P_hhr_Inst, 1, 1);
  //SS_CONST(P_hhr_Inst, 2, 1);
  //SS_CONST(P_hhr_Inst, 2, 1);

  SS_DMA_WRITE(P_hhr_ALPHA, 8, 8, 1, a); //alpha
  SS_REPEAT_PORT(N - 1);
  SS_RECURRENCE(P_hhr_U1INV, P_hhr_B, 1); //normalize
  SS_RECURRENCE(P_hhr_TAU0, P_hhr_IN, 1); //tau
  SS_DMA_WRITE(P_hhr_TAU1, 8, 8, 1, tau);
  SS_CONST(P_hhr_Coef, 1065353216, N - 1);
  SS_DMA_READ(a + N, 8 * N, 8, N - 1, P_hhr_A);
  SS_CONST(P_hhr_reset, 1, N - 1);

  //SS_DMA_WRITE(P_hhr_O, 8, 8, N - 1, buffer); W test pass!
  //SS_GARBAGE(P_hhr_OUTlocal, 1);              tau test pass!
  SS_REPEAT_PORT(N * (N - 1));
  SS_RECURRENCE(P_hhr_OUTlocal, P_hhr_Coef, 1);
  //SS_GARBAGE(P_hhr_OUTremote, 1); //xfer it!
  SS_REPEAT_PORT(N);
  SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_QTAU, 1);

  SS_RECURRENCE(P_hhr_O, P_hhr_IN, N - 1);

  SS_SCR_WRITE(P_hhr_OUTlocal, 8 * (N - 1), w_spad);
  //SS_GARBAGE(P_hhr_OUTremote, N - 1); //xfer it!
  SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_IN, N - 1);

  SS_CONTEXT(2);
  SS_SCR_WRITE(P_fused_OUT, 8 * (N - 1), w_spad);

  SS_CONTEXT(3);
  SS_WAIT_SCR_WR();

  SS_CONTEXT(2);
  
  SS_SCR_PORT_STREAM(0, 8, 8, N, P_fused_QW);
  SS_CONST(P_fused_QM, 1065353216, N);
  SS_CONST(P_fused_Qreset, 1, N);
  SS_REPEAT_PORT(N);
  SS_RECURRENCE(P_fused_QV, P_fused_QA, N);
  //SS_SCR_PORT_STREAM(0, 8, 8, N, P_fused_QA);
  SS_SCR_PORT_STREAM(0, 0, 8 * N, N, P_fused_QB);
  SS_2D_CONST(P_fused_QC, 1065353216, 1, 0, N, N - 1);
  SS_CONST(P_fused_QC, 1065353216, 1);
  SS_2D_CONST(P_fused_QSignal, 1, 1, 0, N - 1, N);
  SS_DMA_WRITE(P_fused_Q_MEM, 8 * N, 8, N, q);
  SS_SCR_WRITE(P_fused_Q_SPAD, 8 * N * (N - 1), q_spad);

  SS_CONTEXT(1);
  SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * N, N - 1, P_hhr_B);
  SS_2D_CONST(P_hhr_reset, 2, N - 1, 1, 1, N - 1);
  //SS_DMA_WRITE(P_hhr_O, 8, 8, N - 1, buffer + N); test pass!
  //SS_GARBAGE(P_hhr_O, N - 1); test pass!
  for (int i = 1; i < N; ++i) {
    SS_DMA_READ(a + i, 8 * N, 8, N, P_hhr_A);
  }

  SS_REPEAT_PORT(N);
  SS_RECURRENCE(P_hhr_O, P_hhr_RA, N - 1);
  SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * N, N - 1, P_hhr_RB);
  SS_2D_CONST(P_hhr_RSignal, 1, 1, 0, N - 1, N - 1);
  SS_DMA_WRITE(P_hhr_R_MEM, 8, 8, N - 1, a + 1);
  SS_SCR_WRITE(P_hhr_R_SPAD, 8 * (N - 1) * (N - 1), r_trans_spad);
  for (int i = 1; i < N; ++i) {
    SS_DMA_READ(a + i, 8 * N, 8, N, P_hhr_RC);
  }

  SS_WAIT_SCR_WR();
  for (int i = 1; i < N - 1; ++i) {
    int n = N - i;

    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_hhr_A);
    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_hhr_B);
    SS_CONST(P_hhr_Coef, 1065353216, n - 1);
    SS_CONST(P_hhr_reset, 2, n - 2);
    SS_CONST(P_hhr_reset, 1, 1);

    //SS_REPEAT_PORT(4);
    SS_RECURRENCE(P_hhr_O, P_hhr_NORM, 1);
    //SS_REPEAT_PORT(4);
    SS_SCRATCH_READ(r_trans_spad, 8, P_hhr_HEAD);

    //SS_CONST(P_hhr_Inst, 0, 1);
    //SS_CONST(P_hhr_Inst, 1, 1);
    //SS_CONST(P_hhr_Inst, 2, 1);
    //SS_CONST(P_hhr_Inst, 2, 1);

    SS_DMA_WRITE(P_hhr_ALPHA, 8, 8, 1, a + i * N + i); //alpha

    SS_REPEAT_PORT(n - 1);
    SS_RECURRENCE(P_hhr_U1INV, P_hhr_B, 1); //normalize
    SS_DMA_WRITE(P_hhr_TAU0, 8, 8, 1, tau + i);
    SS_RECURRENCE(P_hhr_TAU1, P_hhr_IN, 1); //tau
    SS_CONST(P_hhr_Coef, 1065353216, n - 1);
    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_hhr_A);
    SS_CONST(P_hhr_reset, 1, n - 1);

    SS_REPEAT_PORT(n * (n - 1));
    SS_RECURRENCE(P_hhr_OUTlocal, P_hhr_Coef, 1);
    //SS_GARBAGE(P_hhr_OUTremote, 1); //xfer it!
    SS_REPEAT_PORT(N * n);
    SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_QTAU, 1);

    SS_RECURRENCE(P_hhr_O, P_hhr_IN, n - 1);

    SS_SCR_WRITE(P_hhr_OUTlocal, 8 * (n - 1), w_spad);
    //SS_GARBAGE(P_hhr_OUTremote, n - 1); //xfer it!
    //SS_DMA_WRITE(P_hhr_OUTremote, 8, 8, n - 1, buffer); test pass!
    SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_IN, n - 1);

    SS_CONTEXT(3);
    SS_WAIT_SCR_WR();

    SS_CONTEXT(2);
    SS_SCR_WRITE(P_fused_OUT, 8 * (n - 1), w_spad);
    SS_WAIT_SCR_WR();
    SS_SCR_PORT_STREAM(q_spad, 8 * n, 8 * n, N, P_fused_QM);
    SS_SCR_PORT_STREAM(0, 0, 8 * n, N, P_fused_QW);
    SS_2D_CONST(P_fused_Qreset, 2, n - 1, 1, 1, N);

    SS_REPEAT_PORT(n);
    SS_RECURRENCE(P_fused_QV, P_fused_QA, N);
    SS_SCR_PORT_STREAM(0, 0, 8 * n, N, P_fused_QB);
    SS_SCRATCH_READ(q_spad, 8 * N * n, P_fused_QC);
    SS_2D_CONST(P_fused_QSignal, 1, 1, 0, n - 1, N);
    SS_DMA_WRITE(P_fused_Q_MEM, 8 * N, 8, N, q + i);
    if (i < N - 2) {
      SS_SCR_WRITE(P_fused_Q_SPAD, 8 * N * (n - 1), q_spad);
    } else {
      SS_DMA_WRITE(P_fused_Q_SPAD, 8 * N, 8, N, q + N - 1);
    }

    SS_CONTEXT(1);
    SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * n, n - 1, P_hhr_B);
    SS_2D_CONST(P_hhr_reset, 2, n - 1, 1, 1, n - 1);
    SS_SCRATCH_READ(r_trans_spad + 8 * n, 8 * (n - 1) * n, P_hhr_A);

    SS_REPEAT_PORT(n);
    SS_RECURRENCE(P_hhr_O, P_hhr_RA, n - 1);
    SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * n, n - 1, P_hhr_RB);
    SS_2D_CONST(P_hhr_RSignal, 1, 1, 0, n - 1, n - 1);
    SS_DMA_WRITE(P_hhr_R_MEM, 8, 8, n - 1, a + i * N + i + 1);
    SS_SCRATCH_READ(r_trans_spad + 8 * n, 8 * (n - 1) * n, P_hhr_RC);
    SS_SCR_WRITE(P_hhr_R_SPAD, 8 * (n - 1) * (n - 1), r_trans_spad);

    SS_WAIT_SCR_WR();

    //SS_CONTEXT(3);
    //SS_WAIT_ALL();
    //SS_CONTEXT(1);
  }
  SS_CONTEXT(1);
  SS_SCRATCH_DMA_STORE(r_trans_spad, 0, 8, 1, a + N * N - 1);
  SS_CONTEXT(3);
  SS_WAIT_ALL();
}

