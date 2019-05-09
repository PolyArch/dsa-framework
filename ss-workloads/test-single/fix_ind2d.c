#include "testing.h"
#include <iostream>

// want to change datatypes of all these arrays (first change none.dfg)
uint64_t ind_array[AWORDS];
uint64_t size_array[AWORDS];
uint64_t offset_array[AWORDS];
uint64_t data_array[AWORDS*2];
uint64_t known[2*AWORDS];
uint64_t output[2*AWORDS];

void kernel() {
  SS_CONFIG(none_config,none_size);

  // act_ind[i] = 0, 1, 2
  // SS_ADD_PORT(P_IND_4); // something wrong here...
  SS_DMA_READ(&ind_array[0],8,8,AWORDS,P_IND_1);
  
  SS_DMA_READ(&ind_array[0],8,8,AWORDS,P_IND_4);

  
  // offset[act_ind[i]] = 0, 2, 4, 6.. (base_addr)
  SS_CONFIG_INDIRECT(T64,T64,8);
  SS_INDIRECT(P_IND_1, &offset_array[0], AWORDS, P_IND_2);
  // size[act_ind[i]] = 2 always (sstream_size)
  SS_CONFIG_INDIRECT(T64,T64,8);
  SS_INDIRECT(P_IND_4, &size_array[0], AWORDS, P_IND_3);

  // this should read 3*2 8-byte elements
  SS_CONFIG_INDIRECT(T64,T64,8);
  SS_INDIRECT_2D(P_IND_2, &data_array[0], AWORDS, 8, 8, P_IND_3, P_none_in);

  // for my case, this should be non-determinate
  SS_DMA_WRITE(P_none_out,8,8,2*AWORDS,&output[0]);
  SS_WAIT_ALL();

}

int main(int argc, char* argv[]) {
  init();

  for(int i = 0; i<AWORDS; ++i) {
    ind_array[i]= i; // let it be sequential
    //std::cout << i << "->" << ind_array[i] << "\n";
    offset_array[i]=i*2;
    size_array[i]=2;
  }

  for(int i = 0; i<AWORDS*2; ++i) {
    // ind_ind_array[i]=i;
    data_array[i]=i*2;
    known[i]=data_array[i];
  }

  // kernel();
  begin_roi();
  kernel();
  end_roi();

  compare<uint64_t>(argv[0],output,known,(int)2*AWORDS);
}
