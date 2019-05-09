uint16_t i1 = (ops[0]&0x000000000000FFFF)>>0;
uint16_t i2 = (ops[0]&0x00000000FFFF0000)>>16;
uint16_t i3 = (ops[0]&0x0000FFFF00000000)>>32;
uint16_t i4 = (ops[0]&0xFFFF000000000000)>>48;

if ((ops.size() > 1) && (ops[1] == 0))
    return ops[0];

if (i1 & 0x8000)
  i1 = 0;

if (i2 & 0x8000)
  i2 = 0;

if (i3 & 0x8000)
  i3 = 0;

if (i4 & 0x8000)
  i4 = 0;

uint64_t o1 = (uint64_t)(i1)<<0;
uint64_t o2 = (uint64_t)(i2)<<16;
uint64_t o3 = (uint64_t)(i3)<<32;
uint64_t o4 = (uint64_t)(i4)<<48;

return o1 | o2 | o3 | o4;
