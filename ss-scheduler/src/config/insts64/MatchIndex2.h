const uint64_t TokenEnd = ~0ull;

if (ops[0] == TokenEnd && ops[1] != TokenEnd) {
  back_array[0] = 1;
  back_array[1] = 0;
  discard = 1;
  //puts("end of ops[0]");
} else if (ops[1] == TokenEnd && ops[0] != TokenEnd) {
  back_array[0] = 0;
  back_array[1] = 1;
  discard = 1;
  //puts("end of ops[1]");
} else if (ops[0] == ops[1] && ops[0] == TokenEnd) {
  back_array[0] = 0;
  back_array[1] = 0;
  accum = 0;
  return 0;
  //puts("end of both");
} else {
  union {
    int16_t a[4];
    uint64_t val;
  } ret;
  ret.a[0] = ret.a[1] = ret.a[2] = ret.a[3] = 0;

  int a_idx0 = (ops[0] >> 0)  & 65535;
  int a_idx1 = (ops[0] >> 32) & 65535;
  int16_t a_val0 = (ops[0] >> 16) & 65535;
  int16_t a_val1 = (ops[0] >> 48) & 65535;

  int b_idx0 = (ops[1] >> 0)  & 65535;
  int b_idx1 = (ops[1] >> 32) & 65535;
  int16_t b_val0 = (ops[1] >> 16) & 65535;
  int16_t b_val1 = (ops[1] >> 48) & 65535;
  
  int index_a = (accum >> 0) & 65535;
  int index_b = (accum >> 16)& 65535;

  //printf("ops[0]: %d %d, %d %d\n", a_idx0, a_val0, a_idx1, a_val1);
  //printf("ops[1]: %d %d, %d %d\n", b_idx0, b_val0, b_idx1, b_val1);
  //printf("index_a': %d v.s. index_b': %d\n", index_a, index_b);

  index_a += a_idx0;
  index_b += b_idx0;

  if (index_a + a_idx1 == index_b) {
    ret.a[0] = a_val1;
    ret.a[1] = b_val0;
  } else if (index_b + b_idx1 == index_a) {
    ret.a[0] = a_val0;
    ret.a[1] = b_val1;
  } else if (index_a == index_b) {
    ret.a[0] = a_val0;
    ret.a[1] = b_val0;
    if (index_a + a_idx1 == index_b + b_idx1) {
      ret.a[2] = a_val1;
      ret.a[3] = b_val1;
    }
  }

  index_a += a_idx1;
  index_b += b_idx1;
  
  if (index_a < index_b) {
    back_array[0] = 0;
    back_array[1] = 1;
    index_b -= b_idx0 + b_idx1;
  } else if (index_a > index_b) {
    back_array[0] = 1;
    back_array[1] = 0;
    index_a -= a_idx0 + a_idx1;
  } else {
    back_array[0] = 0;
    back_array[0] = 0;
  }

  //printf("index_a: %d v.s. index_b: %d\n", index_a, index_b);

  accum = index_a | (index_b << 16);

  if (ret.val == 0)
    discard = 1;
  else {
    //printf("%d %d %d %d\n", (int16_t) (ret.val & 65535), (int16_t) (ret.val >> 16 & 65535),
       //(int16_t) (ret.val >> 32 & 65535), (int16_t) (ret.val >> 48 & 65535));
  }

  return ret.val;

}

return -1;
