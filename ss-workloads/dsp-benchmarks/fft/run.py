#!/usr/bin/env python3
import imp

run = imp.load_source('run', '../tools/run.py')

run.run([64, 1024], 'N=%d ', ['origin', 'new'], 'fft.res')
