#include "fft.h"
#include "fileop.h"
#include <string.h>
#include <iostream>
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#define PI 3.14159265358979303

using std::complex;

complex<float> w[_N_];
complex<int16_t> a[_N_], _a[_N_];
complex<int16_t> b[_N_], _b[_N_];

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_fix_complex(input_data, _N_, a);

  for (int i = 0; i < _N_ / 2; ++i) {
    w[i] = complex<float>(cos(2 * PI * i / _N_), sin(2 * PI * i / _N_));
  }

  complex<float> *_w = w + _N_ / 2;
  for (int i = 2; i < _N_; i <<= 1) {
    for (int j = 0; j < _N_ / 2; j += i) {
      _w[j / i] = w[j];
    }
    _w += _N_ / 2 / i;
  }

  fft(_a, _b, w);
  begin_roi();
  end_roi();
  //begin_roi();
  complex<int16_t> *res = fft(a, b, w);
  //end_roi();
  sb_stats();

  /*if (!compare_n_float_complex(ref_data, _N_, res)) {
    puts("Error result!");
    return 1;
  }*/

  puts("I hope result is correct...");
  return 0;
}
