#!/usr/bin/env python3

import imp

sched = imp.load_source('run', '../../common/sched.py')

#sched.schedule_and_simulate("gemm.log", "sb-new.exe", ["compute.dfg"], "$SS_TOOLS/configs/revel.sbmodel")
#sched.schedule_and_simulate("gemm.log", "sb-new.exe", ["compute.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs=['sMRT', 'sMR.RT'], deps=[3, 2])
sched.schedule_and_simulate("gemm.log", "sb-new.exe", ["compute.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs=["sMRT"], deps=[3, 2], time_cuts=[2400, 3600])
