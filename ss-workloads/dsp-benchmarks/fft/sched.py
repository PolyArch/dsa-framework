#!/usr/bin/env python3

import imp

sched = imp.load_source('run', '../../common/sched.py')

#sched.schedule_and_simulate("fft.log", "sb-new.exe", ["compute.dfg", "fine1.dfg", "fine2.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs = ["MR.RT", "sMR.RT", "MR'.RT"])
#sched.schedule_and_simulate("fft.log", "sb-new.exe", ["compute.dfg", "fine1.dfg", "fine2.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs=["sMRT", "sMR.RT"], time_cuts=[2400, 3600], deps=[7])

#sched.schedule_and_simulate("fft.log", "sb-new.exe", ["compute.dfg", "fine2.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs=["sMR.RT"], time_cuts=[1200], deps=[3], sim=False)
#sched.schedule_and_simulate("fft.log", "sb-new.exe", ["fine1.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs=["sMR.RT"], time_cuts=[2400], deps=[3], sim=False)

sched.schedule_and_simulate("fft.log", "sb-new.exe", ["fine1.dfg"], "$SS_TOOLS/configs/revel.sbmodel", algs=["MR.RT"], time_cuts=[1200], deps=[15], sim=False)
