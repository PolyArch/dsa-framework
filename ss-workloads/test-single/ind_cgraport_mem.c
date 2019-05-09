#include "testing.h"
#include <iostream>

#include "none2.dfg.h"
int main(int argc, char* argv[]) {
  init();

  uint64_t ind_array[AWORDS];
  uint64_t data_array[AWORDS*2];
  uint64_t known[AWORDS],output[AWORDS];

  for(int i = 0; i<AWORDS; ++i) {
    ind_array[i]=i + i/2 + i/5 + i/10;
    //std::cout << i << "->" << ind_array[i] << "\n";
    known[i]=ind_array[i]*2;
  }

  for(int i = 0; i<AWORDS*2; ++i) {
    data_array[i]=i*2;
  }


  begin_roi();
  SS_CONFIG(none2_config,none2_size);

  SS_DMA_READ(&ind_array[0],8,8,AWORDS,P_none2_ind_in);

  SS_CONFIG_INDIRECT(T64,T64,8); //itype, dtype, mult
  SS_INDIRECT(P_none2_ind_out,&data_array[0],AWORDS,P_none2_in);

  SS_DMA_WRITE(P_none2_out,8,8,AWORDS,&output[0]);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],output,known,(int)AWORDS);
}
