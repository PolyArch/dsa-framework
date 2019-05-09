float a = as_float(ops[0]);
float b = as_float(ops[1]);

union {
  float val;
  uint32_t res;
} ri;
ri.val = a - b;

return ri.res;
