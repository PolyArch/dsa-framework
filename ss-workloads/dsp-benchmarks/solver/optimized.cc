#include <iostream>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <complex>

using std::complex;

#define N _N_

void solver(complex<float> *a, complex<float> *v) {
  for (int i = 0; i < N; ++i) {
    v[i] /= a[i * N + i];
    for (int j = i + 1; j < N; ++j) {
      v[j] -= a[i * N + j] * v[i];
    }
  }
}

#undef N
