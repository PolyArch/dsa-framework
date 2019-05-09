#include "testing.h"
#include "add1.dfg.h"

static DTYPE add1[ASIZE]; 
static DTYPE answer_add1[ASIZE]; 

static DTYPE out2[ASIZE]; 


int main(int argc, char* argv[]) {
  init();

  for(int i=0; i < ASIZE; ++i) {
    answer_add1[i]=i+1;
  }

  begin_roi();

  //Description: Shared Core loads from memory into shared scratchpad.
  //Then, two accelerator instances 
  //  read from shread scratchpad to local
  //  scratchpad write barrier
  //  read from scratch and write to memory

  SS_CONTEXT(0x1);

  SS_CONTEXT(SHARED_SP);
  SS_DMA_SCRATCH_LOAD(&in[0],ABYTES,ABYTES,1,0);
  SS_WAIT_SCR_WR();

  SS_CONTEXT(0x0001);
  SS_CONFIG(add1_config,add1_size);
  SS_SCRATCH_LOAD_REMOTE(0,ABYTES,ABYTES,0,1,0); //stretch parameter added
  SS_WAIT_SCR_WR();
  SS_SCRATCH_READ(0, ABYTES, P_add1_in);
  SS_DMA_WRITE(P_add1_out,8,8,AWORDS,&out2[0]);

  SS_CONTEXT(0x0002);
  SS_CONFIG(none_config,none_size);
  SS_SCRATCH_LOAD_REMOTE(0,ABYTES,ABYTES,0,1,0);
  SS_WAIT_SCR_WR();
  SS_SCRATCH_READ(0, ABYTES, P_none_in);
  SS_DMA_WRITE(P_none_out,8,8,AWORDS,&out[0]);

  SS_CONTEXT(SHARED_SP|0x3);
  SS_WAIT_ALL();

  end_roi();

  compare<DTYPE>(argv[0],out2,answer_add1,ASIZE);
 // compare<DTYPE>(argv[0],out,in,ASIZE);

}
