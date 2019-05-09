back_array[0] = (ops[2] >> 3) & 1;
back_array[1] = (ops[2] >> 4) & 1;
back_array[2] = 0;

uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
uint32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;

int fm1 = (ops[2] >> 5) & 3;
int fm2 = (ops[2] >> 7) & 3;

uint64_t c0 = 0;
uint64_t c1 = 0;

// could have been fix minus
switch(fm1){
    case 1: c0 = (uint64_t)(abs((int)(a0-b0)))<<0;
            break;
    case 2: c0 = (uint64_t)(abs((int)(a0-b1)))<<0;
            break;
    default: c0 = (uint64_t)(0)<<0;
             break;
}

switch(fm2){
    case 1: c1 = (uint64_t)(abs((int)(a1-b0)))<<32;
            break;
    case 2: c1 = (uint64_t)(abs((int)(a1-b1)))<<32;
            break;
    default: c1 = (uint64_t)(0)<<32;
             break;
}

return c0 | c1;
