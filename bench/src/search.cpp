#define restrict __restrict
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <float.h>
#include "util.h"
#include "integer.h"
#include "avltree.h"


/*
 * check function
 */

void chk_errol(double val);


/**
 * Jump structure.
 *   @idx: The shift index.
 *   @integer: The shift value.
 */

struct shift_t {
	uint64_t idx;
	struct integer_t *val;
};


/**
 * Create a new shift.
 *   @idx: The index.
 *   @val: The value.
 *   &returns: The shift.
 */

struct shift_t *shift_new(uint64_t idx, struct integer_t *val)
{
	struct shift_t *shift;

	shift = (struct shift_t *)malloc(sizeof(struct shift_t));
	shift->idx = idx;
	shift->val = val;

	return shift;
}

/**
 * Delete a shift.
 *   @shift: The shift.
 */

void shift_delete(struct shift_t *shift)
{
	integer_delete(shift->val);
	free(shift);
}


struct integer_t *integer_inv(struct integer_t *in, struct integer_t *mod)
{
	struct integer_t *out = integer_copy(in);

	integer_sub(&out, mod);

	return out;
}

/**
 * Search an upper range exponent range for a failure.
 */

void search_upper(int exp)
{
	struct integer_t *zero = integer_zero();
	struct integer_t *one = integer_new(1);

	printf("exp: %u\n", exp);

	/* calculate the modulus */

	unsigned int fact = log10(pow(2, exp + 53)) - 16;
	exp -= fact;
	struct integer_t *mod = integer_pow_ushort(5, fact);

	/* generate the first offset */

	struct integer_t *init = integer_pow_mod(2, exp + 1, mod);

	iprintf("init: %I\n", init);
	/* create the initial shift lists */

	struct avltree_t uplist = avltree_empty((compare_f)integer_cmp, (delete_f)shift_delete);
	struct avltree_t downlist = avltree_empty((compare_f)integer_cmp, (delete_f)shift_delete);

	struct shift_t *up = shift_new(1, integer_copy(init));
	avltree_insert(&uplist, up->val, up);

	struct shift_t *down = shift_new(1, integer_inv(init, mod));
	avltree_insert(&downlist, down->val, down);

	// generate optimal shift lists
	while(true) {
		//get the last elements of each list */
		up = (struct shift_t *)avltree_first(&uplist);
		down = (struct shift_t *)avltree_first(&downlist);

		//printf("idx: %u , %u\n", up->idx, down->idx);
		if(up->idx >= (1ul << 52) && down->idx >= (1ul << 52))
			break;

		// calculate the dist-1 between the first values of each list
		// note that the desired jump has at most magnitude dist-1
		struct integer_t *dist = integer_copy(up->val);
		integer_sub(&dist, down->val);
		integer_sub(&dist, one);

		// check if the up shift list is shorter
		if(up->idx <= down->idx) {
			// select the best down shift
			struct shift_t *best = (struct shift_t *)avltree_atmost(&downlist, dist);

			// calculate the next shift
			struct integer_t *next = integer_copy(up->val);
			integer_add(&next, best->val);

			// zeros are bad errors
			if(integer_iszero(next))
				fprintf(stderr, "Computed zero. idx: %lu :(\n", up->idx + best->idx), abort();

			// determine which list to add to
			struct avltree_t *sel = next->neg ? &downlist : &uplist;
			if(avltree_lookup(sel, next) != NULL)
				continue;//fprintf(stderr, "Computed duplicate shift."), abort();

			// create and add shift
			struct shift_t *shift = shift_new(up->idx + best->idx, next);
			avltree_insert(sel, shift->val, shift);
		}

		// check if the down shift list is shorter

		if(down->idx < up->idx) {
			// select the best down shift
			struct shift_t *best = (struct shift_t *)avltree_atmost(&uplist, dist);

			// calculate the next shift
			struct integer_t *next = integer_copy(down->val);
			integer_add(&next, best->val);

			// zeros are bad errors
			if(integer_iszero(next))
				fprintf(stderr, "Computed zero. :(\n"), abort();

			// determine which list to add to
			struct avltree_t *sel = next->neg ? &downlist : &uplist;
			if(avltree_lookup(sel, next) != NULL)
				continue;//fprintf(stderr, "Computed duplicate shift."), abort();

			// create and add shift
			struct shift_t *shift = shift_new(down->idx + best->idx, next);
			avltree_insert(sel, shift->val, shift);
			iprintf("add2 %u %I\n", shift->idx, shift->val);
		}

		/* cleanup */

		integer_delete(dist);
	}

	// the running index
	uint64_t idx = 0;

	// create the first midpoint
	struct integer_t *first = integer_pow_mod(2, exp + 53, mod);
	struct integer_t *mid = integer_pow_mod(2, exp, mod);
	integer_add(&mid, first);
	integer_mod(&mid, mod);

	// the first shift might not be large enough
	// ensure we take a many idx=1 shifts to get there
	struct shift_t *shift = (struct shift_t *)avltree_last(mid->neg ? &uplist : &downlist);
	while(integer_cmp(shift->val, mid) <= 0) {
		idx += shift->idx;
		integer_add(&mid, shift->val);
	}

	// calculate allowable error
	struct integer_t *err = integer_pow_ushort(2, exp-48);

	// iterate to find a minimal midpoint
	while(true) {
		shift = (struct shift_t *)avltree_atleast(mid->neg ? &uplist : &downlist, mid);
		if(shift == NULL)
			break;
		else if((idx + shift->idx) >= (1l << 52))
			break;

		idx += shift->idx;
		integer_add(&mid, shift->val);

		if(integer_cmp(mid, err) < 0)
			break;
	}

	// we detected that a failure may occur
	if(idx < (1l << 52)) {
		// list of possible failures and values to check
		struct avltree_t errlist = avltree_empty((compare_f)integer_cmp, (delete_f)shift_delete);
		struct avltree_t chklist = avltree_empty((compare_f)integer_cmp, NULL);

		shift = shift_new(idx, integer_copy(mid));
		avltree_insert(&errlist, shift->val, shift);
		avltree_insert(&chklist, shift->val, shift);

		while(chklist.count > 0) {
			struct avltree_iter_t iter;
			struct shift_t *base = (struct shift_t *)avltree_last(&chklist);
			avltree_remove(&chklist, base->val);

			iter = avltree_iter_begin(&uplist);
			while((shift = (struct shift_t *)avltree_iter_next(&iter)) != NULL) {
				if(base->idx + shift->idx >= (1l << 52))
					continue;

				struct integer_t *sum = integer_copy(base->val);
				integer_add(&sum, shift->val);
				if(integer_cmp(sum, err) < 0) {
					if(avltree_lookup(&errlist, sum) != NULL)
						continue;

					shift = shift_new(base->idx + shift->idx, integer_copy(sum));
					avltree_insert(&errlist, shift->val, shift);
					avltree_insert(&chklist, shift->val, shift);
				}
				else {
					integer_delete(sum);
					break;
				}
			}

			iter = avltree_iter_begin(&downlist);
			while((shift = (struct shift_t *)avltree_iter_next(&iter)) != NULL) {
				if(base->idx + shift->idx >= (1l << 52))
					continue;

				struct integer_t *sum = integer_copy(base->val);
				integer_add(&sum, shift->val);
				if(integer_cmp(sum, err) < 0) {
					if(avltree_lookup(&errlist, sum) != NULL)
						continue;

					shift = shift_new(base->idx + shift->idx, integer_copy(sum));
					avltree_insert(&errlist, shift->val, shift);
					avltree_insert(&chklist, shift->val, shift);
				}
				else {
					integer_delete(sum);
					break;
				}
			}
		}

		struct avltree_iter_t iter = avltree_iter_begin(&errlist);
		while((shift = (struct shift_t *)avltree_iter_next(&iter)) != NULL) {
			double pred = ldexp((double)(shift->idx + 0) + (double)(1l << 52), exp + fact + 1);
			double succ = ldexp((double)(shift->idx + 1) + (double)(1l << 52), exp + fact + 1);
			chk_errol(pred);
			chk_errol(succ);
		}

		avltree_destroy(&errlist);
		avltree_destroy(&chklist);
	}


	exp += fact;
	iprintf("mid: %I\n", mid);
	iprintf("val: 2^%u + 2^%u + %l*2^%u\n", exp+53, exp, idx, exp+1);
	//iprintf("%l %I\n", idx, mid);

	//


	/* cleanup */

	integer_delete(zero);
	integer_delete(one);
	integer_delete(init);
	integer_delete(first);
	integer_delete(mid);
	integer_delete(err);
	avltree_destroy(&uplist);
	avltree_destroy(&downlist);
}

/**
 * Search an lower range exponent range for a failure.
 */

void search_lower(int exp)
{
	struct integer_t *zero = integer_zero();
	struct integer_t *one = integer_new(1);

	printf("exp %d\n", exp);
	// minimum number of factors 2 and 5
	unsigned int fact = 54*log10(2) + exp*log10(5) - 16;

	// maximum delta from zero
	double delta = pow(2, -48) * pow(5, exp - fact);

	// cannot have non-integer deltas
	if(delta < 1.0)
		return;

	/* calculate the modulus */

	struct integer_t *mod = integer_pow_ushort(2, fact);

	/* generate the first offset */

	struct integer_t *init = integer_pow_mod(5, exp - fact, mod);
	integer_mul_ushort(&init, 2);
	integer_mod(&init, mod);
	/* create the initial shift lists */

	struct avltree_t uplist = avltree_empty((compare_f)integer_cmp, (delete_f)shift_delete);
	struct avltree_t downlist = avltree_empty((compare_f)integer_cmp, (delete_f)shift_delete);

	struct shift_t *up = shift_new(1, integer_copy(init));
	avltree_insert(&uplist, up->val, up);

	struct shift_t *down = shift_new(1, integer_inv(init, mod));
	avltree_insert(&downlist, down->val, down);

	// generate optimal shift lists
	while(true) {
		//get the last elements of each list */
		up = (struct shift_t *)avltree_first(&uplist);
		down = (struct shift_t *)avltree_first(&downlist);

		//printf("idx: %u , %u\n", up->idx, down->idx);
		if(up->idx >= (1ul << 52) && down->idx >= (1ul << 52))
			break;

		// calculate the dist-1 between the first values of each list
		// note that the desired jump has at most magnitude dist-1
		struct integer_t *dist = integer_copy(up->val);
		integer_sub(&dist, down->val);
		integer_sub(&dist, one);

		// check if the up shift list is shorter
		if(up->idx <= down->idx) {
			// select the best down shift
			struct shift_t *best = (struct shift_t *)avltree_atmost(&downlist, dist);
			if(best == NULL)
				break;

			// calculate the next shift
			struct integer_t *next = integer_copy(up->val);
			integer_add(&next, best->val);

			// zero is the smallest jump possible
			if(integer_iszero(next)) {
				struct shift_t *shift;
				
				shift = shift_new(up->idx + best->idx, integer_copy(zero));
				avltree_insert(&downlist, shift->val, shift);
				
				shift = shift_new(up->idx + best->idx, integer_copy(zero));
				avltree_insert(&uplist, shift->val, shift);

				break;
			}

			// determine which list to add to
			struct avltree_t *sel = next->neg ? &downlist : &uplist;
			if(avltree_lookup(sel, next) != NULL)
				continue;//fprintf(stderr, "Computed duplicate shift."), abort();

			// create and add shift
			struct shift_t *shift = shift_new(up->idx + best->idx, next);
			avltree_insert(sel, shift->val, shift);
		}

		// check if the down shift list is shorter

		if(down->idx < up->idx) {
			// select the best down shift
			struct shift_t *best = (struct shift_t *)avltree_atmost(&uplist, dist);
			if(best == NULL)
				break;

			// calculate the next shift
			struct integer_t *next = integer_copy(down->val);
			integer_add(&next, best->val);

			// zero is the smallest jump possible
			if(integer_iszero(next)) {
				struct shift_t *shift;
				
				shift = shift_new(down->idx + best->idx, integer_copy(zero));
				avltree_insert(&downlist, shift->val, shift);
				
				shift = shift_new(down->idx + best->idx, integer_copy(zero));
				avltree_insert(&uplist, shift->val, shift);

				break;
			}

			// determine which list to add to
			struct avltree_t *sel = next->neg ? &downlist : &uplist;
			if(avltree_lookup(sel, next) != NULL)
				continue;//fprintf(stderr, "Computed duplicate shift."), abort();

			// create and add shift
			struct shift_t *shift = shift_new(down->idx + best->idx, next);
			avltree_insert(sel, shift->val, shift);
		}

		/* cleanup */

		integer_delete(dist);
	}

	// the running index
	uint64_t idx = 0;

	// create the first midpoint
	struct integer_t *mid = integer_pow_mod(2, 54, mod);
	struct integer_t *a = integer_pow_mod(5, exp - fact, mod);
	for(uint64_t i = 0; i < exp - fact; i++) {
		integer_mul_ushort(&mid, 5);
		integer_mod(&mid, mod);
	}
	integer_add(&mid, a);
	integer_mod(&mid, mod);

	//iprintf("first:

	// the first shift might not be large enough
	// ensure we take a many idx=1 shifts to get there
	struct shift_t *shift = (struct shift_t *)avltree_last(mid->neg ? &uplist : &downlist);
	while(integer_cmp(shift->val, mid) <= 0) {
		idx += shift->idx;
		integer_add(&mid, shift->val);
	}

	// calculate allowable error
	struct integer_t *err = integer_pow_ushort(5, exp - fact);
	for(int i = 0; i < 48; i++)
		integer_div_ushort(&err, 2);

	// iterate to find a minimal midpoint
	while(true) {
		shift = (struct shift_t *)avltree_atleast(mid->neg ? &uplist : &downlist, mid);
		if(shift == NULL)
			break;
		else if((idx + shift->idx) >= (1l << 52))
			break;

		idx += shift->idx;
		integer_add(&mid, shift->val);

		if(integer_cmp(mid, err) < 0)
			break;
	}

	// we detected that a failure may occur
	if(idx < (1l << 52)) {
		// list of possible failures and values to check
		struct avltree_t errlist = avltree_empty((compare_f)integer_cmp, (delete_f)shift_delete);
		struct avltree_t chklist = avltree_empty((compare_f)integer_cmp, NULL);

		shift = shift_new(idx, integer_copy(mid));
		avltree_insert(&errlist, shift->val, shift);
		avltree_insert(&chklist, shift->val, shift);

		while(chklist.count > 0) {
			struct avltree_iter_t iter;
			struct shift_t *base = (struct shift_t *)avltree_last(&chklist);
			avltree_remove(&chklist, base->val);

			iter = avltree_iter_begin(&uplist);
			while((shift = (struct shift_t *)avltree_iter_next(&iter)) != NULL) {
				if(base->idx + shift->idx >= (1l << 52))
					continue;

				struct integer_t *sum = integer_copy(base->val);
				integer_add(&sum, shift->val);
				if(integer_cmp(sum, err) <= 0) {
					if(avltree_lookup(&errlist, sum) != NULL)
						continue;

					shift = shift_new(base->idx + shift->idx, integer_copy(sum));
					avltree_insert(&errlist, shift->val, shift);
					avltree_insert(&chklist, shift->val, shift);
				}
				else {
					integer_delete(sum);
					break;
				}
			}

			iter = avltree_iter_begin(&downlist);
			while((shift = (struct shift_t *)avltree_iter_next(&iter)) != NULL) {
				if(base->idx + shift->idx >= (1l << 52))
					continue;

				struct integer_t *sum = integer_copy(base->val);
				integer_add(&sum, shift->val);
				if(integer_cmp(sum, err) <= 0) {
					if(avltree_lookup(&errlist, sum) != NULL)
						continue;

					shift = shift_new(base->idx + shift->idx, integer_copy(sum));
					avltree_insert(&errlist, shift->val, shift);
					avltree_insert(&chklist, shift->val, shift);
				}
				else {
					integer_delete(sum);
					break;
				}
			}
		}

		struct avltree_iter_t iter = avltree_iter_begin(&errlist);
		while((shift = (struct shift_t *)avltree_iter_next(&iter)) != NULL) {
			iprintf("val: %I\n", shift->val);
			printf("here! 2^%d + 2^%d + %lu*2^%d\n", -exp + 54, -exp, shift->idx, -exp + 1);
			double pred = ldexp((double)(shift->idx + 0) + (double)(1l << 53), -exp + 1);
			double succ = ldexp((double)(shift->idx + 1) + (double)(1l << 53), -exp + 1);
			chk_errol(pred);
			chk_errol(succ);
		}

		avltree_destroy(&errlist);
		avltree_destroy(&chklist);
	}


	//exp += fact;
	//iprintf("mid: %I\n", mid);
	//iprintf("val: 2^%u + 2^%u + %l*2^%u\n", exp+53, exp, idx, exp+1);
	//iprintf("%l %I\n", idx, mid);

	//


	/* cleanup */

	integer_delete(zero);
	integer_delete(one);
	integer_delete(init);
	integer_delete(mid);
	integer_delete(err);
	avltree_destroy(&uplist);
	avltree_destroy(&downlist);
}
