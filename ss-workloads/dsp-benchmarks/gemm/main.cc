#include "gemm.h"
#include "loader.dfg.h"
#include "fileop.h"
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#include "ss_insts.h"

using std::complex;

#ifdef LATENCY
#define LANES 8
#else
#define LANES 1
#endif

#define N _N_
#define M _M_
#define P _P_

complex<float> a[N * M + 8 * M], b[M * P + 8 * P], c[N * P + 8 * P], cc[N * P + 8 * P];

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, N * M, a);
  read_n_float_complex(input_data, P * M, b);

  int m = M;
  int p = P;
  for (int i = 0; i < LANES; ++i) {
    SS_CONTEXT(1 << i);
    SS_CONFIG(loader_config, loader_size);
    SS_DMA_READ(b, 0, 8 * m * p, 1, P_loader_In);
    SS_SCR_WRITE(P_loader_Out, 8 * m * p, 0);
    SS_WAIT_SCR_WR();
  }
  SS_WAIT_ALL();

  gemm(N, M, P, a, b, cc);
  begin_roi();
  gemm(N, M, P, a, b, c);
  end_roi();
  sb_stats();

  //if (!compare_n_float_complex( ref_data, N * P, c)) {
  //  puts("Error result!");
  //  return 1;
  //}

  //puts("result correct!");
  return 0;
}

#undef N
#undef M
#undef P
