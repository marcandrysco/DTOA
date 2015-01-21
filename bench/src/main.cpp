#define restrict __restrict
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <float.h>
#include <unistd.h>
#include "util.h"
#include "integer.h"
#include "search.h"

#include "../../grisu/src/fast-dtoa.h"
using namespace double_conversion;

extern "C" {
#include "../../gay/gay.h"
#include "../../errol/src/errol.h"
}

/**
 * Retrieve the value for a decimal string and exponnet.
 *   @str: The decimal string.
 *   @exp: The exponent.
 *   &returns: The double value.
 */

double getval(const char *str, int32_t exp)
{
	char buf[256];
	double chk;

	sprintf(buf, "0.%se%d", str, exp);
	sscanf(buf, "%lf", &chk);

	return chk;
}


/**
 * Convert decimal to string using Dragon4.
 *   @val: The value.
 *   @clock: The clock cycle performance.
 *   &returns: The string.
 */

char *conv_gay(double val, int *clock)
{
	int tm;
	char *buf, *dup;
	int decpt, sign;

	tm = rdtsc();
	buf = dtoa(val, 0, 12, &decpt, &sign, NULL);
	tm = rdtsc() - tm;
	dup = strdup(buf);
	freedtoa(buf);

	if(clock)
		*clock = tm;

	return dup;
}

/**
 * Convert decimal to string using Grisu.
 *   @val: The value.
 *   @buf: The output buffer.
 *   @clock: The clock cycle performance.
 *   @suc: The success flag.
 *   &returns: The exponent.
 */

int conv_grisu(double val, char *buf, int *clock, int *suc)
{
	bool chk;
	static const int kBufferSize = 100;
	Vector<char> buffer(buf, kBufferSize);
	int length, point;
	int tm;

	memset(buf, 0, kBufferSize);
	tm = rdtsc();
	chk = FastDtoa(val, FAST_DTOA_SHORTEST, 0, buffer, &length, &point);
	tm = rdtsc() - tm;
	if(!chk)
		*buf = '\0';

	if(clock)
		*clock = tm;

	if(suc)
		*suc = chk;

	return point;
}


/**
 * Check if a conversion is correct.
 *   @val: The value.
 *   @str: The string.
 *   @exp: The exponent.
 */

bool chk_correct(double val, const char *str, int32_t exp)
{
	return getval(str, exp) == val;
}

/**
 * Check the shortness of the conversion.
 *   @val: The value.
 *   @str: the string.
 */

bool chk_short(double val, const char *str)
{
	bool chk;
	char *buf;
	int decpt, sign;

	buf = dtoa(val, 0, 12, &decpt, &sign, NULL);
	chk = strlen(buf) == strlen(str);
	freedtoa(buf);

	return chk;
}

/**
 * Check an errol conversion.
 *   @val: The value.
 */

void chk_errol(double val)
{
	static FILE *file = NULL;
	int exp;
	char buf[108];

	if(file == NULL)
		file = fopen("err.list", "w");

	exp = errol_short(val, buf);
	if(!chk_correct(val, buf, exp))
		fprintf(file, "%.18e\n", val), fprintf(stderr, "Errol: Incorrect conversion. Expected '%.17e'. Actual '%.17g'. Str '%se%d'\n", val, getval(buf, exp), buf, exp);
	if(!chk_short(val, buf))
		fprintf(file, "%.18e\n", val), fprintf(stderr, "Errol: Shortness failure. Input '%.17e'. Expected '%s'. Actual '%s'.\n", val, conv_gay(val, NULL), buf);
}


static int sort(const void *left, const void *right)
{
	if(*(const int *)left > *(const int *)right)
		return -1;
	else if(*(const int *)left < *(const int *)right)
		return 1;
	else
		return 0;
}


/**
 * Main entry point.
 *   @argc: The number of argument.
 *   @argv: The argument array.
 *   &returns: The error code.
 */

int main(int argc, char *argv[])
{
	FILE *file;
	double val;
	char buf[108];
	int exp;
	unsigned int i, idx;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	srand(1000000 * (int64_t)tv.tv_sec + (int64_t)tv.tv_usec);

	setbuf(stdout, NULL);

	/* error analysis */
	if(0) {
		unsigned int data[2046][3];

		for(i = 0; i < 2046; i++)
			data[i][0] = data[i][1] = data[i][2] = 0;

		for(i = 0; i < 1000*1000*1000; i++) {
			val = randval();
			//val = 1.9930203808312426e+19;

			frexp(val, &exp);
			idx = (exp < -1024) ? 0 : exp + 1024;

			data[idx][0]++;

			exp = errol_fast(val, buf);
			if(!chk_correct(val, buf, exp))
				fprintf(stderr, "Errol: Incorrect conversion. Expected '%.17e'. Actual '%.17g'. Str '%se%d'\n", val, getval(buf, exp), buf, exp);
			if(!chk_short(val, buf))
				data[idx][1]++;

			exp = conv_grisu(val, buf, NULL, NULL);
			if((*buf != '\0') && !chk_correct(val, buf, exp))
				fprintf(stderr, "Grisu: Incorrect conversion. Expected '%.17g'. Actual '%.17g'. Str '%se%d\n", val, getval(buf, exp), buf, exp);
			if((*buf == '\0') || !chk_short(val, buf))
				data[idx][2]++;
		}

		file = fopen("error.log", "w");

		for(i = 0; i < 2046; i++)
			fprintf(file, "%d\t%u\t%u\t%f\t%u\t%f\n", (int)i - 1021, data[i][0], data[i][1], (double)data[i][1] / data[i][0], data[i][2], (double)data[i][2] / data[i][0]);

		fclose(file);
	}

	/* randomize verification */
	if(0) {
		for(i = 0; i < 100*1000; i++) {
			val = randval();

			exp = errol_short(val, buf);
			if(!chk_correct(val, buf, exp))
				fprintf(stderr, "Errol: Incorrect conversion. Expected '%.17e'. Actual '%.17e'. Str '%se%d'\n", val, getval(buf, exp), buf, exp);
			if(!chk_short(val, buf))
				fprintf(stderr, "Errol: Shortness failure. Input '%.17e'. Expected '%s'. Actual '%s'.\n", val, conv_gay(val, NULL), buf);
		}
	}

	/* check midpoints between 2^128 and 2^137 for errors */
	if(0) {
		int e;
		uint64_t i, v;
		double succ, pred;

		v = 476837158203125; //5^21
		for(i = 1; i*v < ((uint64_t)1<<54); i += 2) {
			if(i*v < ((uint64_t)1<<53))
				continue;

			pred = (double)((i*v-1)/2);
			succ = (double)((i*v+1)/2);

			for(e = 0; e <= 137 - 53; e++) {
				val = ldexp(pred, e);

				exp = errol_short(val, buf);
				if(!chk_correct(val, buf, exp))
					fprintf(stderr, "Errol: Incorrect conversion. Expected '%.17e'. Actual '%.17g'. Str '%se%d'\n", val, getval(buf, exp), buf, exp);
				if(!chk_short(val, buf))
					fprintf(stderr, "Errol: Shortness failure. Input '%.17e'. Expected '%s'. Actual '%s'.\n", val, conv_gay(val, NULL), buf);

				val = ldexp(succ, e);

				exp = errol_short(val, buf);
				if(!chk_correct(val, buf, exp))
					fprintf(stderr, "Errol: Incorrect conversion. Expected '%.17e'. Actual '%.17g'. Str '%se%d'\n", val, getval(buf, exp), buf, exp);
				if(!chk_short(val, buf))
					fprintf(stderr, "Errol: Shortness failure. Input '%.17e'. Expected '%s'. Actual '%s'.\n", val, conv_gay(val, NULL), buf);
			}
		}
	}

	/* raw write speed */
	if(1) {
		static int N = 1000;
		int n;
		for(n = N; n <= 10*N; n += N) {
		int i;
		double *data = (double *)malloc(n*sizeof(double));
		uint64_t tm;
		FILE *file;
		
		for(i = 0; i < n; i++)
			data[i] = randval();

		file = fopen("errol.dat", "w");
		tm = utime();
		for(i = 0; i < n; i++) {
			exp = errol_short(data[i], buf);
			fprintf(file, "0.%se%d\n", buf, exp);
		}
		fflush(file);
		fdatasync(fileno(file));
		tm = utime() - tm;
		fclose(file);

		printf("errol\t%f\t", tm/1000.0);
		//printf("errol: %f ms\n", tm/1000.0);

		file = fopen("grisu.dat", "w");
		tm = utime();
		for(i = 0; i < n; i++) {
			int length, point;
			Vector<char> buffer(buf, sizeof(buf));
			bool chk;

			chk = FastDtoa(data[i], FAST_DTOA_SHORTEST, 0, buffer, &length, &point);
			if(chk)
				fprintf(file, "0.%se%d\n", buf, point);
		}
		fflush(file);
		fdatasync(fileno(file));
		tm = utime() - tm;
		fclose(file);

		printf("grisu\t%f\n", tm/1000.0);

		free(data);
		//printf("%grisu: %f ms\n", tm/1000.0);
		}
	}

	/* performance measurements */
	if(0) {
		static const int N = 10*1000, R = 100, D = R/10;
		int seed = time(NULL);
		static int data[N][4][R];

		for(int i = 0; i < R; i++) {
			srand(seed);

			for(int j = 0; j < N; j++) {
				int tm;
				int suc;
				double val = randval();

				conv_grisu(val, buf, &data[j][1][i], &suc);
				free(conv_gay(val, &data[j][2][i]));
				data[j][3][i] = data[j][1][i] + (suc ? 0 : data[j][2][i]);

				tm = rdtsc();
				errol_short(val, buf);
				tm = rdtsc() - tm;

				data[j][0][i] = tm;
			}
		}

		for(int j = 0; j < N; j++) {
			int total;

			for(int k = 0; k < 4; k++) {
				total = 0;
				qsort(data[j][k], R, sizeof(unsigned int), sort);
				for(i = D; i < R-2*D; i++)
					total += data[j][k][i];

				data[j][k][0] = total;
			}
		}

		uint64_t errol = 0, grisu = 0, gay = 0, adj = 0;

		srand(seed);
		FILE *file = fopen("perf.log", "w");
		for(int j = 0; j < N; j++) {
			double val = randval();
			fprintf(file, "%.20e\t%u\t%u\t%u\t%u\n", val, data[j][0][0], data[j][1][0], data[j][2][0], data[j][3][0]);

			errol += data[j][0][0];
			grisu += data[j][1][0];
			gay += data[j][2][0];
			adj += data[j][3][0];
		}
		fclose(file);

		printf("errol: %lu\n", errol);
		printf("grisu: %lu\n", grisu);
		printf("gay: %lu\n", gay);
		printf("adjusted: %lu\n", adj);
		printf("speedup: %f, %f\n", (grisu - errol) / (double)grisu, (adj - errol) / (double)adj);
	}

	/* upper midpoint neighborhood checking */
	if(0) {
		//for(int i = 137-53; i <= 1023-53; i++)
		for(int i = 137-53; i <= 1023-53; i++)
			search_upper(i);
	}

	/* lower midpoint neighborhood checking */
	if(0) {
		//for(int i = -38; i >= -1022; i--)
		for(int i = -1021; i >= -1021; i--)
			search_lower(-i);
		//search_upper(137);
	}

	return 0;
}
