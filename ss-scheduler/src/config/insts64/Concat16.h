// Concat 16-bit slice in cyclic order

uint64_t ret = 0x0000000000000000;
uint16_t val;

/*
switch(reg[1]) {
  case 0: val = (ops[0]&0x000000000000FFFF)>>0;
		  break;
  case 1: val = (ops[0]&0x00000000FFFF0000)>>16;
		  break;
  case 2: val = (ops[0]&0x0000FFFF00000000)>>32;
		  break;
  case 3: val = (ops[0]&0xFFFF000000000000)>>48;
		  break;
  default: printf("wrong slice number");
		   break;
}
*/

switch(reg[1]) {
  case 3: val = (ops[0]&0x000000000000FFFF)>>0;
		  break;
  case 2: val = (ops[0]&0x00000000FFFF0000)>>16;
		  break;
  case 1: val = (ops[0]&0x0000FFFF00000000)>>32;
		  break;
  case 0: val = (ops[0]&0xFFFF000000000000)>>48;
		  break;
  default: printf("wrong slice number");
		   break;
}

ret = ret | val;
discard=true;

if(++reg[1]==4){
  reg[1]=0;
  discard=false;
}

return ret;
