uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;

uint64_t c0 = (uint64_t)(a0)<<0;
uint64_t c1 = (uint64_t)(a0 & 0x00000000FFFFFFFF)<<32;

return c0 | c1;
