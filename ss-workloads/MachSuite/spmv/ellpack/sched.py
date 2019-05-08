#!/usr/bin/env python3

import imp

sched = imp.load_source('run', '../../../common/sched.py')

sched.schedule_and_simulate("ellpack.log", "spmv_sb", ["ellpack.dfg"], "$SS_TOOLS/configs/diannao_simd64.sbmodel")
