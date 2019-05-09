uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
uint32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;

uint32_t sum;

sum = a0 - b0;
if (!((a0 ^ b0) & 0x80000000) && ((a0 ^ sum) & 0x80000000) && !(a0 & 0x80000000))
  a0 = 0x7FFFFFFF;
else if (!((a0 ^ b0) & 0x80000000) && ((a0 ^ sum) & 0x80000000) && (a0 & 0x80000000))
  a0 = 0x80000001;
else
  a0 = sum;

sum = a1 - b1;
if (!((a1 ^ b1) & 0x80000000) && ((a1 ^ sum) & 0x80000000) && !(a1 & 0x80000000))
  a1 = 0x7FFFFFFF;
else if (!((a1 ^ b1) & 0x80000000) && ((a1 ^ sum) & 0x80000000) && (a1 & 0x80000000))
  a1 = 0x80000001;
else
  a1 = sum;

uint64_t c0 = ((uint64_t)(a0))<<0 & 0x00000000FFFFFFFF;
uint64_t c1 = ((uint64_t)(a1))<<32 & 0xFFFFFFFF00000000;
return c0 | c1;
