#!/usr/bin/env python3

import subprocess, sys

if len(sys.argv) != 2:
    print('Usage: ./time.py command')
    print('Command should print "ticks: x" to stdout!')
else:
    total = 0
    for i in range(10):
        total += int(subprocess.check_output(sys.argv[1:]).decode('utf-8').lstrip('ticks: ').rstrip())
    print(total / 10.)
