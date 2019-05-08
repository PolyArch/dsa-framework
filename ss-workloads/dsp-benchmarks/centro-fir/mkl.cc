#include "mkl_vsl.h"
#include "mkl.h"


#include "filter.h"
#include <complex>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "sim_timing.h"
#include "fileop.h"
#include <iostream>

using std::complex;

complex<float> a[_N_], b[_M_], c[_N_];

int main() {
  FILE *input_data = fopen("input.data", "r"), *ref_data = fopen("ref.data", "r");
  if (!input_data || !ref_data) {
    puts("Data error!");
    return 1;
  }

  read_n_float_complex(input_data, _N_, a);
  read_n_float_complex(input_data, _M_, b);

  VSLConvTaskPtr task;
  int status, i;

  int mode = VSL_CONV_MODE_AUTO;

  /*
  *  Create task descriptor (create descriptor of problem)
  */
  status = vslcConvNewTask1D(&task, mode, _M_, _N_, _N_);
  if( status != VSL_STATUS_OK ){
    printf("ERROR: creation of job failed, exit with %d\n", status);
    return 1;
  }

  status = vslcConvExec1D(
      task,
      (lapack_complex_float *) b, 1,
      (lapack_complex_float *) a, 1,
      (lapack_complex_float *) c, 1
  );

  /*
  *  Execute task (Calculate 1 dimension convolution of two arrays)
  */
  begin_roi();
  status = vslcConvExec1D(
      task,
      (lapack_complex_float *) b, 1,
      (lapack_complex_float *) a, 1,
      (lapack_complex_float *) c, 1
  );
  end_roi();
  if( status != VSL_STATUS_OK ){
    printf("ERROR: job status bad, exit with %d\n", status);
    return 1;
  }

  /*
  *  Delete task object (delete descriptor of problem)
  */
  status = vslConvDeleteTask(&task);
  if( status != VSL_STATUS_OK ){
      printf("ERROR: failed to delete task object, exit with %d\n", status);
      return 1;
  }

  if (!compare_n_float_complex(ref_data, _N_ - _M_ + 1, c + _M_ - 1))
    return 1;

  printf("EXAMPLE PASSED\n");
  return 0;
}
