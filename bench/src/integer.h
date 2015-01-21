#ifndef TYPE_INTEGER_H
#define TYPE_INTEGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/**
 * Integer structure.
 *   @neg: Negative flag.
 *   @len: The length.
 *   @arr: The array.
 */

struct integer_t {
	bool neg;
	unsigned int len;

	unsigned int arr[];
};


/*
 * integer function declarations
 */

struct integer_t *integer_zero();
struct integer_t *integer_new(int val);
struct integer_t *integer_copy(const struct integer_t *integer);
void integer_delete(struct integer_t *integer);

void integer_add(struct integer_t **integer, struct integer_t *val);
void integer_add_uint(struct integer_t **integer, unsigned int val);
void integer_add_ushort(struct integer_t **integer, unsigned short val);

void integer_sub(struct integer_t **integer, struct integer_t *val);

void integer_mul_ushort(struct integer_t **integer, unsigned short val);

unsigned short integer_div_ushort(struct integer_t **integer, unsigned short val);

int integer_cmp(const struct integer_t *left, const struct integer_t *right);

void integer_print(const struct integer_t *integer, FILE *file);
void integer_printf(const char *restrict format, ...);

unsigned int integer_log10(struct integer_t *integer);
struct integer_t *integer_pow_ushort(unsigned short base, unsigned short pow);
void integer_mod(struct integer_t **integer, struct integer_t *mod);
struct integer_t *integer_pow_mod(unsigned short base, unsigned short pow, struct integer_t *mod);

#define iprintf integer_printf

/**
 * Test if an integer is zero.
 *   @integer: The itneger.
 *   &returns: True if zero, false otherwise.
 */

static inline bool integer_iszero(const struct integer_t *integer)
{
	return integer->len == 0;
}

#ifdef __cplusplus
}
#endif

#endif
