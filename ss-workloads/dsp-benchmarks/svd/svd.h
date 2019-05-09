#ifndef svd_h
#define svd_h

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <complex>
#include <iostream>
#include <iomanip>

#define BN 4
#define NN (_N_ + BN - 1)

#define eps 1e-5
using std::complex;

struct complex_t {
  float real;
  float imag;
};

void svd(complex<float> *, complex<float> *, float *, complex<float> *);

#endif
