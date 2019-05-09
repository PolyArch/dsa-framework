double a = as_double(ops[0]);
double b = as_double(accum);
double c = a + b;

uint64_t res = as_uint64(c);

accum = res;

if (ops[1] == 2 || ops[1] == 3) {
  discard = 1;
}

if (ops[1] != 0 && ops[1] != 2) {
  accum = 0;
}


return res;
