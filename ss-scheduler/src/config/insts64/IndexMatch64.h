#define SENTINAL ( ((uint64_t)1)<<63)

if(ops[0]==SENTINAL && ops[1]==SENTINAL) {
    back_array[0]=0;
    back_array[1]=0;
    return 1; // should not generate any backpressure
} 
else if(ops[0]==ops[1]){
    back_array[0]=0;
    back_array[1]=0;
    return 0;
}
else if(ops[0]<ops[1] || ops[1]==SENTINAL){
    back_array[0]=0;
    back_array[1]=1;
    return 1;
}
else if(ops[1]<ops[0] || ops[0]==SENTINAL){
    back_array[0]=1;
    back_array[1]=0;
    return 2;
} else {
    assert(0 && "not possible");
}
