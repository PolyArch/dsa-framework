int16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
int16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
int16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
int16_t r3 = (ops[0]&0xFFFF000000000000)>>48;

int16_t x = r0;
if(r1 < x) {x=r1;}
if(r2 < x) {x=r2;}
if(r3 < x) {x=r3;}

if(ops.size() > 1) { //additional op is acc
  int16_t b = (int16_t)ops[1];
  if(b < x) {x=b;}
} 
return (uint64_t)x;

