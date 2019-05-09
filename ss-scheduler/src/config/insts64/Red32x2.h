uint32_t r0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t r1 = (ops[0]&0xFFFFFFFF00000000)>>32;

if(ops.size() > 1) { //additional op is acc
  return (r0+r1+((uint32_t)ops[1]));
} 
return (r0+r1);

