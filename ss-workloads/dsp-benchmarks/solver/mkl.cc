#include "sim_timing.h"
#include "mkl.h"
#include <complex>

#define N _N_

std::complex<float> a[11][N * N], b[11][N];
int p[N * N];

int main() {
  LAPACKE_cgetrf(LAPACK_ROW_MAJOR, N, N, (lapack_complex_float*) a, N, p);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[0], N, p, (lapack_complex_float*)b[0], 1);
  begin_roi();
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[1], N, p, (lapack_complex_float*)b[1], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[3], N, p, (lapack_complex_float*)b[3], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[5], N, p, (lapack_complex_float*)b[5], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[7], N, p, (lapack_complex_float*)b[7], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[9], N, p, (lapack_complex_float*)b[9], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[2], N, p, (lapack_complex_float*)b[2], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[4], N, p, (lapack_complex_float*)b[4], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[6], N, p, (lapack_complex_float*)b[6], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[8], N, p, (lapack_complex_float*)b[8], 1);
  LAPACKE_cgetrs(LAPACK_ROW_MAJOR, 'n', N, 1, (lapack_complex_float*)a[10], N, p, (lapack_complex_float*)b[10], 1);
  end_roi();
}

#undef N
