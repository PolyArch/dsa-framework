#define SIG (op*1024/(1024+op))
//#define SIG op

uint16_t op = (uint16_t)ops[0];

if(ops.size() > 1) {
  if(ops[1]) {
    return (uint64_t) SIG; 
  } else {
    return ops[0];
  }
}
return (uint64_t) SIG;
