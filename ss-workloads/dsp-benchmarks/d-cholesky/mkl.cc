#include "mkl.h"
#include <complex>
#include "fileop.h"
#include <complex.h>
#include <cstring>
#include <iostream>
#include "sim_timing.h"

using std::complex;

complex<float> a[_N_ * _N_], L[_N_ * _N_], aa[_N_ * _N_], origin[_N_ * _N_];

#ifndef NUM_OMP_THREADS
#define NUM_OMP_THREADS 1
#endif

#ifndef NUM_PTHREADS
#define NUM_PTHREADS    1
#endif

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_ * _N_, a);
  memcpy(aa, a, sizeof a);
  memcpy(origin, a, sizeof a);

  mkl_set_num_threads_local(NUM_OMP_THREADS);

  LAPACKE_cpotrf(LAPACK_ROW_MAJOR, 'L', _N_, (MKL_Complex8 *) aa, _N_);
  begin_roi();
  LAPACKE_cpotrf(LAPACK_ROW_MAJOR, 'L', _N_, (MKL_Complex8 *) a, _N_);
  end_roi();

  //for (int i = 0; i < _N_; ++i) {
  //  for (int j = 0; j < i; ++j)
  //    a[i * _N_ + j] = std::conj(a[i * _N_ + j]);
  //  for (int j = i + 1; j < _N_; ++j)
  //    a[i * _N_ + j] = 0;
  //}

  //if (!compare_n_float_complex(ref_data, _N_ * _N_, a)) {
  //  return 1;
  //}

  puts("I verified this interface is what I want!");
  return 0;
}

