#!/usr/bin/env python3

import sys, numpy, random, imp
from math import sin, cos, pi 
output = imp.load_source('output', '../common/output.py')

n = int(sys.argv[1])
m = int(sys.argv[2])

if not (m % 2):
    exit()

numpy.set_printoptions(suppress = True, precision = 4., linewidth = 180, threshold = numpy.nan)

a = numpy.random.rand(n).astype('complex64') + 1j * numpy.random.rand(n).astype('complex64')
b = numpy.random.rand(m // 2).astype('complex64') + 1j * numpy.random.rand(m // 2).astype('complex64')
b = numpy.concatenate((b, numpy.array([random.random() + 1j * random.random()]), b[::-1]))
#a = numpy.array(range(1, n + 1)).astype('complex64')
#b = numpy.array(range(1, m / 2 + 1)).astype('complex64')
#b = numpy.concatenate((b, numpy.array([float(m / 2 + 1) + 0j]), b[::-1]))

output.print_complex_array('input.data', numpy.concatenate((a.flatten(), b.flatten())))
print('input generated')

c = numpy.zeros((n - m + 1,)).astype('complex64')

for i in range(n - m + 1):
    for j in range(m):
        c[i] += a[i + j] * b[j]


print('ASIC Ideal: %d' % (((m / 2) * (n - m + 1) - 1) / 4 + 1 + (n - m) / 4 + 1))
print('ASIC Latency: %d', (((m / 2 - 1) / 4 + 1) * ((n - m) / 8 + 1)  + (n - m) / 4 + 1))
print('SIMD Ideal: %d', ((((m / 2) / 4 + m / 2 % 4) * (n - m + 1) + (n - m + 1) / 4) * 4))

output.print_complex_array('ref.data', c.flatten())
print('output generated!')
