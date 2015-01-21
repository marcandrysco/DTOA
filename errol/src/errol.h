#ifndef ERROL_H
#define ERROL_H

int32_t errol_str(double val, char *buf);
int32_t errol_str2(double val, char *buf);

int32_t errol_alt(double val, char *buf, int8_t off);

/*
 * errol conversion algorithms
 */

int32_t errol_fast(double val, char *buf);
int32_t errol_short(double val, char *buf);

int32_t errol_debug(double val, char *buf);

#endif
