uint16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
uint16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
uint16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
uint16_t r3 = (ops[0]&0xFFFF000000000000)>>48;

if(ops.size() > 1) { //additional op is acc
  return (r0+r1+r2+r3+((uint16_t)ops[1]));
} 
return (r0+r1+r2+r3);

