uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t a1 = (ops[1]&0x00000000FFFFFFFF)>>0;

uint64_t c0 = (uint64_t)(a0)<<32;
uint64_t c1 = (uint64_t)(a1)<<0;

return c0 | c1;

