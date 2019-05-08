#!/usr/bin/env python3

import imp

sched = imp.load_source('run', '../../../common/sched.py')

sched.schedule_and_simulate("crs.log", "spmv_sb", ["mm_lanes.dfg"], "$SS_TOOLS/configs/diannao_simd64.sbmodel")
