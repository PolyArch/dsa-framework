#include "testing.h"
#include <iostream>

#define LEN 256
#define SLEN 64

struct item_array {
  uint32_t item1;
  uint32_t item2;
  uint32_t item3;
  uint32_t item4;
} array[LEN];

int main(int argc, char* argv[]) {
  init();

  uint16_t ind_array[SLEN]; //16-bit ind type

  uint32_t known[SLEN*3]; //32-bit data type
  uint32_t output[SLEN*3];

  for(int i = 0; i < 64; ++i) {
    ind_array[i] = rand()%(SLEN-1);
  }
//  ind_array[0]=0;
//  ind_array[1]=1;
//  ind_array[2]=5;
//  ind_array[3]=4;

  for(int i = 0; i<LEN; ++i) {
    array[i].item1 = i*LEN+1;
    array[i].item2 = i*LEN+2;
    array[i].item3 = i*LEN+3;
    array[i].item4 = i*LEN+4;  
  }
  //lets copy array to scratch
  SS_DMA_SCRATCH_LOAD(array,8,8,sizeof(array)/8,0);
  SS_WAIT_SCR_WR();

  for(int i = 0; i<SLEN; ++i) {
    int ind = ind_array[i];

    known[3*i+0]=array[ind].item1;
    known[3*i+1]=array[ind].item3;
    known[3*i+2]=array[ind].item4;

    output[3*i+0]=-1;
    output[3*i+1]=-1;
    output[3*i+2]=-1;
  }

  SS_CONFIG(none_config,none_size);

  begin_roi();
  for(int i = 0; i < 10; ++ i) {
    SS_DMA_READ(&ind_array[0],8,8,SLEN/4,P_IND_1);
  
    //itype, dtype, mult, offset
    SS_CONFIG_INDIRECT2(T16,T32,sizeof(item_array), 2, 3);  //2 offsets for item 3 and item 4
    SS_INDIRECT_SCR(P_IND_1,0,SLEN,P_none_in);
  
    SS_DMA_WRITE(P_none_out,8,8,SLEN*3/2,&output[0]);
  }
  SS_WAIT_ALL();
  end_roi();


  compare<uint32_t>(argv[0],output,known,(int)SLEN*3);
}
