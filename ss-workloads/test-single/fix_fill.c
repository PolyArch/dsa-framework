#include "testing.h"
#include "add1_vec.dfg.h"

uint64_t input[4];
uint64_t output[16];
uint64_t answer[16];


int main(int argc, char* argv[]) {
  init();
  input[0]=1;
  input[1]=2;
  input[2]=3;
  input[3]=4;

  answer[0]=2;
  answer[1]=2;
  answer[2]=3;
  answer[3]=4;
  answer[4]=2;
  answer[5]=1;
  answer[6]=1;
  answer[7]=1;
  answer[8]=2;
  answer[9]=3;
  answer[10]=4;
  answer[11]=1;
  answer[12]=2;
  answer[13]=3;
  answer[14]=4;
  answer[15]=5;

  begin_roi();
  SS_CONFIG(add1_vec_config,add1_vec_size);
  SS_DMA_READ(&input[0],8,8,  1, P_add1_vec_in);
  SS_DMA_READ(&input[0],8,8,  3, P_add1_vec_in);

  SS_FILL_MODE(POST_ZERO_FILL);

  SS_DMA_READ(&input[0],8,8, 1, P_add1_vec_in);
  SS_DMA_READ(&input[0],8,8, 3,P_add1_vec_in);
  SS_DMA_READ(&input[0],8,8, 4,P_add1_vec_in);

  SS_DMA_WRITE(P_add1_vec_out,8,8, 16,&output[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],answer,output,16);
}
