#include "common.h"
#include "fp.h"


/**
 * Unisgned 128-bit integer representation.
 *   @high, low: The high bits, the low bits.
 */

struct u128_t {
	uint64_t high, low;
};


/*
 * floating point variables
 */

struct hp_t hp_zero = { 0.0, 0.0 };
struct hp_t hp_one = { 1.0, 0.0 };
struct hp_t hp_tenth = { 0.1, -5.551115123125783e-18 };

/*
 * bitmask definitions
 */

#define U32LOW(v)	(v & 0xFFFFFFFF)
#define U32HI(v)	U32LOW(v >> 32)


/**
 * Perform a 64-bit multiplication to obtain a 128-bit number.
 *   @a: The first number.
 *   @b: The second number.
 *   &returns: The 128-bit number.
 */

#define AMD64
#ifdef AMD64

static struct u128_t u128_mul(uint64_t a, uint64_t b)
{
	struct u128_t res;

	asm("mulq %3" : "=a" (res.low), "=d" (res.high) : "a" (a), "r" (b));

	return res;
}

#else

static struct u128_t u128_mul(uint64_t a, uint64_t b)
{
	struct u128_t res;
	uint64_t c, t, o;

	c = U32LOW(a) * U32LOW(b);
	res.low = U32LOW(c);

	o = 0;
	c >>= 32;

	c += t = U32HI(a) * U32LOW(b);
	o += (c < t) ? 1 : 0;

	c += t = U32LOW(a) * U32HI(b);
	o += (c < t) ? 1 : 0;

	res.low |= U32LOW(c) << 32;

	c >>= 32;
	c += o;
	c += U32HI(a) * U32HI(b);
	res.high = c;

	return res;
}

#endif


/**
 * Generate a floating point number by performing a multiple.
 *   @a: The first number.
 *   @b: The second number.
 *   &returns: The floating point number.
 */

struct hp_t fp_gen(double a, double b)
{
	struct hp_t fp;
	int a_exp, b_exp, r_exp;
	uint64_t a_u64, b_u64;
	struct u128_t r_u128;

	a_u64 = frexp(a, &a_exp) * 18446744073709551616.0;
	b_u64 = frexp(b, &b_exp) * 18446744073709551616.0;

	r_u128 = u128_mul(a_u64, b_u64);
	r_exp = a_exp + b_exp;

	a_u64 = r_u128.high & ~0x0FFF;
	b_u64 = (r_u128.high << 52) + (r_u128.low >> 12);

	fp.val = ldexp(a_u64 / 18446744073709551616.0, r_exp);
	fp.err = ldexp(b_u64 / 18446744073709551616.0, r_exp - 52);

	return fp;
}
