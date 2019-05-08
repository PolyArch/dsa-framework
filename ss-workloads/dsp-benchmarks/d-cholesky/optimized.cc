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

void cholesky(complex<float> *a, complex<float> *L) {
  for (int i = 0; i < _N_; ++i) {
    complex<float> div = std::sqrt(a[i * (_N_ + 1)]);
    complex<float> *b = a + i * (_N_ + 1);
    L[i * (_N_ + 1)] = div;
    for (int j = i + 1; j < _N_; ++j)
      L[j * _N_ + i] = b[j - i] / div;
    complex<float> v = a[i * (_N_ + 1)];
    float norm = 1 / complex_norm(v);
    for (int j = i + 1; j < _N_; ++j) {
      for (int k = j; k < _N_; ++k) {
        complex<float> temp = complex<float>(complex_conj_mul(b[j - i], b[k - i]));
        complex<float> val  = complex<float>(complex_conj_mul(v, temp));
        complex<float> delta= complex<float>(complex_mul_cons(val, norm));
        a[j * _N_ + k] -= delta;
      }
    }
  }
}

