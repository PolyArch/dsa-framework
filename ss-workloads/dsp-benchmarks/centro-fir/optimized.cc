#include <iostream>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#include <complex>

using std::complex;
const complex<float> _zero(0, 0); 
#include <complex>
#include <cmath>
#include <algorithm>
#include "ss_insts.h"

using std::complex;

complex<float> _one = complex<float>(1, 0);

void filter(
    int n, int m,
    complex<float> *a,
    complex<float> *b,
    complex<float> *c,
    complex<float> *
) {
  for (int io = 0; io + m <= n; io += 32) {
    for (int ii = 0; ii < 32; ++ii) {
      int i = io + ii;
      if (i + m > n)
        break;
      for (int j = 0, jj = m - 1; j < m / 2; ++j, --jj) {
        c[i] += (a[i + j] + a[i + jj]) * b[j];
      }
      c[i] += a[i + m / 2] * b[m / 2];
    }
  }
}

