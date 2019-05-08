import numpy, cmath, sys, imp
output = imp.load_source('output', '../common/output.py')


numpy.set_printoptions(precision = 4, suppress = True, threshold = 1000, linewidth = 150)

n = int(sys.argv[1])

ideal = seq = 0

_a = numpy.random.rand(n, n) + 1j * numpy.random.rand(n, n)
_a = numpy.zeros((n, n), dtype = 'complex64')
for i in range(n):
    for j in range(n):
        _a[i, j] = ((j - i + n) % n + 1) + 1j * ((j - (n - 1 - i) + n) % n + 1)
_a *= 0.1
a = _a.copy()
output.print_complex_array('input.data', a.flatten())

ans = numpy.linalg.svd(a, compute_uv = False)

output.print_complex_array('ref.data', numpy.concatenate((ans, a.flatten())))

V = numpy.identity(n, dtype = 'complex128')

def upper_div(a, b):
    return (a - 1) / b + 1

vn = 4
vn_red = 3

def household(v):
    global ideal, vec, seq
    #ideal += upper_div(len(v), vn) * vn_red + len(v) - 1 + 36
    w = v.copy()
    normx = numpy.linalg.norm(w)
    s = -w[0] / cmath.sqrt(w[0].conjugate() * w[0])
    u1 = w[0] - s * normx
    w /= u1
    w[0] = 1 + 0j
    alpha = s * normx
    tau = -s.conjugate() * u1 /normx
    return alpha, tau, w

f,d = [],[]
r = a.copy()

for i in range(n - 1):
    #print i
    alpha, tau, w = household(r[:,0].copy())
    #print tau
    #print 'w', w
    r = r[:,1:] - tau * numpy.outer(w, numpy.dot(numpy.conj(w), r[:,1:]))
    #print r[1:,:]
    d.append(alpha)
    if i != n - 2:
        alpha, tau, w = household(r[0,:].copy())
        #print 'w\'', w
        r = r[1:,:] - tau * numpy.outer(numpy.dot(r[1:,:], numpy.conj(w)), w)
        V[i+1:,1:] = V[i+1:,1:] - tau * numpy.outer(numpy.conj(w), numpy.dot(w, V[i+1:,1:]))
        f.append(alpha)
    #print r

#print V

d.append(r[1,0])
f.append(r[0,0])

d = numpy.array(d)
f = numpy.array(f)

print(f)
print(d)

implicit_log = []

def givens(a, b):
    r = cmath.sqrt(a.conjugate() * a + b.conjugate() * b)
    c = a.conjugate() / r
    s = b.conjugate() / r
    return (r, c, s)

def implicit_kernel(d, f, V):
    n = len(d)
    assert n > 1
    global seq, ideal

    mu = d[-1].conjugate() * d[-1]
    alpha, c, s = givens(d[0] * d[0].conjugate() - mu, d[0] * f[0].conjugate())

    d[0], f[0], extra, d[1] = d[0] * c + f[0] * s.conjugate(), d[0] * s - f[0] * c.conjugate(), d[1] * s.conjugate(), d[1] * -c.conjugate()
    V[:2,:] = numpy.dot([[c, s], [s.conjugate(), -c.conjugate()]], V[:2,:])

    alpha, c, s = givens(d[0], extra)
    d[0] = alpha
    f[0], d[1] = c * f[0] + s * d[1], s.conjugate() * f[0] - c.conjugate() * d[1]
    if n != 2:
        extra = s * f[1]
        f[1]  = -c.conjugate() * f[1]
        #print "f[0]:", f[0]
        #print "extra:", extra
        #print "d[1]:", d[1]
        #print "f[1]:", f[1], '\n'
    else:
        pass
        #print "[FIN]", f[0], d[1]

    for i in range(1, n - 1):
        alpha, c, s = givens(f[i - 1], extra)

        f[i-1] = alpha
        d[i], f[i] = d[i] * c + f[i] * s, d[i] * s.conjugate() - f[i] * c.conjugate()
        extra  = d[i+1] * s
        d[i+1] = d[i+1] * -c.conjugate()
        V[i:i+2,:] = numpy.dot([[c, s.conjugate()], [s, -c.conjugate()]], V[i:i+2,:])

        #print '[FIN]', alpha
        alpha, c, s = givens(d[i], extra)
        #print '[FIN]', alpha, '\n'

        d[i]   = alpha
        f[i], d[i+1] = c * f[i] + s * d[i+1], s.conjugate() * f[i] - c.conjugate() * d[i+1]
        #print 'cos', c
        #print 'sin', s
        #print 'f[i]', f[i]
        #print 'd[i+1]', d[i+1], '\n'
        if i != n - 2:
            extra  = s * f[i+1]
            f[i+1] = -c.conjugate() * f[i+1]
            #print "f[i]:", f[i]
            #print "extra:", extra
            #print "d[i+1]:", d[i+1]
            #print "f[i+1]:", f[i+1], '\n'
        else:
            pass
            #print "[FIN]", f[i], d[i+1]
    print(f)
    print(d)
    print('=====================================')

l, r = 0, n - 1
while l < r:
    while l < n - 1 and abs(f[l].real) < 1e-5 and abs(f[l].imag) < 1e-5:
        l += 1
    while r >= 1 and abs(f[r - 1].real) < 1e-5 and abs(f[r - 1].imag) < 1e-5:
        r -= 1
    if r - l >= 1:
        implicit_kernel(d[l:r+1], f[l:r], V[l:r+1,:])
        implicit_log.append((l, r))
        ideal += (r - l)

#while True:
#    i = 0
#    called = False
#    while i < n - 1:
#        j = i
#        while j < n - 1 and (abs(f[j].real) > 1e-5 or abs(f[j].imag) > 1e-5):
#            j += 1
#        if i != j:
#            implicit_kernel(d[i:j+1], f[i:j], V[i:j+1,:])
#            implicit_log.append((i, j))
#            ideal += 14 * (j - i + 1)
#            ideal += n * (j - i + 1)
#            called = True
#        i = j + 1
#    implicit_log.append('=====')
#    if not called:
#        break


""" check pass!
invsd = numpy.dot(a, V)
numpy.testing.assert_allclose(
    numpy.dot(numpy.conj(invsd.transpose()), invsd),
    ata,
    atol = 1e-5,
    rtol = 1e-5
)
"""

print("Total iteration: %d" % sum(i != '=====' for i in implicit_log))
print('\n'.join(str(i) for i in implicit_log))
sv = d
print(d)
sv = numpy.real(numpy.sqrt(sv * numpy.conj(sv)))

try:
    numpy.testing.assert_allclose(numpy.sort(sv)[::-1], ans, atol = 1e-5, rtol = 1e-5)
    print("Check pass!")
except:
    print("ERROR: SV not computed correctly")
    print(sv)
    print(ans)
    quit()

print('Singular value:\n', sv)

print(V)
U = numpy.dot(_a, numpy.conj(V).transpose())
for i in range(n):
    U[:,i] /= sv[i]


""" check pass!
numpy.testing.assert_allclose(
    numpy.dot(numpy.conj(U).transpose(), U),
    numpy.identity(n, dtype = 'complex128'),
    atol = 1e-5, rtol = 1e-5
)
"""

#verify code:

sigma = numpy.zeros((n, n), dtype = 'complex128')
for i in range(n):
    sigma[i, i] = sv[i]

try:
    numpy.testing.assert_allclose(
        numpy.dot(numpy.dot(U, sigma), V),
        _a,
        atol = 1e-4, rtol = 1e-4
    )
except:
    print('WARN: Precision loss too much!')

print('AVG ERROR: %.6f' % (abs(numpy.dot(numpy.dot(U, sigma), V) - _a).sum() / (n * n)))
print('ASIC Ideal:', ideal)
print('ASIC Latency:', ideal)
print('Sequential Ideal', seq)

