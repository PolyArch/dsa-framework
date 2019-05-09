//Dummy instruction for HouseHolder vectors intermediate variables

uint32_t t_a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t t_a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t t_b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
uint32_t t_b1 = (ops[1]&0xFFFFFFFF00000000)>>32;
uint64_t inst = ops[2];

float a0=as_float(t_a0);
float a1=as_float(t_a1);
float b0=as_float(t_b0);
float b1=as_float(t_b1);

float normx(a0 + a1);
std::complex<float> head(b0, b1);
normx += head.real() * head.real();
normx += head.imag() * head.imag();
normx = sqrt(normx);

//printf("normx: %.5f\n", normx);

std::complex<float> s = -head / ((float)sqrt(head.real() * head.real() + head.imag() * head.imag()));
std::complex<float> alpha = s * normx;

//printf("s: (%.5f, %.5f)\n", s.real(), s.imag());

if (inst == 0) {
  uint64_t c0 = (uint64_t)(as_uint32(alpha.real()))<<0;
  uint64_t c1 = (uint64_t)(as_uint32(alpha.imag()))<<32;
  return c0 | c1;
}

std::complex<float> u1 = 1.0f / (head - s * normx);

if (inst == 1) {
  u1 = std::conj(u1);
  uint64_t c0 = (uint64_t)(as_uint32(u1.real()))<<0;
  uint64_t c1 = (uint64_t)(as_uint32(u1.imag()))<<32;
  return c0 | c1;
}

std::complex<float> tau = -std::conj(s) / u1 / normx;

if (inst == 2) {
  uint64_t c0 = (uint64_t)(as_uint32(tau.real()))<<0;
  uint64_t c1 = (uint64_t)(as_uint32(tau.imag()))<<32;
  return c0 | c1;
}

