#include "svd.h"
#include "fileop.h"
#include <algorithm>
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#include <cstring>

using std::complex;

complex<float> a[_N_ * _N_], U[_N_ * _N_], V[_N_ * _N_], tmp[_N_ * _N_], res[_N_ * _N_];
complex<float> ss[_N_];
float S[_N_], SS[_N_];

complex<float> aa[_N_ * _N_], bb[_N_ * _N_];

int main() {
  FILE *input_data = fopen("input.data", "r");
  FILE *ref_data = fopen("ref.data", "r");

  if (!input_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_ * _N_, a);

  memset(V, 0, sizeof V);
  for (int i = 0; i < _N_; ++i)
    V[i * _N_ + i] = 1.0f;

  memcpy(aa, a, sizeof a);
  svd(aa, bb, SS, bb);
  begin_roi();
  svd(a, U, S, V);
  end_roi();
  sb_stats();

  for (int i = 0; i < _N_; ++i)
    for (int j = 0; j < _N_; ++j)
      tmp[i * _N_ + j] = U[i * _N_ + j] * S[j];

  for (int i = 0; i < _N_; ++i)
     for (int j = 0; j < _N_; ++j)
      for (int k = 0; k < _N_; ++k)
        res[i * _N_ + j] += tmp[i * _N_ + k] * V[k * _N_ + j];

  std::sort(S, S + _N_, [](float a, float b) { return a > b; });
  for (int i = 0; i < _N_; ++i)
    ss[i] = S[i];

  if (!compare_n_float_complex(ref_data, _N_, ss)) {
    puts("singular value error!");
    return 0;
  }

  if (!compare_n_float_complex(ref_data, _N_ * _N_, res)) {
    puts("origin matrix error!");
    return 0;
  }

  puts("result correct!");

  return 0;
}
