uint32_t t_a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t t_a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t t_b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
uint32_t t_b1 = (ops[1]&0xFFFFFFFF00000000)>>32;

float a0=as_float(t_a0);
float a1=as_float(t_a1);
float b0=as_float(t_b0);
float b1=as_float(t_b1);

std::complex<float> a(a0, a1);
std::complex<float> b(b0, b1);
std::complex<float> c = a * b;

uint64_t c0 = (uint64_t)(as_uint32(c.real()))<<0;
uint64_t c1 = (uint64_t)(as_uint32(c.imag()))<<32;

return c0 | c1;

