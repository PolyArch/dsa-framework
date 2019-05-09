int16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
int16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
int16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
int16_t a3 = (ops[0]&0xFFFF000000000000)>>48;

if ((ops.size() > 1) && (ops[1] == 0))
    return ops[0];

double d0 = FIX_TO_DOUBLE(a0);
double d1 = FIX_TO_DOUBLE(a1);
double d2 = FIX_TO_DOUBLE(a2);
double d3 = FIX_TO_DOUBLE(a3);

d0 = 1 / (1 + exp(-d0));
d1 = 1 / (1 + exp(-d1));
d2 = 1 / (1 + exp(-d2));
d3 = 1 / (1 + exp(-d3));

int16_t b0 = DOUBLE_TO_FIX(d0);
int16_t b1 = DOUBLE_TO_FIX(d1);
int16_t b2 = DOUBLE_TO_FIX(d2);
int16_t b3 = DOUBLE_TO_FIX(d3);

uint64_t c0 = ((uint64_t)(b0)<<0)&0x000000000000FFFF;
uint64_t c1 = ((uint64_t)(b1)<<16)&0x00000000FFFF0000;
uint64_t c2 = ((uint64_t)(b2)<<32)&0x0000FFFF00000000;
uint64_t c3 = ((uint64_t)(b3)<<48)&0xFFFF000000000000;

return c0 | c1 | c2 | c3;
