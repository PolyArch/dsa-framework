uint32_t r0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t r1 = (ops[0]&0xFFFFFFFF00000000)>>32;

uint32_t sum;

uint32_t sum0 = r0 + r1;
if (!((r0 ^ r1) & 0x80000000) && ((r0 ^ sum0) & 0x80000000) && !(r0 & 0x80000000))
  sum0 = 0x7FFFFFFF;
else if (!((r0 ^ r1) & 0x80000000) && ((r0 ^ sum0) & 0x80000000) && (r0 & 0x80000000))
  sum0 = 0x80000001;

if(ops.size() > 1) { //additional op is acc
  sum = sum0 + (uint32_t)ops[1];
  if (!((sum0 ^ (uint32_t)ops[1]) & 0x80000000) && ((sum0 ^ sum) & 0x80000000) && !(sum0 & 0x80000000))
    sum = 0x7FFFFFFF;
  else if (!((sum0 ^ (uint32_t)ops[1]) & 0x80000000) && ((sum0 ^ sum) & 0x80000000) && (sum0 & 0x80000000))
    sum = 0x80000001;
} else {
  sum = sum0;
}

return sum;
