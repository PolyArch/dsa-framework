import sys, imp, numpy
from math import sin, cos, pi , log
output = imp.load_source('output', '../common/output.py')

n = int(sys.argv[1])

_a = numpy.random.rand(n).astype('complex64') + 1j * numpy.random.rand(n).astype('complex64')
#_a = numpy.array(range(1, n + 1)).astype('complex64')

output.print_complex_array('input.data', _a.flatten())

print('%d-length input generated' % n)
a = _a.copy()

def brute_force(a):
    n = len(a)
    res = []
    for i in range(n):
        w = cos(2 * pi * i / n) + 1j * sin(2 * pi * i / n)
        cur = 0
        for j in range(n):
            cur += w ** j * a[j]
        res.append(cur)
    return numpy.array(res)

def non_recursive(_a):
    a = _a.copy()
    n = len(_a)
    blocks = n // 2
    w = numpy.array([cos(2 * pi * i / n) + 1j * sin(2 * pi * i / n) for i in range(n // 2)])
    while blocks:
        span = n // blocks
        dup = a.copy()
        for j in range(0, span // 2 * blocks, blocks):
            for i in range(0, blocks):
                L, R = dup[2 * j + i], dup[2 * j + i + blocks]
                a[i + j] = L + w[j] * R
                a[i + j + span // 2 * blocks] = L - w[j] * R
        blocks //= 2
    return a

#numpy.testing.assert_allclose(non_recursive(a), brute_force(a), atol = 1e-4)
#print 'check pass!'

output.print_complex_array('ref.data', non_recursive(a).flatten())
print('output generated!')
print('ASIC Ideal:', int(log(n) / log(2)) * (n // 8 + 2))
print('ASIC Latency:', int(log(n) / log(2)) * ((n - 1) // 8 + 3))
