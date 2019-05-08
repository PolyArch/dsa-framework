#include <iostream>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <complex>
#include "cholesky.h"
#include "matvec.h"

using std::complex;

struct complex_t {
  float real, imag;
};

#define N _N_
void cholesky(complex<float> *a, complex<float> *L) {
  #pragma config
  for (int i = 0; i < N; ++i) {
    complex<float> div = std::sqrt(a[i * (N + 1)]);
    complex<float> *b = a + i * (N + 1);
    L[i * (N + 1)] = div;
    for (int j = i + 1; j < N; ++j)
      L[j * N + i] = b[j - i] / div;
    complex<float> v = a[i * (N + 1)];
    float norm = 1 / complex_norm(v);
    for (int j = i + 1; j < N; ++j) {
      for (int k = j; k < N; ++k) {
        complex<float> temp = complex<float>(complex_conj_mul(b[j - i], b[k - i]));
        complex<float> val  = complex<float>(complex_conj_mul(v, temp));
        complex<float> delta= complex<float>(complex_mul_cons(val, norm));
        a[j * N + k] -= delta;
      }
    }
  }
}
#undef N
