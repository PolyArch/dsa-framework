#include "testing.h"
#include <iostream>

#define LEN 128

struct item_array {
  uint64_t item1;
  uint64_t item2;
  uint64_t item3;
  uint64_t item4;
} array[LEN];

int main(int argc, char* argv[]) {
  init();

  uint64_t ind_array[LEN];
  uint64_t known[LEN*3];
  uint64_t output[LEN*3];


  for(int i = 0; i<LEN; ++i) {
    array[i].item1 = i+1;
    array[i].item2 = i+2;
    array[i].item3 = i+3;
    array[i].item4 = i+4;

    ind_array[i]=LEN-i-1;

    //assume offsets 1 and 4
    known[3*i+0]=(LEN-i-1)+1;
    known[3*i+1]=(LEN-i-1)+3;
    known[3*i+2]=(LEN-i-1)+4;
    output[3*i+0]=-1;
    output[3*i+1]=-1;
    output[3*i+2]=-1;
  }

  SS_CONFIG(none_config,none_size);

  begin_roi();
  SS_DMA_READ(&ind_array[0],8,8,LEN,P_IND_1);

  //itype, dtype, mult, offset
  SS_CONFIG_INDIRECT2(T64,T64,sizeof(item_array), 2, 3);  //2 offsets for item 3 and item 4
  SS_INDIRECT(P_IND_1,&array[0],LEN,P_none_in);

  SS_DMA_WRITE(P_none_out,8,8,LEN*3,&output[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],output,known,(int)LEN);
}
