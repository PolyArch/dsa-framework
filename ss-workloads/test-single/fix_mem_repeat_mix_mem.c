#include "testing.h"

#define MAX_REPEAT 8
#define MAX_SIZE   256

DTYPE answer[MAX_SIZE*MAX_REPEAT];
DTYPE out_big[MAX_SIZE*MAX_REPEAT];
DTYPE in_mine[MAX_SIZE];

int main(int argc, char* argv[]) {
  init();

  for(int i = 0; i < MAX_SIZE; ++i) {
    in_mine[i]=i;
  }

  uint64_t* answer64 = (uint64_t*) &answer[0];
  uint64_t* out64    = (uint64_t*) &out_big[0];
  uint64_t* in64     = (uint64_t*) &in_mine[0];

  uint64_t total_i=0;
  uint64_t total_rep_i=0;

  //linear for 5
  int repeat=1;
  int ntimes=5;
  for(int i=0; i < ntimes; ++i) {
    for(int j=0; j < repeat; ++j) {
      answer64[total_rep_i]=in64[total_i];
      total_rep_i++;
    }
    total_i++;
  }

  //repeat 7 for 3
  repeat=7;
  ntimes=3;
  for(int i=0; i < ntimes; ++i) {
    for(int j=0; j < repeat; ++j) {
      answer64[total_rep_i]=in64[total_i];
      total_rep_i++;
    }
    total_i++;
  }

  //repeat 4 for 6
  repeat=4;
  ntimes=6;
  for(int i=0; i < ntimes; ++i) {
    for(int j=0; j < repeat; ++j) {
      answer64[total_rep_i]=in64[total_i];
      total_rep_i++;
    }
    total_i++;
  }

  //repeat 1 for 2
  repeat=1;
  ntimes=2;
  for(int i=0; i < ntimes; ++i) {
    for(int j=0; j < repeat; ++j) {
      answer64[total_rep_i]=in64[total_i];
      total_rep_i++;
    }
    total_i++;
  }

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_DMA_READ(&in64[0],8,8,5,P_none_in);

  SS_REPEAT_PORT(7);
  SS_DMA_READ(&in64[5],8,8,3,P_none_in);

  SS_REPEAT_PORT(4);
  SS_DMA_READ(&in64[8],8,8,6,P_none_in);

  SS_DMA_READ(&in64[14],8,8,2,P_none_in);

  SS_DMA_WRITE(P_none_out,8,8,total_rep_i,&out_big[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<DTYPE>(argv[0],out_big,answer,ASIZE*repeat);

}
