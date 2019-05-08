#include "testing.h"
#include "add1.dfg.h"

DTYPE add1[ASIZE]; 
DTYPE answer_add1[ASIZE * 4]; 

DTYPE out2[ASIZE]; 


int main(int argc, char* argv[]) {
  if(AWORDS&1) {
    return 0;
  }

  init();

  for(int i=0; i < AWORDS; ++i) {
    for(int j=0; j < 4; ++j) {
      answer_add1[i*4+j]=(i/2)*4+j+1;
    }
  }
  
  SS_CONTEXT(0x0001);
  SS_CONFIG(add1_config,add1_size);
  
  SS_CONTEXT(0x0002);
  SS_CONFIG(none_config,none_size);
  
  begin_roi();
  
  
  
  SS_CONTEXT(0x0002);
  SS_DMA_READ(&in[0],ABYTES/2,ABYTES/2,1,P_none_in);
  
  SS_REPEAT_PORT(2);
  SS_XFER_LEFT(P_none_out,P_add1_in,AWORDS/2);
  
  
  SS_CONTEXT(0x0001);
  SS_DMA_WRITE(P_add1_out,8,8,AWORDS,&out2[0]);
  
  SS_CONTEXT(0x0003);
  SS_WAIT_ALL();
  
  end_roi();
  
  compare<DTYPE>(argv[0],out2,answer_add1,ASIZE);
}
