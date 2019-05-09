// ops[0] is sind1, ops[1] is Tx, ops[2] is Kx

uint16_t a0 = (ops[0]&0x000000000000FFFF)>>0;
uint16_t a1 = (ops[0]&0x00000000FFFF0000)>>16;
uint16_t a2 = (ops[0]&0x0000FFFF00000000)>>32;
uint16_t a3 = (ops[0]&0xFFFF000000000000)>>48;
uint16_t b0 = (ops[1]&0x000000000000FFFF)>>0;
uint16_t b1 = (ops[1]&0x00000000FFFF0000)>>16;
uint16_t b2 = (ops[1]&0x0000FFFF00000000)>>32;
uint16_t b3 = (ops[1]&0xFFFF000000000000)>>48;
uint16_t c0 = (ops[2]&0x000000000000FFFF)>>0;
uint16_t c1 = (ops[2]&0x00000000FFFF0000)>>16;
uint16_t c2 = (ops[2]&0x0000FFFF00000000)>>32;
uint16_t c3 = (ops[2]&0xFFFF000000000000)>>48;

a0 = (uint16_t)(a0/c0) + (uint16_t)(b0*(a0%c0));
a1 = (uint16_t)(a1/c1) + (uint16_t)(b1*(a1%c1));
a2 = (uint16_t)(a2/c2) + (uint16_t)(b2*(a2%c2));
a3 = (uint16_t)(a3/c3) + (uint16_t)(b3*(a3%c3));

uint64_t d0 = (uint64_t)(a0)<<0;
uint64_t d1 = (uint64_t)(a1)<<16;
uint64_t d2 = (uint64_t)(a2)<<32;
uint64_t d3 = (uint64_t)(a3)<<48;
return d0 | d1 | d2 | d3;
