// i/p is ops[0]-64 bit, ops[1]-64 bit
uint16_t a0 = (ops[0]&0x000000000000F)>>0;

uint16_t prev_x = reg[0];
uint16_t prev_y = reg[1];

// ops[1] is R
// ops[2] is S
x = (prev_x + a0 + 1)%ops[1];
y = (prev_y + (a0 + 1)/ops[1])%ops[2];

return x*ops[2]+y;
