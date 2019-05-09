uint32_t _a0 = (ops[0]&0x00000000FFFFFFFF)>>0;

float a0=as_float(_a0);

complex<float> val = sqrt(a0);

uint64_t res;

memcpy(&res, &val, sizeof res);

return res;
