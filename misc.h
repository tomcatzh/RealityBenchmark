#ifndef __ZLIB_BENCH_MISC_H
#define __ZLIB_BENCH_MISC_H

#include <stdlib.h>

int timeval_subtract(struct timeval *result, const struct timeval *x,
                    const struct timeval *ya);
void timeval_add(struct timeval *result, const struct timeval *x,
                const struct timeval *y);
void timeval_addadd(struct timeval *result, const struct timeval *x);

int timeval_before(const struct timeval *x, const struct timeval *y);
int timeval_before_timeout(const struct timeval *timeout,
                          const struct timeval *x, const struct timeval *y);

unsigned long to_usec(struct timeval *x);

#endif