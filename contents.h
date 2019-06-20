#ifndef __ZLIB_BENCH_CONTENTS_H
#define __ZLIB_BENCH_CONTENTS_H

#include <string.h>

struct f_data {
  unsigned char *body;
  size_t size;
};

typedef struct f_data CONTENTS;

CONTENTS *getContents(const char *url);
CONTENTS *randomContents(const size_t size);

int destroyContents(CONTENTS *file);
CONTENTS *cloneContents(CONTENTS *source);
int compareContents(const CONTENTS *x, const CONTENTS *y);

#endif