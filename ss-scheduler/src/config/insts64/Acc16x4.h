// Decode control logic
bool do_reset   = ops[1] & (1 << 0);
bool do_discard = ops[1] & (1 << 1);
bool dont_accum = ops[1] & (1 << 2);

if(!dont_accum) {
  uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
  uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
  uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
  uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
  uint16_t b0 = (accum&0x000000000000FFFF)>>0;
  uint16_t b1 = (accum&0x00000000FFFF0000)>>16;
  uint16_t b2 = (accum&0x0000FFFF00000000)>>32;
  uint16_t b3 = (accum&0xFFFF000000000000)>>48;

  a0+=b0;
  a1+=b1;
  a2+=b2;
  a3+=b3;
  uint64_t c0 = (uint64_t)(a0)<<0;
  uint64_t c1 = (uint64_t)(a1)<<16;
  uint64_t c2 = (uint64_t)(a2)<<32;
  uint64_t c3 = (uint64_t)(a3)<<48;

  accum = c0 | c1 | c2 | c3;
}

uint64_t ret = accum;

if(do_discard) {
  discard=1;
} 
if(do_reset) {
  accum=0;
}

return ret; 
