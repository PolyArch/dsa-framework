uint32_t _a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t _a1 = (ops[0]&0xFFFFFFFF00000000)>>32;

float a0=as_float(_a0);
float a1=as_float(_a1);

std::complex<float> res(a0 + a1, 0.0f);

uint64_t c0 = (uint64_t)(as_uint32(res.real()))<<0;
uint64_t c1 = (uint64_t)(as_uint32(res.imag()))<<32;

return c0 | c1;
