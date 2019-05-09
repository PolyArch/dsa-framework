union {
  uint16_t a[2];
  uint32_t val;
} res;

res.a[0] = ops[0] & 65535;
res.a[1] = ops[1] & 65535;

return res.val;
