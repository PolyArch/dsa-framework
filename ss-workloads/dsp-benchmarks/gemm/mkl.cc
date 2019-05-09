#include "mkl.h"
#include <complex>
#include "fileop.h"
#include <complex.h>
#include <iostream>
#include "sim_timing.h"

using std::complex;

#ifndef NUM_OMP_THREADS
#define NUM_OMP_THREADS 1
#endif

#ifndef NUM_PTHREADS
#define NUM_PTHREADS    1
#endif

complex<float> a[_N_ * _M_], b[_M_ * _P_], c[_N_ * _P_];
complex<float> _a[_N_ * _M_], _b[_M_ * _P_], _c[_N_ * _P_];
complex<float> _one(1), _zero(0);

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_ * _M_, a);
  read_n_float_complex(input_data, _P_ * _M_, b);

  mkl_set_num_threads_local(NUM_OMP_THREADS);

  cblas_cgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, _N_, _P_, _M_,
      &_one,
      _a, _M_,
      _b, _P_,
      &_zero,
      _c, _P_
  );
  begin_roi();
  cblas_cgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, _N_, _P_, _M_,
      &_one,
      a, _M_,
      b, _P_,
      &_zero,
      c, _P_
  );
  end_roi();
  sb_stats();

  if (!compare_n_float_complex( ref_data, _N_ * _P_, c)) {
    puts("Error result!");
    return 1;
  }

  puts("result correct!");
  return 0;
}
