#!/usr/bin/env python3
import imp

run = imp.load_source('run', '../tools/run.py')

#run.run([12, 32], 'N=%d ', ['origin', 'access', 'new', 'latency'])
run.run([12, 32], 'N=%d ', ['origin', 'new', 'latency'])
