// control signals for select
back_array[0] = ops[2] & (1 << 1);
back_array[1]= ops[2] & (1 << 2);

return ((ops[2]&1)==0) ? ops[0] : ops[1];
