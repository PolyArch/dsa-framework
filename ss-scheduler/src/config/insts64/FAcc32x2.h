uint32_t t_a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t t_a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t t_b0 = (accum&0x00000000FFFFFFFF)>>0;
uint32_t t_b1 = (accum&0xFFFFFFFF00000000)>>32;

float a0=as_float(t_a0);
float a1=as_float(t_a1);
float b0=as_float(t_b0);
float b1=as_float(t_b1);

a0+=b0;
a1+=b1;

uint64_t c0 = (uint64_t)(as_uint32(a0))<<0;
uint64_t c1 = (uint64_t)(as_uint32(a1))<<32;
accum = c0 | c1;

uint64_t ret = accum;

if(ops[1]==2 || ops[1]==3) {
  discard=1;
} 
if(ops[1]!=0 && ops[1]!=2) {
  accum=0;
}


return ret; 
