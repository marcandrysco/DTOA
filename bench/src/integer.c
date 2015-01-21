#include "integer.h"


/*
 * bit manipulation definitions
 */

#define ROR(val) ((val) >> (8 * sizeof(unsigned short)))
#define ROL(val) ((val) << (8 * sizeof(unsigned short)))

#define GETLOW(val) ((val) & (unsigned int)(USHRT_MAX))
#define GETHI(val) (ROR(val) & (unsigned int)(USHRT_MAX))

#define SETLOW(val, low) (((val) & ~(unsigned int)(USHRT_MAX)) | (unsigned int)(GETLOW(low)))
#define SETHI(val, hi) (((val) & ~ROL((unsigned int)(USHRT_MAX))) | ROL((unsigned int)(GETLOW(hi))))

/*
 * local function declarations
 */

static void op_resize(struct integer_t **integer, unsigned int len);
static void op_shrink(struct integer_t **integer);
static void op_add(unsigned int *val, unsigned int add, bool *c);
static void op_sub(unsigned int *val, unsigned int sub, bool *c);
static void op_rev(unsigned int *val, unsigned int sub, bool *c);


/**
 * Create a zero integer.
 *   &returns: The zero value.
 */

struct integer_t *integer_zero()
{
	struct integer_t *integer;

	integer = malloc(sizeof(struct integer_t));
	integer->neg = false;
	integer->len = 0;

	return integer;
}

/**
 * Create an integer from a native value.
 *   @val: The native value.
 *   &returns: The integer.
 */

struct integer_t *integer_new(int val)
{
	struct integer_t *integer;

	integer = malloc(sizeof(struct integer_t) + sizeof(unsigned int));
	integer->len = 1;

	if(val >= 0) {
		integer->neg = false;
		integer->arr[0] = val;
	}
	else {
		integer->neg = true;
		integer->arr[0] = -val;
	}

	return integer;
}

/**
 * Copy an integer.
 *   @integer: The integer.
 *   &returns: The copy.
 */

struct integer_t *integer_copy(const struct integer_t *integer)
{
	struct integer_t *copy;

	copy = malloc(sizeof(struct integer_t) + integer->len * sizeof(unsigned int));
	copy->neg = integer->neg;
	copy->len = integer->len;
	memcpy(copy->arr, integer->arr, integer->len * sizeof(unsigned int));

	return copy;
}

/**
 * Delete an integer.
 *   @integer: The integer.
 */

void integer_delete(struct integer_t *integer)
{
	free(integer);
}


/**
 * Add an integer to a base integer.
 *   @integer: The base integer.
 *   @val: The value integer to add.
 */

void integer_add(struct integer_t **integer, struct integer_t *val)
{
	if(!(*integer)->neg && val->neg) {
		val->neg = false;
		integer_sub(integer, val);
		val->neg = true;
	}
	else if((*integer)->neg && !val->neg) {
		(*integer)->neg = false;
		integer_sub(integer, val);
		(*integer)->neg = !(*integer)->neg;
	}
	else {
		bool c;
		unsigned int i;

		if(val->len > (*integer)->len)
			op_resize(integer, val->len);

		for(i = 0, c = false; (i < val->len) || (c && (i < (*integer)->len)); i++)
			op_add(&(*integer)->arr[i], (i < val->len) ? val->arr[i] : 0, &c);

		if(c) {
			*integer = realloc(*integer, sizeof(struct integer_t) + (i + 1) * sizeof(unsigned int));
			(*integer)->len = i + 1;
			(*integer)->arr[i] = 1;
		}
	}
}

/**
 * Add an unsigned integer to the integer.
 *   @integer: The integer.
 *   @val: The value to add.
 */

void integer_add_uint(struct integer_t **integer, unsigned int val)
{
	unsigned int i, t;

	for(i = 0; i < (*integer)->len; i++) {
		t = (*integer)->arr[i];
		(*integer)->arr[i] += val;

		if((*integer)->arr[i] >= t)
			return;

		val = 1;
	}

	if(val > 0) {
		*integer = realloc(*integer, sizeof(struct integer_t) + (i + 1) * sizeof(unsigned int));
		(*integer)->len = i + 1;
		(*integer)->arr[i] = val;
	}
}

/**
 * Add an unsigned short to the integer.
 *   @integer: The integer.
 *   @val: The value to add.
 */

void integer_add_ushort(struct integer_t **integer, unsigned short val)
{
	integer_add_uint(integer, val);
}


/**
 * Subtract an integer to a base integer.
 *   @integer: The base integer.
 *   @val: The value integer to subtract.
 */

void integer_sub(struct integer_t **integer, struct integer_t *val)
{
	if(val->neg) {
		val->neg = false;
		integer_add(integer, val);
		val->neg = true;
	}
	else if(integer_cmp(*integer, val) >= 0) {
		bool c;
		unsigned int i;

		for(i = 0, c = false; (i < val->len) || (c && (i < (*integer)->len)); i++)
			op_sub(&(*integer)->arr[i], (i < val->len) ? val->arr[i] : 0, &c);

		op_shrink(integer);
	}
	else {
		bool c;
		unsigned int i;

		op_resize(integer, val->len);

		for(i = 0, c = false; i < (*integer)->len; i++)
			op_rev(&(*integer)->arr[i], (i < val->len) ? val->arr[i] : 0, &c);

		(*integer)->neg = true;
		op_shrink(integer);
	}
}


/**
 * Multiply the integer by an unsigned short.
 *   @integer: The integer.
 *   @val: The value to multiply.
 */

void integer_mul_ushort(struct integer_t **integer, unsigned short val)
{
	unsigned int i, mul, carry = 0;

	for(i = 0; i < (*integer)->len; i++) {
		mul = GETLOW((*integer)->arr[i]) * val + carry;
		(*integer)->arr[i] = SETLOW((*integer)->arr[i], mul);
		carry = GETHI(mul);

		mul = GETHI((*integer)->arr[i]) * val + carry;
		(*integer)->arr[i] = SETHI((*integer)->arr[i], mul);
		carry = GETHI(mul);
	}

	if(carry > 0) {
		*integer = realloc(*integer, sizeof(struct integer_t) + (i + 1) * sizeof(unsigned int));
		(*integer)->len = i + 1;
		(*integer)->arr[i] = carry;
	}
}


/**
 * Divide the integer by an unsigned short.
 *   @integer: The integer.
 *   @val: The value to divide.
 *   &returns: The remainder
 */

unsigned short integer_div_ushort(struct integer_t **integer, unsigned short val)
{
	unsigned int i, rem = 0;

	for(i = (*integer)->len - 1; i != UINT_MAX; i--) {
		rem = ROL(rem) + GETHI((*integer)->arr[i]);
		(*integer)->arr[i] = SETHI((*integer)->arr[i], rem / val);
		rem %= val;

		rem = ROL(rem) + GETLOW((*integer)->arr[i]);
		(*integer)->arr[i] = SETLOW((*integer)->arr[i], rem / val);
		rem %= val;
	}

	for(i = (*integer)->len - 1; i != UINT_MAX; i--) {
		if((*integer)->arr[i] > 0)
			break;
	}

	if(++i != (*integer)->len) {
		*integer = realloc(*integer, sizeof(struct integer_t) + i * sizeof(unsigned int));
		(*integer)->len = i;
	}

	return rem;
}


/**
 * Compare two integer.
 *   @left: The left integer.
 *   @right: The right integer.
 *   &returns: Their order.
 */

int integer_cmp(const struct integer_t *left, const struct integer_t *right)
{
	unsigned int i;

	if(left->len > right->len)
		return 2;
	else if(left->len < right->len)
		return -2;

	for(i = left->len - 1; i != UINT_MAX; i--) {
		if(left->arr[i] > right->arr[i])
			return 1;
		else if(left->arr[i] < right->arr[i])
			return -1;
	}

	return 0;
}



/**
 * Print an integer to the output.
 *   @integer: The integer.
 *   @file: The file.
 */

void integer_print(const struct integer_t *integer, FILE *file)
{
	struct integer_t *copy;
	char *str, buf[3 * integer->len * sizeof(unsigned int) + 1];

	str = buf + sizeof(buf) - 1;

	copy = integer_copy(integer);

	while(!integer_iszero(copy))
		*str-- = '0' + integer_div_ushort(&copy, 10);

	integer_delete(copy);

	*str-- = (integer->neg ? '-' : '+');
	fwrite(str+1, sizeof(buf) - (str - buf + 1), 1, file);
}

/**
 * Print formatted text, including arbitrary size integer.
 *   @format: The format string.
 *   @...: The printf-style arguments.
 */

void integer_printf(const char *restrict format, ...)
{
	va_list args;

	va_start(args, format);

	while(*format != '\0') {
		if(*format == '%') {
			format++;

			if(*format == 'I')
				integer_print(va_arg(args, struct integer_t *), stdout);
			else if(*format == 'u')
				vfprintf(stdout, "%u", args);
			else if(*format == 'l')
				vfprintf(stdout, "%lu", args);
		}
		else
			fputc(*format, stdout);

		format++;
	}

	va_end(args);
}



/**
 * Resize an integer to a given length, zero filling all new elements.
 *   @integer: The integer.
 *   @len: The length.
 */

static void op_resize(struct integer_t **integer, unsigned int len)
{
	unsigned int i;

	*integer = realloc(*integer, sizeof(struct integer_t) + len * sizeof(unsigned int));

	for(i = (*integer)->len; i < len; i++)
		(*integer)->arr[i] = 0;

	(*integer)->len = len;
}

/**
 * Shrink the integer to the minimum necessary size.
 *   @integer: The integer.
 */

static void op_shrink(struct integer_t **integer)
{
	unsigned int i;

	for(i = (*integer)->len; i > 0; i--) {
		if((*integer)->arr[i-1] != 0)
			break;
	}

	if(i < (*integer)->len) {
		*integer = realloc(*integer, sizeof(struct integer_t) + i * sizeof(unsigned int));
		(*integer)->len = i;
	}
}

/**
 * Perform an add with carry on a single digit.
 *   @val: The base value.
 *   @add: The value to add.
 *   @c: The carry flag.
 */

static void op_add(unsigned int *val, unsigned int add, bool *c)
{
	unsigned int t;

	t = *val;
	*val += add + (*c ? 1 : 0);
	*c = (*val < t) || ((*val == t) && *c);
}

/**
 * Perform a subtract with carry on a single digit.
 *   @val: The base value.
 *   @sub: The value to subract.
 *   @c: The carry flag.
 */

static void op_sub(unsigned int *val, unsigned int sub, bool *c)
{
	unsigned int t;

	t = *val;
	*val -= sub + (*c ? 1 : 0);
	*c = (*val > t) || ((*val == t) && *c);
}

/**
 * Perform a reverse subtract with carry on a single digit.
 *   @val: The base value.
 *   @sub: The value to subract from.
 *   @c: The carry flag.
 */

static void op_rev(unsigned int *val, unsigned int sub, bool *c)
{
	unsigned int t;

	t = sub;
	*val = sub - (*val + (*c ? 1 : 0));
	*c = (*val > t) || ((*val == t) && *c);
}




unsigned int integer_log10(struct integer_t *integer)
{
	unsigned int log;

	for(log = 0; !integer_iszero(integer); log++)
		integer_div_ushort(&integer, 10);

	return log;
}

struct integer_t *integer_pow_ushort(unsigned short base, unsigned short pow)
{
	struct integer_t *v;

	v = integer_new(1);

	for(pow--; pow != USHRT_MAX; pow--)
		integer_mul_ushort(&v, base);

	return v;
}

void integer_mod(struct integer_t **integer, struct integer_t *mod)
{
	while(integer_cmp(*integer, mod) >= 0)
		integer_sub(integer, mod);
}

struct integer_t *integer_pow_mod(unsigned short base, unsigned short pow, struct integer_t *mod)
{
	struct integer_t *v;

	v = integer_new(1);

	for(pow--; pow != USHRT_MAX; pow--) {
		integer_mul_ushort(&v, base);
		integer_mod(&v, mod);
	}

	return v;
}
