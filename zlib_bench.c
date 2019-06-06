#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <zlib.h>
#include <pthread.h>

#include "contents.h"
#include "zlib_bench.h"
#include "misc.h"

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
    (void)deflateEnd(strm);
    free(strm);
    strm = NULL;
  }
  return result;
}

int loop(CONTENTS *(*run)(const CONTENTS *), const CONTENTS *contents,
             const struct timeval *timeout, RESULT* result) {
	assert(run != NULL && contents != NULL && contents->body != NULL && 
				contents->size > 0 && result != NULL);

	int ret = 1;

  CONTENTS *r = NULL;

  struct timeval start, loop, now, interval;

	memset (result, 0, sizeof(RESULT));

  gettimeofday(&start, NULL);

  do {
    gettimeofday(&loop, NULL);
    r = (*run)(contents);
    gettimeofday(&now, NULL);

    if (!r) {
      goto END;
    }
    result->loops++;
    result->input_bytes += contents->size;
    result->output_bytes += r->size;
    destroy_contents(r);

    timeval_subtract(&interval, &now, &loop);
    timeval_add(&(result->real_run), &interval, &(result->real_run));
  } while (timeval_before_timeout(timeout, &now, &start));

  ret = 0;

END:
  return ret;
}

struct thread_data {
	RESULT *result;
	const struct timeval *timeout;
	CONTENTS *(*run)(const CONTENTS *);
	const CONTENTS *contents;
};

static void *loopThread(void *arg) {
	struct thread_data *data = (struct thread_data *)arg;

	int ret = loop(data->run, data->contents, data->timeout, data->result);

	return (void*)(intptr_t)ret;
}

int thread_loop(CONTENTS *(*run)(const CONTENTS *), const CONTENTS *contents,
										const struct timeval *timeout, const unsigned int threads,
										RESULT* results) {
	assert(run != NULL && contents != NULL && contents->body != NULL && 
				contents->size > 0 && threads > 0);
	pthread_t *pids = NULL;
	struct thread_data* data = NULL;
	int ret = 1;

	pids = malloc(sizeof(pthread_t) * threads);
	if (!pids) {
		errno = ENOMEM;
		goto END;
	}

	data = malloc(sizeof(struct thread_data) * threads);
	if (!data) {
		errno = ENOMEM;
		goto END;
	}

	for (int i = 0; i < threads; i++) {
		data[i].result = results + i;
		data[i].timeout = timeout;
		data[i].run = run;
		data[i].contents = contents;
		pthread_create(pids+i, NULL, loopThread, (void *)(data+i));
	}

	for (int i = 0; i < threads; i++) {
		void* threadRet = NULL;
		pthread_join(pids[i], threadRet);

		if ((intptr_t)threadRet) {
			goto END;
		}
	}

	ret = 0;

END:
	if (pids) {
		free(pids);
		pids = NULL;
	}
	if (data) {
		free(data);
		data = NULL;
	}
	return ret;
}
