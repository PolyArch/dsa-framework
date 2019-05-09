#define SENTINAL16 ( ((uint16_t)1)<<15)

if(ops[0]==SENTINAL16 && ops[1]==SENTINAL16) {
    back_array[0]=0;
    back_array[1]=0;
    // return 0; // should not generate any backpressure
    return 3;
} 
else if(ops[0]==ops[1]){
    back_array[0]=0;
    back_array[1]=0;
    return 0;
}
else if(ops[0]<ops[1] || ops[1]==SENTINAL16){
    back_array[0]=0;
    back_array[1]=1;
    return 1;
}
else if(ops[1]<ops[0] || ops[0]==SENTINAL16){
    back_array[0]=1;
    back_array[1]=0;
    return 2;
} else {
    assert(0 && "not possible");
}
