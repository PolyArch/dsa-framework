#include <cstdio>
#include "solver.h"
#include "sim_timing.h"

#define N _N_

complex<float> a[N * N], v[N];

int main() {
  //for (int i = 0; i < N; ++i)
  //  for (int j = 0; j <= i; ++j)
  //    scanf("%f", a + i * N + j);
  //for (int i = 0; i < N; ++i)
  //  scanf("%f", v + i);

  solver(a, v);
  solver(a, v);
  begin_roi();
  solver(a, v);
  solver(a, v);
  end_roi();
  sb_stats();
  return 0;
}

#undef N
