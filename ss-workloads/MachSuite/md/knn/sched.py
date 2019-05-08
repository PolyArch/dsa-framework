#!/usr/bin/env python3

import imp

sched = imp.load_source('run', '../../../common/sched.py')

#sched.schedule_and_simulate("knn.log", "md_sb", ["md_sb.dfg"], "$SS_TOOLS/configs/diannao_simd64.sbmodel")
sched.schedule_and_simulate("knn.log", "md_sb", ["md_sb.dfg"], "$SS_TOOLS/configs/diannao_simd64.sbmodel", algs=['sMRT\''], deps=[2], time_cuts=[2400, 3600])
