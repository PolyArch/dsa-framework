#!/usr/bin/env python3
import imp

run = imp.load_source('run', '../tools/run.py')

run.run([12, 48], 'N=%d M=16 P=64 ', ['origin', 'new', 'latency'], log='gemm.res')

