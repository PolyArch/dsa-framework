#include "qr.h"
#include "fileop.h"
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#include <iostream>
#include "loader.dfg.h"

#ifndef __x86_64__
#include "ss_insts.h"
#endif

using std::complex;

complex<float> a[_N_ * _N_], tau[_N_], q[_N_ * _N_];
complex<float> aa[_N_ * _N_];

int main() {
  int N = _N_;
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  std::cout << std::fixed;
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, N * N, a);

#ifndef __x86_64__
  SS_CONFIG(loader_config, loader_size);
  for (int i = 0; i < N; ++i) {
    SS_DMA_READ(a + i, 8 * N, 8, N, P_loader_In);
    SS_SCR_WRITE(P_loader_Out, 8 * N, i * N * 8);
  }
  SS_WAIT_ALL();
#endif

  qr(aa, aa, aa);
  begin_roi();
  qr(a, q, tau);
  end_roi();
  sb_stats();

  //for (int i = 1; i < N; ++i)
  //  for (int j = 0; j < i; ++j)
  //    a[i * N + j] = 0.0f;
  
  //if (!compare_n_float_complex(ref_data, N * N, a)) {
  //  //puts("error r");
  //  return 0;
  //}

  //if (!compare_n_float_complex(ref_data, N - 1, tau)) {
  //  //puts("error tau");
  //  return 0;
  //}

  //if (!compare_n_float_complex(ref_data, N * N, q)) {
  //  //puts("error q");
  //  return 0;
  //}

  //puts("result correct!");
  return 0;
}
