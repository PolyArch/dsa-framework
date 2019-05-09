uint16_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint16_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;

uint64_t b = ops[1];
if(ops.size()==1) {
  b = 2;
}
uint64_t c0 = (uint64_t)(a0>>b)<<0;
uint64_t c1 = (uint64_t)(a1>>b)<<32;
return c0 | c1;
//return (uint64_t) _mm_adds_pu16((__m64)ops[0], (__m64)ops[1]);
