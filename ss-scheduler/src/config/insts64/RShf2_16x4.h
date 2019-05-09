uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;

uint64_t b = ops[1];
if(ops.size()==1) {
  b = 2;
}
uint64_t c0 = (uint64_t)(a0>>b)<<0;
uint64_t c1 = (uint64_t)(a1>>b)<<16;
uint64_t c2 = (uint64_t)(a2>>b)<<32;
uint64_t c3 = (uint64_t)(a3>>b)<<48;
return c0 | c1 | c2 | c3;
//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
