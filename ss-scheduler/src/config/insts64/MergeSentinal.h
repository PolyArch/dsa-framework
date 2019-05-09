#define SENTINAL ( ((uint64_t)1)<<63)

if(ops[0]==SENTINAL && ops[1]==SENTINAL) {
    back_array[0]=0;
    back_array[1]=0;
    discard=true;
    return -1; // don't generate anything 
} 
else if(ops[0]<=ops[1] || ops[1]==SENTINAL){
    back_array[0]=0;
    back_array[1]=1;
    return ops[0];
}
else if(ops[1]<ops[0] || ops[0]==SENTINAL){
    back_array[0]=1;
    back_array[1]=0;
    return ops[1];
} else {
    assert(0 && "not possible");
}
