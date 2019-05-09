back_array[0] = (ops[2] >> 3) & 1;
back_array[1] = (ops[2] >> 4) & 1;
back_array[2] = 0;

double a = as_double(ops[0]);
double b = as_double(ops[1]);
double c = a*b;
return as_uint64(c); 
