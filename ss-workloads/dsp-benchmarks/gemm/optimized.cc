#include <complex>
#include <stdint.h>
#include <cmath>
#include <algorithm>

using std::complex;

#define PI 3.14159265358979303

void gemm(int n, int m, int p, complex<float> *a, complex<float> *b, complex<float> *c) {
  for (int i = 0; i < n; ++i) {
    for (int k = 0; k < m; ++k) {
      complex<float> tmp = a[i * m + k];
      for (int j = 0; j < p; ++j) {
        complex<float> delta(tmp * b[k * p + j]);
        c[i * p + j] += tmp * b[k * p + j];
      }
    }
  }
}
