if ((ops.size() > 1) && (ops[1] == 0))
    return ops[0];

double d = FIX_TO_DOUBLE(ops[0]);

d = exp(d);

uint64_t b = DOUBLE_TO_FIX(d);


return b;
