#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <zlib.h>

#include "contents.h"
#include "benchmark.h"
#include "misc.h"

#include "zlib_bench.h"

CONTENTS *deflate_content(const CONTENTS *data, const int level) {
  assert(data != NULL);
  assert(data->body != NULL);
  assert(data->size > 0);

  int ret;
  size_t bufSize = 0;

  CONTENTS *result = NULL;
  z_stream *strm = NULL;

  result = calloc(1, sizeof(CONTENTS));
  if (!result) {
    errno = ENOMEM;
    goto ERROR_END;
  }

  strm = (z_stream *)calloc(1, sizeof(z_stream));
  if (!strm) {
    errno = ENOMEM;
    goto ERROR_END;
  }

  ret = deflateInit(strm, level);
  if (ret != Z_OK) {
    goto ERROR_END;
  }

  strm->avail_in = data->size;
  strm->next_in = data->body;

  do {
    if (strm->avail_out == 0) {
      unsigned char *ptr =
        (unsigned char *)realloc(result->body, bufSize + data->size);
      if (ptr == NULL) {
        errno = ENOMEM;
        goto ERROR_END;
      }

      result->body = ptr;
      strm->next_out = ptr + bufSize;
      strm->avail_out = data->size;
      bufSize += data->size;
    }
    
    ret = deflate(strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
  } while (strm->avail_in > 0);

  result->size = strm->total_out;
  goto END;

ERROR_END:
  if (result) {
    destroy_contents(result);
    result = NULL;
  }
END:
  if (strm) {
    (void)deflateEnd(strm);
    free(strm);
    strm = NULL;
  }
  return result;
}

CONTENTS *deflate_content_best_speed(const CONTENTS *plain) {
  return deflate_content(plain, Z_BEST_SPEED);
}

CONTENTS *deflate_content_best_compression(const CONTENTS *plain) {
  return deflate_content(plain, Z_BEST_COMPRESSION);
}

CONTENTS *deflate_content_default_compression(const CONTENTS *plain) {
  return deflate_content(plain, Z_DEFAULT_COMPRESSION);
}

CONTENTS *inflate_content(const CONTENTS *data) {
  assert(data != NULL);
  assert(data->body != NULL);
  assert(data->size > 0);

  int ret;
  size_t bufSize = 0;

  CONTENTS *result = NULL;
  z_stream *strm = NULL;

  result = calloc(1, sizeof(CONTENTS));
  if (!result) {
    errno = ENOMEM;
    goto ERROR_END;
  }

  strm = (z_stream *)calloc(1, sizeof(z_stream));
  if (!strm) {
    errno = ENOMEM;
    goto ERROR_END;
  }

  ret = inflateInit(strm);
  if (ret != Z_OK) {
    goto ERROR_END;
  }

  strm->avail_in = data->size;
  strm->next_in = data->body;

   do {
    if (strm->avail_out == 0) {
      unsigned char *ptr =
          (unsigned char *)realloc(result->body, bufSize + data->size * 5);
      if (ptr == NULL) {
        errno = ENOMEM;
        goto ERROR_END;
      }

      result->body = ptr;
      strm->next_out = ptr + bufSize;
      strm->avail_out = data->size * 5;
      bufSize += data->size * 5;
    }

    ret = inflate(strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
  } while (strm->avail_in > 0);

  result->size = strm->total_out;
  goto END;

ERROR_END:
  if (result) {
    destroy_contents(result);
    result = NULL;
  }
END:
  if (strm) {
    (void)inflateEnd(strm);
    free(strm);
    strm = NULL;
  }
  return result;
}

