#include <stdio.h>
#include "none.dfg.h"
#include "none16.dfg.h"
#include "check.h"
#include "ss_insts.h"
#include "../common/include/sim_timing.h"
#include <inttypes.h>
#include <stdlib.h>


//top level should define ASIZE

#define DTYPE  uint16_t
#define ABYTES (sizeof(DTYPE)*ASIZE)
#define AWORDS (ABYTES/8)

#define A2WORDS (AWORDS/16)*8

static DTYPE out[ASIZE]; 
static DTYPE in[ASIZE];

void finalfunc() {
  sb_stats();
}

void init() {
  for(int i = 0; i < ASIZE; ++i) {
    in[i] = i;
    out[i] = 0;
  }

  atexit(finalfunc);
}
