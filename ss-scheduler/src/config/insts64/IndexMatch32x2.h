// Control Signal Definitions
int do_reset   = 1 << 0;
int do_discard = 1 << 1;
int dont_accum = 1 << 2;
int bp_op1     = 1 << 3;
int bp_op2     = 1 << 4;

#define SENTINAL32 ( ((uint32_t)1)<<31)

uint32_t a0 = (ops[0]&0x00000000FFFFFFFF)>>0;
uint32_t a1 = (ops[0]&0xFFFFFFFF00000000)>>32;
uint32_t b0 = (ops[1]&0x00000000FFFFFFFF)>>0;
uint32_t b1 = (ops[1]&0xFFFFFFFF00000000)>>32;

int fm1=0;
int fm2=0;

if(a0==b0){
    fm1=1;
} else if(a0==b1){
    fm1=2;
}

if(a1==b0){
    fm2=1;
} else if(a1==b1){
    fm2=2;
}

int mul_sel = (fm1 << 5) | (fm1 << 7);

if(a1==SENTINAL32 && b1==SENTINAL32) {
    back_array[0]=0;
    back_array[1]=0;
    return dont_accum | do_reset | mul_sel;
} 
else if(a1==b1){
    back_array[0]=0;
    back_array[1]=0;
    return do_discard | mul_sel;
}
else if(a1<b1 || b1==SENTINAL32){
    back_array[0]=0;
    back_array[1]=1;
    return dont_accum | do_discard | bp_op2 | mul_sel;
}
else if(a1>b1 || a1==SENTINAL32){
    back_array[0]=1;
    back_array[1]=0;
    return dont_accum | do_discard | bp_op1 | mul_sel;
} else {
    assert(0 && "not possible");
}
