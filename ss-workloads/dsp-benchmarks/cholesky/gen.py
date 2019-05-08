import imp
output = imp.load_source('output', '../common/output.py')

def round(a):
    return a.real.astype('int16') + a.imag.astype('int16') * 1j


# A quick POC (proof of concept) of Cholesky Decomposition in numpy.
import numpy, cmath, sys
n = int(sys.argv[1])
a = numpy.random.rand(n, n) + 1j * numpy.random.rand(n, n)
a = (numpy.dot(a, numpy.conj(a.transpose())))

output.print_complex_array('input.data', a.flatten())
print("%d x %d Input generated!" % (n, n))

L = numpy.zeros((n, n)).astype('complex64')

origin = a.copy()

simd = 0
starting = 0
finish   = 0
compute  = 0

for i in range(n):
    l = numpy.identity(n).astype('complex')
    div = cmath.sqrt(a[i, i])
    l[i, i] = div
    b = a[i, i + 1:]
    l[i + 1:, i] = b / l[i, i]

    sub = n - i
    simd += sub // 4 + sub % 4
    simd += sum(j // 4 + j % 4 for j in range(1, sub)) * 4

    compute += (sub - 1) * (sub - 2) // 2
    starting += sub + 12
    finish  = max(finish, starting + (sub - 1) * (sub - 2) // 2)

    aa = a.copy()
    aa[i, i] = 1
    aa[i, i + 1:] = numpy.zeros(n - i - 1)
    aa[i + 1:, i] = numpy.zeros(n - i - 1)
    aa[i + 1:, i + 1:] = a[i + 1:, i + 1:] - numpy.outer(numpy.conj(b), b) / a[i, i]
    # Mathematically, it is L = L * l. However, in this case, we can just copy the corresponding
    # column to L, because the speciality of L and l
    L[i:, i] = l[i:, i]
    #L = numpy.dot(L, l)
    a = aa

numpy.testing.assert_allclose(origin, numpy.dot(numpy.conj(L), L.transpose()), rtol = 1e-4)
print("Correctness check pass!")

output.print_complex_array('ref.data', L.flatten())
print("New data generated!")

init = 12 * n
print('ASIC Ideal:', init + compute)
print('ASIC Latency:', finish)
print('SIMD Ideal:', init + simd)
