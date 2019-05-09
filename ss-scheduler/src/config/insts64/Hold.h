bool done = ops[1] & (1 << 0);
bool do_update = ops[1] & (1 << 1);

uint64_t ret = reg[0];
// should I use accum?
if(do_update) {
    reg[0] = ops[0];
    ret = reg[0];
}
if(done) {
    reg[0]=0;
} else {
    discard=true;
}

return ret;
