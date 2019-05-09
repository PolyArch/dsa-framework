int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
int16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
int16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
int16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
int16_t b3 = (ops[1]&0xFFFF000000000000)>>48;

a0 = FIX_MUL(a0, b0);
a1 = FIX_MUL(a1, b1);
a2 = FIX_MUL(a2, b2);
a3 = FIX_MUL(a3, b3);

uint64_t c0 = (uint64_t)(a0 >= 0 ? a0 : (~(-a0)+1)&0x000000000000FFFF)<<0;
uint64_t c1 = (uint64_t)(a1 >= 0 ? a1 : (~(-a1)+1)&0x000000000000FFFF)<<16;
uint64_t c2 = (uint64_t)(a2 >= 0 ? a2 : (~(-a2)+1)&0x000000000000FFFF)<<32;
uint64_t c3 = (uint64_t)(a3 >= 0 ? a3 : (~(-a3)+1)&0x000000000000FFFF)<<48;

return c0 | c1 | c2 | c3;
