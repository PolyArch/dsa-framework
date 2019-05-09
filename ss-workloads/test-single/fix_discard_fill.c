#include "testing.h"
#include "add1_vec_o2.dfg.h"

uint64_t input[6];
uint64_t output[16];
uint64_t answer[16];


int main(int argc, char* argv[]) {
  init();
  input[0]=1;
  input[1]=2;
  input[2]=3;
  input[3]=4;
  input[4]=5;
  input[5]=6;


  answer[0]=2;  //every fourth input was 0
  answer[1]=3;
  answer[2]=6;
  answer[3]=7;
  answer[4]=2;
  answer[5]=3;
  answer[6]=4;
  answer[7]=5;

  SS_CONFIG(add1_vec_o2_config,add1_vec_o2_size);
  SS_FILL_MODE(STRIDE_DISCARD_FILL);

  begin_roi();
  SS_DMA_READ(&input[0],   8, 8*6, 1, P_add1_vec_o2_in);
  SS_DMA_READ(&input[0],   8, 8*2, 1, P_add1_vec_o2_in);

  SS_DMA_WRITE(P_add1_vec_o2_outA,8,8, 6,&output[0]);
  SS_DMA_WRITE(P_add1_vec_o2_outB,8,8, 2,&output[6]);

  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],answer,output,8);
}
