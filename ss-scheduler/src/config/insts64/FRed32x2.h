uint32_t t_r0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t t_r1 = (ops[0]&0xFFFFFFFF00000000)>>32;

float r0=as_float(t_r0);
float r1=as_float(t_r1);

float result;
if(ops.size() > 1) { //additional op is acc
  std::cout << as_float((uint32_t)ops[1]) << std::endl;
  result = r0 + r1 + as_float((uint32_t)ops[1]);
} else {
  result = r0 + r1;
}

return (uint64_t)(as_uint32(result));

