import imp
output = imp.load_source('output', '../common/output.py')

# A quick POC (proof of concept) of Cholesky Decomposition in numpy.
import numpy, cmath, sys
n = int(sys.argv[1])
a = numpy.random.rand(n, n) + 1j * numpy.random.rand(n, n)
a = a + numpy.identity(n)

origin = a.copy()

output.print_complex_array('input.data', a.flatten())
print("%d x %d Input generated!" % (n, n))

tau = numpy.zeros((n - 1, ), dtype = 'complex128')
q = numpy.identity(n, dtype = 'complex128')

for i in range(n - 1):
    w = a[i:,i].copy()
    normx = numpy.linalg.norm(w)
    s = -w[0] / cmath.sqrt(w[0].conjugate() * w[0])
    u1 = w[0] - s * normx
    w /= u1
    w[0] = 1 + 0j
    a[i, i] = s * normx
    #a[i+1:,i] = w[1:]
    a[i+1:,i] = numpy.zeros(n - i - 1)
    tau[i] = -s.conjugate() * u1 / normx
    print('norm', normx)
    print('alpha', a[i, i])
    print('u1inv', 1.0 / u1)
    print('tau', tau[i])
    print()

    v = tau[i] * numpy.dot(numpy.conj(w), a[i:,i+1:])
    a[i:,i+1:] -= numpy.outer(w, v)

    v = numpy.dot(q[:,i:], w)
    q[:,i:] -= tau[i] * numpy.outer(v, numpy.conj(w))

#for i in xrange(n - 2, -1, -1):
#    w = a[i+1:,i]
#    v = numpy.dot(numpy.conj(w), q[i+1:,i+1:])
#    q[i,i] = 1 - tau[i]
#    q[i,i+1:] = -tau[i] * v
#    q[i+1:,i+1:] -= tau[i] * numpy.outer(w, v)
#    q[i+1:,i] = a[i+1:,i] * -tau[i]

#print a
#print numpy.dot(numpy.conj(q.transpose()), origin)
output.print_complex_array('ref.data', numpy.concatenate((a.flatten(), tau, q.flatten())))

print("New data generated!")

ideal = 0
simd  = 0
last_h = 0
for i in range(n - 1):
    r_kernel = n - i + (n - i) * n
    simd    += ((n - i) // 4 + (n - i) % 4) * n * 5
    q_kernel = n - i + (n - i) * (n - i)
    simd    += ((n - i) // 4 + (n - i) % 4) * (n - i) * 5
    if i == 0:
        ideal += (n - i) + (n - i - 1) + 40
    else:
        ideal += max(0, 5 + (n - i) * 2 + (n - i - 1) + 40 - diff)
    ideal += max(r_kernel, q_kernel)
    diff   = abs(r_kernel - q_kernel)
    simd  += ((n - i) // 4 + (n - i) % 4) * 3 + 40

ideal += diff

print('ASIC Latency:', ideal)
print('ASIC Ideal:', ideal)
print('SIMD Ideal:', simd)
