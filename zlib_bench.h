#ifndef __ZLIB_BENCH_H
#define __ZLIB_BENCH_H

#include <sys/time.h>

CONTENTS* deflate_content(const CONTENTS* plain, const int level);
CONTENTS* deflate_content_best_speed(const CONTENTS* plain);
CONTENTS* deflate_content_best_compression(const CONTENTS* plain);
CONTENTS* deflate_content_default_compression(const CONTENTS* plain);

CONTENTS* inflate_content(const CONTENTS* compressed);

struct result_data {
  struct timeval real_run;
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