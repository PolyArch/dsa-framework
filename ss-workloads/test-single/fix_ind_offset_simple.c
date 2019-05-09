#include "testing.h"
#include <iostream>

#define LEN 16
#define SLEN 4

struct item_array {
  uint64_t item1;
  uint64_t item2;
  uint64_t item3;
  uint64_t item4;
} array[LEN];

int main(int argc, char* argv[]) {
  init();

  uint16_t ind_array[SLEN];
  uint64_t known[SLEN*3];
  uint64_t output[SLEN*3];

  ind_array[0]=0;
  ind_array[1]=1;
  ind_array[2]=5;
  ind_array[3]=4;

  for(int i = 0; i<LEN; ++i) {
    array[i].item1 = i*LEN+1;
    array[i].item2 = i*LEN+2;
    array[i].item3 = i*LEN+3;
    array[i].item4 = i*LEN+4;  
  }
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
  SS_DMA_READ(&ind_array[0],8,8,SLEN/4,P_IND_1);

  //itype, dtype, mult, offset
  SS_CONFIG_INDIRECT2(T16,T64,sizeof(item_array), 2, 3);  //2 offsets for item 3 and item 4
  SS_INDIRECT(P_IND_1,&array[0],SLEN,P_none_in);

  SS_DMA_WRITE(P_none_out,8,8,SLEN*3,&output[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],output,known,(int)SLEN*3);
}
