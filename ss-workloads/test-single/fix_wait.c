#include <stdio.h>
#include "none.dfg.h"
#include "check.h"
#include "ss_insts.h"
#include "../common/include/sim_timing.h"
#include <inttypes.h>

int main(int argc, char* argv[]) {

  begin_roi();
  SS_WAIT_ALL(); 
  end_roi();
}
