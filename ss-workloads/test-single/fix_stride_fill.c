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

  answer[0]=2;  //every fourth input was 0
  answer[1]=3;
  answer[2]=4;
  answer[3]=1;
  answer[4]=2;
  answer[5]=3;
  answer[6]=4;
  answer[7]=1;
  answer[8]=2;
  answer[9]=3;
  answer[10]=4;
  answer[11]=1;
  answer[12]=1;
  answer[13]=1;
  answer[14]=1;
  answer[15]=1;

  SS_CONFIG(add1_vec_config,add1_vec_size);
  SS_FILL_MODE(STRIDE_ZERO_FILL);

  begin_roi();
  SS_DMA_READ(&input[0],0,3*8,  3, P_add1_vec_in);


  SS_SCR_PORT_STREAM(0,4*8,3*8,1,P_add1_vec_in);

  SS_DMA_WRITE(P_add1_vec_out,8,8, 16,&output[0]);

  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],answer,output,16);
}
