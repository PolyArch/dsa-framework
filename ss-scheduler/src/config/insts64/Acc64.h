// Decode Control Logic
bool do_reset   = ops[1] & (1 << 0);
bool do_discard = ops[1] & (1 << 1);
bool dont_accum = ops[1] & (1 << 2);
// bool do_set_input   = ops[1] & (1 << 3);

if(!dont_accum) {
  accum+=ops[0];
}

uint64_t ret = accum;

if(do_discard) {
  discard=1;
} 
if(do_reset) {
  accum=0;
}
return ret; 
