#!/usr/bin/env python3

import os, sys, imp

sched = imp.load_source('run', '../../common/sched.py')

#sched.schedule_and_simulate('diannao.log', 'pool1sb', ['pool2x2l4avg.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel')
#sched.schedule_and_simulate('diannao.log', 'pool3sb', ['pool4x4l2avg.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel')
#sched.schedule_and_simulate('diannao.log', 'conv4sb', ['red16to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel')
#sched.schedule_and_simulate('diannao.log', 'conv5sb', ['red32to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel')
#sched.schedule_and_simulate('diannao.log', 'conv2sb', ['red8to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel')
#sched.schedule_and_simulate('diannao.log', 'conv4sb', ['red16to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel', deps=[2], algs=['sMRT'])
#sched.schedule_and_simulate('diannao.log', 'conv5sb', ['red32to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel', deps=[2], algs=['sMRT'])
#sched.schedule_and_simulate('diannao.log', 'pool1sb', ['pool2x2l4avg.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel', deps[2, 3, 7, 15], time_cuts=[2400, 3600])

#sched.schedule_and_simulate('diannao.log', 'conv4sb', ['red16to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel', deps=[2], time_cuts=[2400], algs=['sMRT'])
sched.schedule_and_simulate('diannao.log', 'conv5sb', ['red32to1sig.dfg'], '$SS_TOOLS/configs/diannao_simd64.sbmodel', deps=[2], time_cuts=[2400], algs=['sMRT'])
