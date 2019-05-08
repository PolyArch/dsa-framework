#!/usr/bin/env python3

import imp

sched = imp.load_source('run', '../../../common/sched.py')

sched.schedule_and_simulate("stencil2d.log", "stencil_sb", ["stencil_sb.dfg"], "$SS_TOOLS/configs/diannao_simd64.sbmodel")
