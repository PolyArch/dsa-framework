#include "mkl.h"
#include "fileop.h"
#include <cstring>
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#include <iostream>

using std::complex;

complex<float> a[_N_ * _N_], tau[_N_];
complex<float> aa[_N_ * _N_];
complex<float> b[_N_ * _N_];
complex<float> _one(1), _zero(0);

#ifndef NUM_OMP_THREADS
#define NUM_OMP_THREADS 4
#endif

#ifndef NUM_PTHREADS
#define NUM_PTHREADS    4
#endif

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  std::cout << std::fixed;
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  int n = _N_;

  read_n_float_complex(input_data, n * n, a);
  memcpy(aa, a, sizeof a);

  mkl_set_num_threads_local(NUM_OMP_THREADS);

  LAPACKE_cgeqrf(CblasRowMajor, n, n, (lapack_complex_float *) aa, n, (lapack_complex_float *) aa);
  LAPACKE_cungqr(CblasRowMajor, n, n, n, (lapack_complex_float *) aa, n, (lapack_complex_float *) aa);
  begin_roi();
  LAPACKE_cgeqrf(CblasRowMajor, n, n, (lapack_complex_float *) a, n, (lapack_complex_float *) tau);
  LAPACKE_cungqr(CblasRowMajor, n, n, n, (lapack_complex_float *) a, n, (lapack_complex_float *) tau);
  end_roi();
  sb_stats();

  puts("I hope the result is correct, because the interface of QR is weird...");
  return 0;
}
