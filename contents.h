#ifndef __ZLIB_BENCH_CONTENTS_H
#define __ZLIB_BENCH_CONTENTS_H

#include <string.h>

struct f_data {
  unsigned char *body;
  size_t size;
};

typedef struct f_data CONTENTS;

CONTENTS *get_contents(const char *url);
void destroy_contents(CONTENTS *file);

#endif