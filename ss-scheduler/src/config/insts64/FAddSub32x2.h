uint32_t t_a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t t_a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t t_b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
uint32_t t_b1 = (ops[1]&0xFFFFFFFF00000000)>>32;

float a0=as_float(t_a0);
float a1=as_float(t_a1);
float b0=as_float(t_b0);
float b1=as_float(t_b1);

a0+=a1;
b0-=b1;

uint64_t c0 = (uint64_t)(as_uint32(a0))<<0;
uint64_t c1 = (uint64_t)(as_uint32(b0))<<32;
return c0 | c1;

// Reduce the value respectively:
// A: A0    A1
// B: B0    B1
// C: A0+A1 B0-B1
