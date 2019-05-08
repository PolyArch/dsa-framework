#include "mkl_dfti.h"
#include "mkl.h"
#include <iostream>
#include <cstring>
#include "sim_timing.h"
#include "fileop.h"
#include <complex>

using std::complex;

complex<float> a[26][_N_], b[26][_N_];

#ifndef NUM_OMP_THREADS
#define NUM_OMP_THREADS 4
#endif

#ifndef NUM_PTHREADS
#define NUM_PTHREADS    4
#endif

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_, a[0]);
  for (int i = 1; i < 26; ++i) {
    memcpy(a[i], a[0], sizeof (complex<float>) * _N_);
  }

  mkl_set_num_threads_local(NUM_OMP_THREADS);

  MKL_LONG status;

  DFTI_DESCRIPTOR_HANDLE my_desc1_handle;
  DFTI_DESCRIPTOR_HANDLE my_desc2_handle;

  DftiCreateDescriptor( &my_desc1_handle, DFTI_SINGLE, DFTI_COMPLEX, 1, _N_);
  DftiSetValue( my_desc1_handle, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
  DftiCommitDescriptor( my_desc1_handle );
  DftiComputeBackward( my_desc1_handle, a[0], b[0]);
  begin_roi();
  DftiComputeBackward(my_desc1_handle, a[1], b[1]);
  DftiComputeBackward(my_desc1_handle, a[3], b[3]);
  DftiComputeBackward(my_desc1_handle, a[2], b[2]);
  DftiComputeBackward(my_desc1_handle, a[4], b[4]);
  DftiComputeBackward(my_desc1_handle, a[5], b[5]);
  DftiComputeBackward(my_desc1_handle, a[7], b[7]);
  DftiComputeBackward(my_desc1_handle, a[6], b[6]);
  DftiComputeBackward(my_desc1_handle, a[8], b[8]);
  DftiComputeBackward(my_desc1_handle, a[10], b[10]);
  DftiComputeBackward(my_desc1_handle, a[9], b[9]);
  DftiComputeBackward(my_desc1_handle, a[11], b[11]);
  DftiComputeBackward(my_desc1_handle, a[13], b[13]);
  DftiComputeBackward(my_desc1_handle, a[12], b[12]);
  DftiComputeBackward(my_desc1_handle, a[14], b[14]);
  DftiComputeBackward(my_desc1_handle, a[16], b[16]);
  DftiComputeBackward(my_desc1_handle, a[15], b[15]);
  DftiComputeBackward(my_desc1_handle, a[17], b[17]);
  DftiComputeBackward(my_desc1_handle, a[19], b[19]);
  DftiComputeBackward(my_desc1_handle, a[18], b[18]);
  DftiComputeBackward(my_desc1_handle, a[20], b[20]);
  DftiComputeBackward(my_desc1_handle, a[22], b[22]);
  DftiComputeBackward(my_desc1_handle, a[24], b[24]);
  DftiComputeBackward(my_desc1_handle, a[21], b[21]);
  DftiComputeBackward(my_desc1_handle, a[23], b[23]);
  DftiComputeBackward(my_desc1_handle, a[25], b[25]);
  end_roi();
  DftiFreeDescriptor(&my_desc1_handle);

  return 0;
}
