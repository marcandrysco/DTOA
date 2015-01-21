#include "common.h"
#include <string.h>
#include "errol.h"
#include "fp.h"

#define ERROL_LOOKUP

#ifdef ERROL_LOOKUP
#	include "lookup.h"
#endif

static const double tens[] = {
	1e22,
	1e21,
	1e20,
	1e19,
	1e18,
	1e17,
	1e16,
	1e15,
	1e14,
	1e13,
	1e12,
	1e11,
	1e10,
	1e09,
	1e08,
	1e07,
	1e06,
	1e05,
	1e04,
	1e03,
	1e02,
	1e01,
	1e00
};

/**
 * Double to string conversion, producing maximum precision.
 *   @val: The value.
 *   @buf: The fraction data buffer.
 *   &returns: The exponent.
 */

_export
int32_t errol_hard(double val, char *buf)
{
	uint8_t i, digit, ldig, hdig;
	double prev = nextafter(val, -INFINITY), next = nextafter(val, INFINITY);

	if(val >= 5e22)
		return -1;
	else if(val < 1e15)
		return -1;
	else if(fmod(val, 1.0) != 0)
		return -1;

	for(i = 0; i < sizeof(tens) / sizeof(double); i++) {
		digit = val / tens[i];
		val -= digit * tens[i];
		buf[i] = digit + '0';

		ldig = prev / tens[i];
		prev -= ldig * tens[i];

		hdig = next / tens[i];
		next -= hdig * tens[i];
	}

	buf[i] = '\0';

	return 23;
}


_export
int32_t errol_str(double val, char *buf)
{
	int hdig, mdig, ldig;
	int32_t exp;
	struct hp_t high, mid, low;

	if((val == 0.0) || (val == -0.0))
		return buf[0] = '\0', 0;

	exp = errol_hard(val, buf);
	if(exp != -1)
		return exp;

#ifdef ERROL_LOOKUP
	exp = 309 + (int)log10(val);
	if(exp < 0)
		exp = 0;
	else if(exp >= LOOKUP_TABLE_LEN)
		exp = LOOKUP_TABLE_LEN - 1;

	mid = lookup_table[exp];
	mid = fp_prod(mid, val);

	high.val = mid.val;
	//high.err = mid.err + (nextafter(val, INFINITY) - val) * lookup_table[exp].val / 2.000000000001;
	high.err = mid.err + (nextafter(val, INFINITY) - val) * lookup_table[exp].val / 2;
	//high.err = mid.err + (nextafter(val, INFINITY) - val) / 2.000000000001 * lookup_table[exp].val;

	low.val = mid.val;
	low.err = mid.err + (nextafter(val, -INFINITY) - val) * lookup_table[exp].val / 2;
	//low.err = mid.err + (nextafter(val, -INFINITY) - val) / 2.000000000001 * lookup_table[exp].val;

	exp -= 308;
#else
	high = (struct hp_t){ val, (nextafter(val, INFINITY) - val) / 2.0 };
	low = (struct hp_t){ val, (nextafter(val, -INFINITY) - val) / 2.0 };
	exp = 0;
#endif

	while(high.val < 0.1 || (high.val == 0.1 && high.err < hp_tenth.err))
		exp--, fp_mul(&high), fp_mul(&low); //midpt , fp_mul(&mid);

	while(high.val > 1.0 || (high.val == 1.0 && high.err >= 0.0))
		exp++, fp_div(&high), fp_div(&low); //midpt , fp_div(&mid);

	mdig=0,ldig=0;
	while(high.val != 0.0 || high.err != 0.0) {
		fp_mul(&high);
		fp_mul(&mid);
		fp_mul(&low);

		hdig = (int)(high.val);
		high.val -= hdig;

		ldig = (int)(low.val);
		low.val -= ldig;

		*buf++ = hdig + '0';

		if(ldig != hdig)
			break;
	}

	*buf = '\0';

	return exp;
}


/**
 * Perform the conversion fast, even if the generated number is not shortest.
 *   @val: The value.
 *   @buf: The output buffer.
 *   &returns: The exponent.
 */
/**
 * Read the time source clock.
 *   &returns: THe source clock as a 64-bit unsignd integer.
 */

static inline uint64_t rdtsc()
{
	uint32_t a, d;

	__asm__ __volatile__("cpuid;"
			"rdtsc;"
			: "=a" (a), "=d" (d)
			:
			: "%rcx", "%rbx", "memory");

	return ((uint64_t)a) | (((uint64_t)d) << 32);
}

static inline double getprev(double d)
{
	union { double d; uint64_t i; } u = { .d = d };

	u.i--;
	return u.d;
}

static inline double getnext(double d)
{
	union { double d; uint64_t i; } u = { .d = d };

	u.i++;
	return u.d;
}


/**
 * A simple implementation of Errol that does not use many optimizations.
 *   @val: The value.
 *   @buf: The output buffer.
 *   &returns: The exponent.
 */

_export
int32_t errol_simple(double val, char *buf)
{
	uint32_t hdig, ldig;
	int32_t exp;
	struct hp_t high, mid, low;

	int e;
	frexp(val, &e);
	exp = 309 + (double)e*0.30103; //0.30103 = log_10(2)
	if(exp < 0)
		exp = 0;
	else if(exp >= LOOKUP_TABLE_LEN)
		exp = LOOKUP_TABLE_LEN - 1;

	mid = lookup_table[exp];
	mid = fp_prod(mid, val);

	high.val = low.val = mid.val;
	high.err = mid.err + (getnext(val) - val) * lookup_table[exp].val / 2.0000000000000016;
	low.err = mid.err + (getprev(val) - val) * lookup_table[exp].val / 2.0000000000000016;

	exp -= 308;

	while(high.val < 0.1 || (high.val == 0.1 && high.err < hp_tenth.err))
		exp--, fp_mul(&high), fp_mul(&low);

	while(high.val > 1.0 || (high.val == 1.0 && high.err >= 0.0))
		exp++, fp_div(&high), fp_div(&low);

	while(1) {
		fp_mul(&high);
		fp_mul(&low);

		hdig = (int)(high.val);
		high.val -= hdig;
		if(high.val == 0.0 && high.err < 0)
			hdig -= 1, high.val += 1.0;

		ldig = (int)(low.val);
		low.val -= ldig;
		if(low.val == 0.0 && low.err < 0)
			hdig -= 1, low.val += 1.0;

		*buf++ = hdig + '0';

		if(ldig != hdig)
			break;
	}

	*buf = '\0';

	return exp;
}

_export
int32_t errol_fast(double val, char *buf)
{
	uint32_t hdig, ldig;
	int32_t exp;
	struct hp_t high, mid, low;

	int e;
	frexp(val, &e);
	exp = 309 + (double)e*0.30103; //0.30103 = log_10(2)
	if(exp < 0)
		exp = 0;
	else if(exp >= LOOKUP_TABLE_LEN)
		exp = LOOKUP_TABLE_LEN - 1;

	mid = lookup_table[exp];
	mid = fp_prod(mid, val);

	high.val = low.val = mid.val;
	high.err = mid.err + (getnext(val) - val) * lookup_table[exp].val / 2.0000000000000016;
	low.err = mid.err + (getprev(val) - val) * lookup_table[exp].val / 2.0000000000000016;

	exp -= 308;

	while(high.val < 0.1 || (high.val == 0.1 && high.err < hp_tenth.err))
		exp--, fp_mul(&high), fp_mul(&low);

	while(high.val > 1.0 || (high.val == 1.0 && high.err >= 0.0))
		exp++, fp_div(&high), fp_div(&low);

	while(1) {
		high = hp_mul10000(high);
		low = hp_mul10000(low);

		uint16_t h = (uint16_t)high.val;
		high.val -= h;
		if(high.val == 0.0 && high.err < 0)
			h -= 1, high.val += 1.0;

		uint16_t l = (uint16_t)low.val;
		low.val -= l;
		if(low.val == 0.0 && low.err < 0)
			l -= 1, low.val += 1.0;

		hdig = h / 1000;
		ldig = l / 1000;
		*buf++ = hdig + '0';
		if(ldig != hdig)
			break;

		h -= hdig * 1000;
		l -= ldig * 1000;
		hdig = h/100;
		ldig = l/100;
		*buf++ = hdig + '0';
		if(ldig != hdig)
			break;

		h -= hdig * 100;
		l -= ldig * 100;
		hdig = h/10;
		ldig = l/10;
		*buf++ = hdig + '0';
		if(ldig != hdig)
			break;

		hdig = h % 10;
		ldig = l % 10;
		*buf++ = hdig + '0';
		if(ldig != hdig)
			break;
	}

	*buf = '\0';

	return exp;
}

/**
 * Perform the shortest possible conversion.
 *   @val: The value.
 *   @buf: The output buffer.
 *   &returns: The exponent.
 */
char allzero(const char *str)
{
	while(*str == '0')
		str++;

	return *str == '\0';
}

_export
int32_t errol_short(double val, char *buf)
{
	if(val == 4.503599627370496e+38) {
		strcpy(buf, "4503599627370496");
		return 39;
	}
	else if((val < 1.80143985094820e+16) || (val >= 3.40282366920938e+38)) {
		uint32_t hdig, ldig;
		int32_t exp;
		struct hp_t high, mid, low;

		int e;
		frexp(val, &e);
		exp = 309 + (double)e*0.30103; //0.30103 = log_10(2)
		if(exp < 0)
			exp = 0;
		else if(exp >= LOOKUP_TABLE_LEN)
			exp = LOOKUP_TABLE_LEN - 1;

		mid = lookup_table[exp];
		mid = fp_prod(mid, val);

		high.val = low.val = mid.val;
		high.err = mid.err + (getnext(val) - val) * lookup_table[exp].val / 2;
		low.err = mid.err + (getprev(val) - val) * lookup_table[exp].val / 2;

		exp -= 308;

		while(high.val < 0.1 || (high.val == 0.1 && high.err < hp_tenth.err))
			exp--, fp_mul(&high), fp_mul(&low);

		while(high.val > 1.0 || (high.val == 1.0 && high.err >= 0.0))
			exp++, fp_div(&high), fp_div(&low);

		while(1) {
			high = hp_mul10000(high);
			low = hp_mul10000(low);

			uint16_t h = (uint16_t)high.val;
			high.val -= h;
			if(high.val == 0.0 && high.err < 0)
				h -= 1, high.val += 1.0;

			uint16_t l = (uint16_t)low.val;
			low.val -= l;
			if(low.val == 0.0 && low.err < 0)
				l -= 1, low.val += 1.0;

			hdig = h / 1000;
			ldig = l / 1000;
			*buf++ = hdig + '0';
			if(ldig != hdig)
				break;

			h -= hdig * 1000;
			l -= ldig * 1000;
			hdig = h/100;
			ldig = l/100;
			*buf++ = hdig + '0';
			if(ldig != hdig)
				break;

			h -= hdig * 100;
			l -= ldig * 100;
			hdig = h/10;
			ldig = l/10;
			*buf++ = hdig + '0';
			if(ldig != hdig)
				break;

			hdig = h % 10;
			ldig = l % 10;
			*buf++ = hdig + '0';
			if(ldig != hdig)
				break;
		}

		*buf = '\0';

		return exp;
	}
	else {
		int8_t i, j;
		int32_t exp;
		union { double d; uint64_t i; } bits;
		char lstr[41], hstr[41];
		uint64_t l64, h64;
		__uint128_t v, low, high, pow19 = (__uint128_t)1e19;

		low = (__uint128_t)val - (__uint128_t)((nextafter(val, INFINITY) -val) / 2.0);
		high = (__uint128_t)val + (__uint128_t)((val - nextafter(val, -INFINITY)) / 2.0);

		bits.d = val;
		if(bits.i & 0x1)
			low++, high--;

		i = 39;
		lstr[40] = hstr[40] = '\0';
		while(high != 0) {
			l64 = low % pow19;
			low /= pow19;
			h64 = high % pow19;
			high /= pow19;

			for(j = 0; ((high != 0) && (j < 19)) || ((high == 0) && (h64 != 0)); j++, i--) {
				lstr[i] = '0' + l64 % (uint64_t)10;
				hstr[i] = '0' + h64 % (uint64_t)10;

				l64 /= 10;
				h64 /= 10;
			}
		}

		exp = 39 - i++;

		do
			*buf++ = hstr[i++];
		while(hstr[i] == lstr[i]);

		if(allzero(lstr+i) || allzero(hstr+i)) {
			while(buf[-1] == '0')
				buf--;
		}
		else
			*buf++ = hstr[i];

		*buf = '\0';

		return exp;
	}
}
