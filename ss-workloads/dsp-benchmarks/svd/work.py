#!/usr/bin/env python3
import imp

run = imp.load_source('run', '../tools/run.py')

print(run.parse_log('sb-new.log'))

