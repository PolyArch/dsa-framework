#include "testing.h"
#include "add.dfg.h"

uint64_t input[AWORDS];

// Acces Pattern on 1D Array
// 1 2 3 4 5 6 7 8
// - - - - - - - -
//   - - - - - - - 
//     - - - - - -
//       - - - - -
//         - - - -
//           - - -
//             - -
//               -

int main(int argc, char* argv[]) {
  init();

  uint64_t answer=0;
  uint64_t my_answer=0;
  int iters=0;
  for(int i = 1; i <= AWORDS; ++i) {
    input[i-1]=i;
    iters+=i;
  }
  for (int i = 0; i < AWORDS; ++i)
    for (int j = i; j < AWORDS; ++j)
      answer += input[j];

  //Do the triangle sum
  begin_roi();
  SS_CONFIG(add_config,add_size);
  SS_CONST(P_add_in2,0,1);
  SS_DMA_READ_STRETCH(&input[0],8,AWORDS*8,-8,AWORDS,P_add_in1);
  SS_RECURRENCE(P_add_out,P_add_in2,iters-1);
  SS_DMA_WRITE(P_add_out,8,8,1,&my_answer);
  SS_WAIT_ALL();
  end_roi();

  compare<uint64_t>(argv[0],&answer,&my_answer,1);
}
