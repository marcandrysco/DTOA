#ifndef UTIL_H
#define UTIL_H


#include <stdint.h>
#include <math.h>


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

/**
 * Create a random double value. The random value is always positive, non-zero, not
 * infinity, and non-NaN.
 *   &returns: The random double.
 */

static inline double randval()
{
	union {
		double d;
		uint16_t arr[4];
	} val;

	do {
		val.arr[0] = rand();
		val.arr[1] = rand();
		val.arr[2] = rand();
		val.arr[3] = rand() & ~(0x8000);
	} while(isnan(val.d) || (val.d == 0.0) || !isfinite(val.d));

	return val.d;
}

static inline uint64_t utime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_usec + 1000000 * tv.tv_sec;
}

#endif
