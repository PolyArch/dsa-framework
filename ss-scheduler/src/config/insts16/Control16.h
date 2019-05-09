uint16_t ret;

// control signal
if(ops[0]==0 || ops[1] == 0) {
  back_array[0]=0;
  back_array[1]=0;
} else {
  if(ops[0]<ops[1]){
	back_array[0]=0;
	back_array[1]=1;
  } else {
	back_array[0]=1;
	back_array[1]=0;
  }
}

if(ops[0]==SENTINAL16 && ops[1]==SENTINAL16){
  back_array[0]=0;
  back_array[1]=0;
}

ret = back_array[0] << 1 | back_array[1] << 2;

if(ops[0]<ops[1]){
  ret = ret | 1;
} else if(ops[0]==SENTINAL16 && ops[1]==SENTINAL16){
  ret = 3; // 0,1,1(this will always be 0)
}
  

return ret;
