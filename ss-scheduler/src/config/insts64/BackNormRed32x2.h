// Decode Control Logic
bool do_reset   = ops[1] & (1 << 0);
bool do_discard = ops[1] & (1 << 1);
bool dont_accum = ops[1] & (1 << 2);

if(!dont_accum) {
  uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
  uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
  accum+=((a0*a0)+(a1*a1));
}

uint64_t ret = accum;

if(do_discard) {
  discard=1;
} 
if(do_reset) {
  accum=0;
}
return ret; 
