int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
int16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
int16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
int16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
int16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
/*
printf("FxAdd16x4 A: %f %f %f %f\n", FIX_TO_DOUBLE(a0), FIX_TO_DOUBLE(a1), FIX_TO_DOUBLE(a2), FIX_TO_DOUBLE(a3));
for (int i = 0; i < 64; ++i) putchar(ops[0] >> i & 1 ? '0' : '1'); puts("");
printf("FxAdd16x4 B: %f %f %f %f\n", FIX_TO_DOUBLE(b0), FIX_TO_DOUBLE(b1), FIX_TO_DOUBLE(b2), FIX_TO_DOUBLE(b3));
for (int i = 0; i < 64; ++i) putchar(ops[1] >> i & 1 ? '0' : '1'); puts("");
*/
a0 = FIX_ADD(a0, b0);
a1 = FIX_ADD(a1, b1);
a2 = FIX_ADD(a2, b2);
a3 = FIX_ADD(a3, b3);

uint64_t c0 = (uint64_t)(a0 >= 0 ? a0 : (~(-a0)+1)&0x000000000000FFFF)<<0;
uint64_t c1 = (uint64_t)(a1 >= 0 ? a1 : (~(-a1)+1)&0x000000000000FFFF)<<16;
uint64_t c2 = (uint64_t)(a2 >= 0 ? a2 : (~(-a2)+1)&0x000000000000FFFF)<<32;
uint64_t c3 = (uint64_t)(a3 >= 0 ? a3 : (~(-a3)+1)&0x000000000000FFFF)<<48;

uint64_t res = c0 | c1 | c2 | c3;

/*
a0 = (res&0x000000000000FFFF)>>0;
a1 = (res&0x00000000FFFF0000)>>16;
a2 = (res&0x0000FFFF00000000)>>32;
a3 = (res&0xFFFF000000000000)>>48;
printf("FxAdd16x4 res: %f %f %f %f\n", FIX_TO_DOUBLE(a0), FIX_TO_DOUBLE(a1), FIX_TO_DOUBLE(a2), FIX_TO_DOUBLE(a3));
for (int i = 0; i < 64; ++i) putchar(res >> i & 1 ? '0' : '1'); puts("");
*/

return res;
