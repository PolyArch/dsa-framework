// NOTE: The macros below were copied from cambricon/include/fix_common.h
// in the ss-workloads repository

// In 16-bit integer representation,
// one bit is reserved for sign,
// the maximum supported number is 32767,
// the minimum supported number is -32768.
// Here FIX_MAX = 32767, and FIX_MIN is chosen to be negative
// of FIX_MAX instead of -32768 to keep the symmetry.

// FIX_TRUNC is to keep the number falling within the range
// between FIX_MIN and FIX_MAX (both inclusively)
#define FIX_MAX ((1 << 15) - 1)
#define FIX_MIN (-FIX_MAX)
#define FIX_TRUNC(x)  (x > FIX_MAX ? FIX_MAX : (x < FIX_MIN ? FIX_MIN : x) )

// FRAC_BITS is the number of bits reserved for fractional parts.
// So the integer part has 15 - FRAC_BITS bits.

// DELTA is the minimum positive amount that can be represented in this number system.

// FLOAT_MAX is the largest real value that can be represented in this number system.
// FLOAT_MIN is the smallest real value that can be represented in this number system.

// FLOAT_TRUNC is to keep numbers within the range
// between FLOAT_MIN and FLOAT_MAX (both inclusively)
#define FRAC_BITS 11  // 11 or 12 is recommended
#define DELTA (((double)1.0)/(1 << FRAC_BITS))
#define FLOAT_MAX (FIX_MAX * DELTA)
#define FLOAT_MIN (FIX_MIN * DELTA)
#define FLOAT_TRUNC(x)  (x > FLOAT_MAX ? FLOAT_MAX : (x < FLOAT_MIN ? FLOAT_MIN : x) )

// DOUBLE_TO_FIX converts a double number to integer in our fixed representation.
// FIX_TO_DOUBLE converts a integer number to double in our fixed representation.
#define DOUBLE_TO_FIX(x)  ( (int)(FLOAT_TRUNC(x) / DELTA) )
#define FIX_TO_DOUBLE(x) (x * DELTA)

// FIX_ADD fixed addition.
// FIX_MINUS fixed subtraction.
// FIX_MUL fixed multiplication.
// FIX_TAN_H fixed tanh, but is right now using tanh from math.h
#define FIX_ADD(a, b) ( a + b )
#define FIX_MINUS(a, b) ( a - b )
#define FIX_MUL(a, b) ( a * b )
#define FIX_TAN_H(x) ( DOUBLE_TO_FIX(tanh(FIX_TO_DOUBLE(x))) )

