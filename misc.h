#ifndef __ZLIB_BENCH_MISC_H
#define __ZLIB_BENCH_MISC_H

#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>

int timevalSubtract(struct timeval *result, const struct timeval *x,
                    const struct timeval *ya);
void timevalAdd(struct timeval *result, const struct timeval *x,
                const struct timeval *y);
void timevalAddAdd(struct timeval *result, const struct timeval *x);

int timevalBefore(const struct timeval *x, const struct timeval *y);
int timevalBeforeTimeout(const struct timeval *timeout,
                          const struct timeval *x, const struct timeval *y);

unsigned long timevalToUsec(const struct timeval *x);

uintmax_t parseHumanSize (const char* s);

#endif