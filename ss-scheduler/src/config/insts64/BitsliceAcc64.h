// Decode Control Logic
bool do_reset   = ops[1] & (1 << 0);
bool do_discard = ops[1] & (1 << 1);
bool dont_accum = ops[1] & (1 << 2);
// bool do_set_input   = ops[1] & (1 << 3);

// I need to extract 16-bits from reg[1] location in ops[0] and accum
// uint64_t b = (ops[0] << (reg[1]*16))&OxFFFFFFFFFFFFFFFF;
uint64_t b = ops[0] << (reg[1]*16);


if(!dont_accum) {
  accum+=b;
}

uint64_t ret = accum;

if(do_discard) {
  discard=1;
} 
if(do_reset) {
  accum=0;
}

if(!do_discard){
 if(++reg[1]!=4){
   discard=true;
 } else {
   reg[1]=0;
 }
}

return ret; 

