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

void gemm(int n, int m, int p, complex<float> *, complex<float> *, complex<float> *);

#endif
