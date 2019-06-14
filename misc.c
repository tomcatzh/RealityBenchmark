#include <string.h>

#include "misc.h"

unsigned long timevalToUsec(const struct timeval *x) {
  return x->tv_sec * 1000000 + x->tv_usec;
}

void timevalAdd(struct timeval *result, const struct timeval *x,
                const struct timeval *y) {
    result->tv_sec = x->tv_sec + y->tv_sec;
    result->tv_usec += x->tv_usec + y->tv_sec;
    if (result->tv_usec > 1000000) {
      result->tv_usec -= 1000000;
      result->tv_sec++;
    }
}

void timevalAddAdd(struct timeval *result, const struct timeval *x) {
  timevalAdd(result, x, result);
}

int timevalSubtract(struct timeval *result, const struct timeval *x,
                            const struct timeval *ya) {
  struct timeval yb;
  memcpy(&yb, ya, sizeof(yb));
  struct timeval *y = &yb;

  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int timevalBeforeTimeout(const struct timeval *timeout,
                          const struct timeval *x, const struct timeval *y) {
  struct timeval r;

  timevalSubtract(&r, x, y);

  if (r.tv_sec < timeout->tv_sec) {
    return 1;
  } else if (r.tv_usec < timeout->tv_usec) {
    return 1;
  }

  return 0;
}

int timevalBefore(const struct timeval *x, const struct timeval *y) {
  struct timeval e;

  e.tv_sec = 0;
  e.tv_usec = 0;

  return timevalBeforeTimeout(&e, x, y);
}