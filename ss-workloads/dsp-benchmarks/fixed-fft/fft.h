#ifndef svd_h
#define svd_h

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

complex<int16_t> *fft(complex<int16_t> *, complex<int16_t> *, complex<float> *);

#endif
