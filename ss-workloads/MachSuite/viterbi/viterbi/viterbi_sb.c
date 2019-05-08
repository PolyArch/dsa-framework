#include "viterbi.h"
#include "viterbi_sb.dfg.h"
#include "../../../common/include/ss_insts.h"
#include "../../../common/include/sim_timing.h"

int viterbi( tok_t obs[N_OBS], prob_t init[N_STATES], prob_t transition[N_STATES*N_STATES], prob_t emission[N_STATES*N_TOKENS], state_t path[N_OBS] )
{
  end_roi();
  prob_t llike[N_OBS][N_STATES];
  step_t t;
  state_t prev, curr;
  prob_t min_p, p;
  state_t min_s, s;
  // All probabilities are in -log space. (i.e.: P(x) => -log(P(x)) )
 
  // Initialize with first observation and initial probabilities
  L_init: for( s=0; s<N_STATES; s++ ) {
    llike[0][s] = init[s] + emission[s*N_TOKENS+obs[0]];
    L_timestep_fake: for( t=1; t<N_OBS; t++ ) { //TONY ADDED B/C OF SIMULATOR TROUBLE
      llike[t][s] = 0; }
  }

  //TONY ADDED BECAUSE MACHSUITE IS BAD
  prob_t transition_inv[N_STATES*N_STATES];
  for( int i=0; i<N_STATES; i++ ) {
    for( int j=0; j<N_STATES; j+=20 ) {
      transition_inv[i*N_STATES+j] = transition[i+N_STATES*j];
    }
  }

  printf("N_OBS: %d, N_STATES %d\n", N_OBS, N_STATES);

  begin_roi();
  SS_CONFIG(viterbi_sb_config,viterbi_sb_size);

  // Iteratively compute the probabilities over time
  L_timestep: for( t=1; t<N_OBS; t++ ) {
    //SS_WAIT_SCR_RD();
    //SS_DMA_SCRATCH_LOAD(&llike[t-1][0], 0, N_STATES * sizeof(TYPE), 1, 0);
    //SS_WAIT_SCR_WR();
    //SS_SCR_PORT_STREAM(0, 0, sizeof(TYPE)*N_STATES, N_STATES, P_viterbi_sb_llike);

    L_curr_state: for( curr=0; curr<N_STATES; curr++ ) {
      // Compute likelihood HMM is in current state and where it came from.
      SS_DMA_READ(&transition_inv[0+N_STATES*curr],8,8,N_STATES,P_viterbi_sb_trans);
      SS_DMA_READ(&llike[t-1][0],8,8,N_STATES,P_viterbi_sb_llike);

      SS_CONST(P_viterbi_sb_emission,emission[curr*N_TOKENS+obs[t]],N_STATES/4);
      SS_CONST(P_viterbi_sb_reset,0,N_STATES/4-1);
      SS_CONST(P_viterbi_sb_reset,1,1);
      SS_GARBAGE(P_viterbi_sb_MR,N_STATES/4-1);
      SS_DMA_WRITE(P_viterbi_sb_MR,8,8,1,&llike[t][curr]);

      //min_p = llike[t-1][0] +
      //        transition_inv[0+N_STATES*curr] +0;

      //L_prev_state: for( prev=1; prev<N_STATES; prev++ ) {
      //  p = llike[t-1][prev] +
      //      transition_inv[prev+N_STATES*curr] +
      //      0;
      //  if( p<min_p ) {
      //    min_p = p;
      //  }
      //}
                    
      //llike[t][curr] = min_p + emission[curr*N_TOKENS+obs[t]];
    }
    SS_WAIT_ALL();
  }
  SS_WAIT_ALL();
  end_roi();


  // Identify end state
  min_s = 0;
  min_p = llike[N_OBS-1][min_s];
  L_end: for( s=1; s<N_STATES; s++ ) {
    p = llike[N_OBS-1][s];
    if( p<min_p ) {
      min_p = p;
      min_s = s;
    }
  }
  path[N_OBS-1] = min_s;

  // Backtrack to recover full path
  L_backtrack: for( t=N_OBS-2; t>=0; t-- ) {
    min_s = 0;
    min_p = llike[t][min_s] + transition_inv[min_s+N_STATES*path[t+1]];
    L_state: for( s=1; s<N_STATES; s++ ) {
      p = llike[t][s] + transition_inv[s+N_STATES*path[t+1]];
      if( p<min_p ) {
        min_p = p;
        min_s = s;
      }
    }
    path[t] = min_s;
  }

  begin_roi();
  return 0;
}

