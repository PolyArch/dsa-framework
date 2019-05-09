uint32_t _a0 = (ops[0]&0x00000000FFFFFFFF)>>0;

float a0=as_float(_a0);

float res = 1.0 / a0;

uint64_t c0 = ((uint64_t)(as_uint32(res)))<<0;
uint64_t c1 = ((uint64_t)(as_uint32(res)))<<32;

return c0 | c1;
