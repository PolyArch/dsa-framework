#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "spmv.h"

#define ROW 0
#define COL 1

// Row first, then column comparison
int compar(const void *v_lhs, const void *v_rhs)
{
  int row_lhs = ((int *)v_lhs)[ROW];
  int row_rhs = ((int *)v_rhs)[ROW];
  int col_lhs = ((int *)v_lhs)[COL];
  int col_rhs = ((int *)v_rhs)[COL];

  if( row_lhs==row_rhs ) {
    if( col_lhs==col_rhs )
      return 0;
    else if( col_lhs<col_rhs )
      return -1;
    else
      return 1;
  } else if( row_lhs<row_rhs ) {
    return -1;
  } else {
    return 1;
  }
}

int main(int argc, char **argv)
{
  struct bench_args_t data;
  struct stat file_info;
  char *current, *next, *buffer;
  int status, i, fd, nbytes;
  int coords[NNZ][2]; // row, col
  struct prng_rand_t state;

  // Load matrix file
  fd = open("494_bus_full.mtx", O_RDONLY);
  assert( fd>=0 && "couldn't open matrix" );
  status = fstat( fd, &file_info );
  assert( status==0 && "couldn't get filesize of matrix" );
  buffer = malloc(file_info.st_size+1);
  buffer[file_info.st_size]=(char)0;
  nbytes = 0;
  do {
    status = read(fd, buffer, file_info.st_size-nbytes);
    assert(status>=0 && "Couldn't read from matrix file");
    nbytes+=status;
  } while( nbytes<file_info.st_size );
  close(fd);

  // Parse matrix file
  current = buffer;
  next = strchr(current, '\n');// skip first two lines
  *next = (char)0;
  current = next+1;
  for(i=0; i<NNZ; i++) {
    next = strchr(current, '\n');
    *next = (char)0;
    current = next+1;
    status = sscanf(current, "%d %d %lf", &coords[i][ROW], &coords[i][COL], &data.val[i]);
    assert(status==3 && "Parse error in matrix file");
  }

  // Sort by row
  qsort(coords, NNZ, 2*sizeof(int), &compar);

  // Fill data structure
  for(i=0; i<NNZ; i++)
    data.cols[i] = coords[i][COL]-1;
  memset(data.rowDelimiters, 0, (N+1)*sizeof(int));
  for(i=0; i<NNZ; i++)
    data.rowDelimiters[coords[i][ROW]-1+1] += 1; // count
    // (-1 because matrix is 1-indexed, +1 because it's counting cells before it)
  for(i=1; i<N+1; i++)
    data.rowDelimiters[i] += data.rowDelimiters[i-1]; // scan

  // Set vector
  prng_srand(1,&state);
  for( i=0; i<N; i++ )
    data.vec[i] = ((double)prng_rand(&state))/((double)PRNG_RAND_MAX);

  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );
  data_to_input(fd, (void *)(&data));

  return 0;
}
