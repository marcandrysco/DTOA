#ifndef FP_H
#define FP_H

/*
 * headers
 */

#include <math.h>


/**
 * Floating point structure.
 *   @val, err: The value and error.
 */

struct hp_t {
	double val, err;
};


/*
 * floating point variables
 */

extern struct hp_t hp_zero;
extern struct hp_t hp_one;
extern struct hp_t hp_tenth;


/**
 * Normalize the number by factoring in the error.
 *   @fp: The float pair.
 */

static inline void fp_normalize(struct hp_t *fp)
{
	double val = fp->val;

	fp->val += fp->err;
	fp->err += val - fp->val;
}

/**
 * Multiply the float pair by ten.
 *   @fp: The float pair.
 */

static inline void fp_mul(struct hp_t *fp)
{
	double err, val = fp->val;

	fp->val *= 10.0;
	fp->err *= 10.0;
	
	err = fp->val;
	err -= val * 8.0;
	err -= val * 2.0;

	fp->err -= err;

	fp_normalize(fp);
}

/**
 * Multiply the float pair by 100.
 *   @hp: The high-precision number.
 */

static inline void fp_mul100(struct hp_t *fp)
{
	double err, val = fp->val;

	fp->val *= 100.0;
	fp->err *= 100.0;
	
	err = fp->val;
	err -= val * 64.0;
	err -= val * 32.0;
	err -= val * 4.0;

	fp->err -= err;

	fp_normalize(fp);
}

/**
 * Multiply the float pair by 10000.
 *   @hp: The high-precision number.
 */

static inline struct hp_t hp_mul10000(struct hp_t in)
{
	struct hp_t out;

	out.val = in.val * 10000.0;
	out.err = in.err * 10000.0 - (out.val - (in.val * 8192.0) - (in.val * 1024.0) - (in.val * 512.0) - (in.val * 256.0) - (in.val * 16.0));
	fp_normalize(&out);

	return out;
}

/**
 * Multiply the float pair by ten.
 *   @fp: The float pair.
 */

static inline void fp_div(struct hp_t *fp)
{
	double val = fp->val;

	fp->val /= 10.0;
	fp->err /= 10.0;

	val -= fp->val * 8.0;
	val -= fp->val * 2.0;

	fp->err += val / 10.0;

	fp_normalize(fp);
}

/**
 * Multiply the float pair by ten.
 *   @fp: The float pair.
 */

static inline struct hp_t fp_prod(struct hp_t in, double val)
{
	int exp;
	double comp, err, frac;
	struct hp_t out;

	out.val = in.val * val;

	err = out.val;
	frac = frexp(val, &exp);
	comp = ldexp(in.val, exp);
	while(frac != 0.0) {
		if(frac >= 1.0) {
			frac -= 1.0;
			err -= comp;
		}

		comp /= 2.0;
		frac *= 2.0;
	}

	out.err = val * in.err - err;
	fp_normalize(&out);

	return out;
}

#endif
