// Control Signal Definitions
int do_reset   = 1 << 0;
int do_discard = 1 << 1;
int dont_accum = 1 << 2;
int bp_op1     = 1 << 3;
int bp_op2     = 1 << 4;

#define SENTINAL ( ((uint64_t)1)<<63)

if(ops[0]==SENTINAL && ops[1]==SENTINAL) {
    back_array[0]=0;
    back_array[1]=0;
    return dont_accum | do_reset;
} 
else if(ops[0]==ops[1]){
    back_array[0]=0;
    back_array[1]=0;
    return do_discard;
}
else if(ops[0]<ops[1] || ops[1]==SENTINAL){
    back_array[0]=0;
    back_array[1]=1;
    return dont_accum | do_discard | bp_op2;
}
else if(ops[1]<ops[0] || ops[0]==SENTINAL){
    back_array[0]=1;
    back_array[1]=0;
    return dont_accum | do_discard | bp_op1;
} else {
    assert(0 && "not possible");
}
