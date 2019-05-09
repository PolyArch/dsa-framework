int32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
int32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
int32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
int32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;

int64_t im0 = ((int64_t)a0 * (int64_t)b0) >> 14; // 14 fractional bits 
int32_t m0 = im0 > (int64_t)0x000000007FFFFFFF ? (int32_t)0x7FFFFFFF : (im0 < (int64_t)0xFFFFFFFF80000001 ? (int32_t)0x80000001 : im0);

int64_t im1 = ((int64_t)a1 * (int64_t)b1) >> 14;
int32_t m1 = im1 > (int64_t)0x000000007FFFFFFF ? (int32_t)0x7FFFFFFF : (im1 < (int64_t)0xFFFFFFFF80000001 ? (int32_t)0x80000001 : im1);

uint64_t c0 = ((uint64_t)(m0)<<0)&0x00000000FFFFFFFF;
uint64_t c1 = ((uint64_t)(m1)<<32)&0xFFFFFFFF00000000;

return c0 | c1;
