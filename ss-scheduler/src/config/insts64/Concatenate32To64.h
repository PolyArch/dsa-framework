union {
  uint32_t a[2];
  uint64_t val;
} res;

res.a[0] = ops[0];
res.a[1] = ops[1];

return res.val;
