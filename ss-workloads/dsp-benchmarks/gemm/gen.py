import sys, numpy, imp
from math import sin, cos, pi 
output = imp.load_source('output', '../common/output.py')

n = int(sys.argv[1])
m = int(sys.argv[2])
p = int(sys.argv[3])
#numpy.set_printoptions(suppress = True, precision = 4., linewidth = 180, threshold = numpy.nan)

a = numpy.random.rand(n, m).astype('complex64') + 1j * numpy.random.rand(n, m).astype('complex64')
b = numpy.random.rand(m, p).astype('complex64') + 1j * numpy.random.rand(m, p).astype('complex64')
output.print_complex_array('input.data', numpy.concatenate((a.flatten(), b.flatten())))
print('input generated')

c = numpy.dot(a, b)
output.print_complex_array('ref.data', c.flatten())
print('output generated!')

print('ASIC Ideal:', (n * m * p - 1) // 8 + 1)
print('ASIC Latency:', ((n // 8) * m * p - 1) // 8 + 1 + 1)
print('VM Ideal:', (n * m * p - 1) // 8 * 4)
