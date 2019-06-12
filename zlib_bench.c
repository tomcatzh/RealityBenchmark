#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <zlib.h>

#include "contents.h"
#include "benchmark.h"
#include "misc.h"

#include "zlib_bench.h"

CONTENTS *deflateContent(const CONTENTS *data, const int level) {
  assert(data != NULL);
  assert(data->body != NULL);
  assert(data->size > 0);

  int ret;
  size_t bufSize = 0;

  CONTENTS *result = calloc(1, sizeof(CONTENTS));
  assert(result);

  z_stream *strm = (z_stream *)calloc(1, sizeof(z_stream));
  assert(strm);

  ret = deflateInit(strm, level);
  assert(ret == Z_OK);

  strm->avail_in = data->size;
  strm->next_in = data->body;

  do {
    if (strm->avail_out == 0) {
      unsigned char *ptr =
        (unsigned char *)realloc(result->body, bufSize + data->size);
      assert(ptr);

      result->body = ptr;
      strm->next_out = ptr + bufSize;
      strm->avail_out = data->size;
      bufSize += data->size;
    }
    
    ret = deflate(strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
  } while (strm->avail_in > 0);

  result->size = strm->total_out;

  (void)deflateEnd(strm);
  free(strm);

  return result;
}

CONTENTS *deflateContentBestSpeed(const CONTENTS *plain) {
  return deflateContent(plain, Z_BEST_SPEED);
}

CONTENTS *deflateContentBestCompression(const CONTENTS *plain) {
  return deflateContent(plain, Z_BEST_COMPRESSION);
}

CONTENTS *deflateContentDefaultCompression(const CONTENTS *plain) {
  return deflateContent(plain, Z_DEFAULT_COMPRESSION);
}

CONTENTS *inflateContent(const CONTENTS *data) {
  assert(data != NULL);
  assert(data->body != NULL);
  assert(data->size > 0);

  int ret;
  size_t bufSize = 0;

  CONTENTS *result = calloc(1, sizeof(CONTENTS));
  assert(result);

  z_stream *strm = (z_stream *)calloc(1, sizeof(z_stream));
  assert(strm);

  ret = inflateInit(strm);
  assert(ret == Z_OK);

  strm->avail_in = data->size;
  strm->next_in = data->body;

   do {
    if (strm->avail_out == 0) {
      unsigned char *ptr =
          (unsigned char *)realloc(result->body, bufSize + data->size * 5);
      assert(ptr);

      result->body = ptr;
      strm->next_out = ptr + bufSize;
      strm->avail_out = data->size * 5;
      bufSize += data->size * 5;
    }

    ret = inflate(strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
  } while (strm->avail_in > 0);

  result->size = strm->total_out;

  (void)inflateEnd(strm);
  free(strm);

  return result;
}

