#ifndef gemm_h
#define gemm_h

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <complex>

#define eps 1e-4
using std::complex;

struct complex_t {
  float real;
  float imag;
};

void qr(complex<float> *, complex<float> *, complex<float> *);

#endif
