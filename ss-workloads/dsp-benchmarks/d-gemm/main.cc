#include "gemm.h"
#include "fileop.h"
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#include "ss-config/fixed_point.h"
#define PI 3.14159265358979303

using std::complex;

complex<float> a[_N_ * _M_], b[_M_ * _P_], c[_N_ * _P_], cc[_N_ * _P_];

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_ * _M_, a);
  read_n_float_complex(input_data, _P_ * _M_, b);

  gemm(_N_, _M_, _P_, a, b, cc);
  begin_roi();
  gemm(_N_, _M_, _P_, a, b, c);
  end_roi();
  sb_stats();

  if (!compare_n_float_complex(ref_data, _N_ * _P_, c)) {
    puts("Error result!");
    return 1;
  }

  puts("result correct!");
  return 0;
}
