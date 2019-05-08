#include "filter.h"
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "ss_insts.h"
#include "sim_timing.h"
#include "loader.dfg.h"
#include "fileop.h"
#include <iostream>

#define PI 3.14159265358979303

using std::complex;

complex<float> a[_N_], b[_M_], c[_N_], cc[_N_], w[_N_ + _M_];

#ifdef LATENCY
#define LANES 8
#else
#define LANES 1
#endif

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_, a);
  read_n_float_complex(input_data, _M_, b);

  for (int i = 0; i < LANES; ++i) {
    SS_CONTEXT(1 << i);
    SS_CONFIG(loader_config, loader_size);
    SS_DMA_READ(a, 0, 8 * _N_, 1, P_loader_In);
    SS_SCR_WRITE(P_loader_Out, 8 * _N_, 0);
    SS_WAIT_ALL();
  }

  int n;
  for (n = 1; n < _N_ + _M_; n <<= 1);
  for (int i = 0; i < n / 2; ++i) {
    w[i] = complex<float>(cos(2 * PI * i / n), sin(2 * PI * i / n));
  }

  filter(_N_, _M_, cc, cc, cc, w);
  begin_roi();
  filter(_N_, _M_, a, b, c, w);
  end_roi();
  sb_stats();

  //for (int i = 0; i < _N_ - _M_ + 1; ++i) std::cout << c[i]; std::cout << "\n";

  if (!compare_n_float_complex(ref_data, _N_ - _M_ + 1, c))
    return 1;

  puts("result correct!");
  return 0;
}
