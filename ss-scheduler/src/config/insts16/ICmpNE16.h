#define SENTINAL16 ( ((uint16_t)1)<<15)

if(ops[0]==SENTINAL16 || ops[1]==SENTINAL16){
    return 2;
}
return ops[0] != ops[1]; 
