#include <complex>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "ss_insts.h"
#include "sim_timing.h"
#include "compute.dfg.h"
#include "fine1.dfg.h"
#include "fine2.dfg.h"

using std::complex;

static bool _first_time = true;

complex<float> *fft(complex<float> *from, complex<float> *to, complex<float> *w) {
  //if (!_first_time)
    //begin_roi();

  int N = _N_;
  complex<float> *_w = w + _N_ - 2;

  SS_CONFIG(compute_config, compute_size);

  int blocks = N / 2;
  int span = N / blocks;

  //SS_DMA_READ(from,          2 * blocks * 8, blocks * 8, span / 2, P_compute_L);
  //SS_DMA_READ(from + blocks, 2 * blocks * 8, blocks * 8, span / 2, P_compute_R);
  //SS_CONST(P_compute_W, 1065353216, N / 8);
    
  //SS_SCR_WRITE(P_compute_A, N * 4, 0);
  //SS_SCR_WRITE(P_compute_B, N * 4, N * 4);
  //SS_WAIT_SCR_WR();
    
  //blocks >>= 1;
  //span <<= 1;

  int scr = 0;

  while (blocks >= 4) {

    //SS_CONTEXT(1);
    SS_SCR_PORT_STREAM(scr             , 2 * blocks * 8, blocks * 8, span / 2, P_compute_L);
    SS_SCR_PORT_STREAM(scr + blocks * 8, 2 * blocks * 8, blocks * 8, span / 2, P_compute_RR);
    SS_REPEAT_PORT(blocks / 4);
    SS_DMA_READ(_w, 0, 4 * span, 1, P_compute_W);

    scr ^= 8192;
    SS_SCR_WRITE(P_compute_A, N * 4, scr);
    SS_SCR_WRITE(P_compute_B, N * 4, scr + N * 4);
    SS_WAIT_SCR_WR();

    blocks >>= 1;
    span <<= 1;
    _w -= span / 2;
  }

  SS_WAIT_ALL();
  
  //SS_CONFIG(fine2_config, fine2_size);
  SS_FILL_MODE(STRIDE_DISCARD_FILL);
  SS_SCR_PORT_STREAM(scr             , 2 * blocks * 8, blocks * 8, span / 2, P_compute_L);
  SS_SCR_PORT_STREAM(scr + blocks * 8, 2 * blocks * 8, blocks * 8, span / 2, P_compute_RR);
  //SS_REPEAT_PORT(8);
  SS_DMA_READ(_w, 0, N * 2, 1, P_compute_W);
  scr ^= 8192;
  SS_SCR_WRITE(P_compute_A, N * 4, scr); SS_GARBAGE(P_compute_A, N / 2);
  SS_SCR_WRITE(P_compute_B, N * 4, scr + N * 4); SS_GARBAGE(P_compute_B, N / 2);
  blocks >>= 1;
  span <<= 1;
  SS_WAIT_ALL();

  //SS_CONFIG(fine1_config, fine1_size);
  SS_DMA_READ(w, 8, 8, N / 2, P_compute_W);
  SS_SCR_PORT_STREAM(scr             , 2 * blocks * 8, blocks * 8, span / 2, P_compute_L);
  SS_SCR_PORT_STREAM(scr + blocks * 8, 2 * blocks * 8, blocks * 8, span / 2, P_compute_RR);
  SS_DMA_WRITE(P_compute_A, 0, 4 * N, 1, to); SS_GARBAGE(P_compute_A, N + N / 2);
  SS_DMA_WRITE(P_compute_B, 0, 4 * N, 1, to + N / 2);  SS_GARBAGE(P_compute_B, N + N / 2);
  SS_WAIT_ALL();

  //if (!_first_time)
    //end_roi();
  //else
    //_first_time = false;

  return to;
}
