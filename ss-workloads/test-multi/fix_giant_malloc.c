#include "testing.h"
#include <cstdio>
#include <stdlib.h>

#define VTYPE uint64_t
// #define GSIZE 1024*1024*128
#define GSIZE 1024*128*128

int main(int argc, char* argv[]) {
  char* giant_array = (char*) aligned_alloc(64, GSIZE);
  // int n = GSIZE/2; // earlier it was 1
  int n = GSIZE/5; // -64; // earlier it was 1

  SS_CONFIG(none_config,none_size);
 
  SS_DMA_READ(&giant_array[0], 8, 8, n/8, P_none_in);
  // SS_DMA_READ(&giant_array[(GSIZE/3)*1],8,8,n/8,P_none_in);
  // SS_DMA_READ(&giant_array[3], 8, 8, n/8, P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,n/8,&giant_array[0]);
 
  // SS_DMA_READ(&giant_array[(GSIZE/3)*1],8,8,k,P_none_in);
  // // SS_DMA_READ(&giant_array[(GSIZE/3)*2],8,8,k,P_none_in);

  // SS_DMA_WRITE(P_none_out,8,8,k,&giant_array[GSIZE/2]);
  // // SS_DMA_WRITE(P_none_out,8,8,k,&giant_array[GSIZE-1]);

  SS_WAIT_ALL();

  delete giant_array;
}
