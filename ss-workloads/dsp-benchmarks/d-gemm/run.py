#!/usr/bin/env python3
import imp

run = imp.load_source('run', '../tools/run.py')

run.run([12, 48], 'N=%d M=16 P=64 ', ['origin', 'new', 'latency'])
#run.run([96], 'N=64 M=16 P=%d ', ['origin', 'new', 'latency'], False, False)
run.run([2, 3, 6], 'N=%d M=16 P=64 ', ['origin'])
#run.run([8], 'N=%d M=16 P=96 ', ['origin'], True, False)
