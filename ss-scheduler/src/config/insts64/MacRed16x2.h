#define FxPnt 8
#define fx_to_flt(x) ((float)(x) / (1<<FxPnt))
#define flt_to_fx(x) ((int)(x) * (1<<FxPnt))

int16_t a = (ops[0] >> 0)  & 65535;
int16_t b = (ops[0] >> 16) & 65535;
int16_t c = (ops[0] >> 32) & 65535;
int16_t d = (ops[0] >> 48) & 65535;


int64_t _accum = accum;

_accum += ((int) a * (int) b) + ((int) c * (int) d);

//printf("%f * %f = %f\n", fx_to_flt(a), fx_to_flt(b), fx_to_flt(a) * fx_to_flt(b));
//printf("%f * %f = %f\n", fx_to_flt(c), fx_to_flt(d), fx_to_flt(c) * fx_to_flt(d));

accum = _accum;


if (ops[0] != 0) {
  discard = 1;
} else {
  accum = 0;
  //printf("Accum: %f\n", fx_to_flt(_accum) / (1 << FxPnt));
}

return _accum;

#undef FxPnt
#undef fx_to_flt
#undef flt_to_fx
