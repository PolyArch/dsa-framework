
complex<float> a(as_float_complex(ops[0])), b(as_float_complex(ops[1])), c(0.);

c = sqrt(
    a.real() * a.real() + a.imag() * a.imag() +
    b.real() * b.real() + b.imag() * b.imag()
);

c = std::conj(a) / c;

uint64_t res;

memcpy(&res, &c, sizeof res);

return res;
