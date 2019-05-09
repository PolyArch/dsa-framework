uint16_t r0 = (ops[0]&0x000000000000FFFF)>>0;
uint16_t r1 = (ops[0]&0x00000000FFFF0000)>>16;
uint16_t r2 = (ops[0]&0x0000FFFF00000000)>>32;
uint16_t r3 = (ops[0]&0xFFFF000000000000)>>48;

uint16_t sum;

uint16_t sum0 = r0 + r1;
if (!((r0 ^ r1) & 0x8000) && ((r0 ^ sum0) & 0x8000) && !(r0 & 0x8000))
  sum0 = 0x7FFF;
else if (!((r0 ^ r1) & 0x8000) && ((r0 ^ sum0) & 0x8000) && (r0 & 0x8000))
  sum0 = 0x8001;

uint16_t sum1 = r2 + r3;
if (!((r2 ^ r3) & 0x8000) && ((r2 ^ sum1) & 0x8000) && !(r2 & 0x8000))
  sum1 = 0x7FFF;
else if (!((r2 ^ r3) & 0x8000) && ((r2 ^ sum1) & 0x8000) && (r2 & 0x8000))
  sum1 = 0x8001;

uint16_t sum2 = sum0 + sum1;
if (!((sum0 ^ sum1) & 0x8000) && ((sum0 ^ sum2) & 0x8000) && !(sum0 & 0x8000))
  sum2 = 0x7FFF;
else if (!((sum0 ^ sum1) & 0x8000) && ((sum0 ^ sum2) & 0x8000) && (sum0 & 0x8000))
  sum2 = 0x8001;

if(ops.size() > 1) { //additional op is acc
  sum = sum2 + (uint16_t)ops[1];
  if (!((sum2 ^ (uint16_t)ops[1]) & 0x8000) && ((sum2 ^ sum) & 0x8000) && !(sum2 & 0x8000))
    sum = 0x7FFF;
  else if (!((sum2 ^ (uint16_t)ops[1]) & 0x8000) && ((sum2 ^ sum) & 0x8000) && (sum2 & 0x8000))
    sum = 0x8001;
} else {
  sum = sum2;
}

return sum;
