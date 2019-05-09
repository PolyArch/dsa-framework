complex<float> val(as_float_complex(ops[0]));
uint16_t real = val.real() * (1 << FRAC_BITS);
uint16_t imag = val.imag() * (1 << FRAC_BITS);

uint16_t a0(real);
uint16_t a1(imag);
uint16_t a2(real);
uint16_t a3(imag);

uint64_t c0 = (uint64_t)(a0 >= 0 ? a0 : (~(-a0)+1)&0x000000000000FFFF)<<0;
uint64_t c1 = (uint64_t)(a1 >= 0 ? a1 : (~(-a1)+1)&0x000000000000FFFF)<<16;
uint64_t c2 = (uint64_t)(a2 >= 0 ? a2 : (~(-a2)+1)&0x000000000000FFFF)<<32;
uint64_t c3 = (uint64_t)(a3 >= 0 ? a3 : (~(-a3)+1)&0x000000000000FFFF)<<48;

return c0 | c1 | c2 | c3;
