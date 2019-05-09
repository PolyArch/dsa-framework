#ifndef svd_h
#define svd_h

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <complex>

#define eps 2e-2
using std::complex;

void filter(int, int, complex<float> *, complex<float> *, complex<float> *, complex<float> *);

#endif
