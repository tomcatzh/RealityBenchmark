#ifndef __REALITY_BENCHMARK_H
#define __REALITY_BENCHMARK_H

#include <sys/time.h>

struct result_data {
  struct timeval real_run;
  double sd;
  unsigned long loops;
  size_t input_bytes;
  size_t output_bytes;
};

typedef struct result_data RESULT;

int loop(CONTENTS* (*run)(const CONTENTS*), const CONTENTS* contents,
             const struct timeval* timeout, RESULT* result);

int thread_loop(CONTENTS *(*run)(const CONTENTS *), const CONTENTS *contents,
                    const struct timeval *timeout, const unsigned int threads,
                    RESULT* results);

#endif