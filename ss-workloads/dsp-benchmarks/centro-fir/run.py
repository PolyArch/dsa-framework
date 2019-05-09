#!/usr/bin/env python3
import imp

run = imp.load_source('run', '../tools/run.py')

run.run([37, 199], 'M=%d ', ['origin', 'new', 'latency'], 'fir.res')
#run.run([(164, 37), (326, 199)], 'N=%d M=%d ', ['origin'])
