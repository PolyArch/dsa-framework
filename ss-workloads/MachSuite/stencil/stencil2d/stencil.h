#include <stdio.h>
#include <stdlib.h>
#include "support.h"

//Define input sizes
#define col_size 130
#define row_size 130
#define f_size 9

//Data Bounds
#define TYPE int64_t
#define MAX 1000
#define MIN 1

//Set number of iterations to execute
#define MAX_ITERATION 1

void stencil( TYPE orig[row_size * col_size],
        TYPE sol[row_size * col_size],
        TYPE filter[f_size] );

//SB kernel definition
void stencil_sb( TYPE orig[row_size * col_size],
        TYPE sol[row_size * col_size],
        TYPE filter[f_size] );

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
    TYPE orig[row_size*col_size];
    TYPE sol[row_size*col_size];
    TYPE filter[f_size];
};
