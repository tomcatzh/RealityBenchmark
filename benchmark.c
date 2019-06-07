#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#include "contents.h"
#include "misc.h"

#include "benchmark.h"

int loop(CONTENTS *(*run)(const CONTENTS *), const CONTENTS *contents,
             const struct timeval *timeout, RESULT* result) {
  assert(run != NULL && contents != NULL && contents->body != NULL && 
        contents->size > 0 && result != NULL);

  int ret = 1;

  CONTENTS *r = NULL;

  struct timeval start, loop, now, interval;

  unsigned long *usecBuf = NULL;
  size_t usecBufSize = 0;

  memset (result, 0, sizeof(RESULT));

  gettimeofday(&start, NULL);

  do {
    gettimeofday(&loop, NULL);
    r = (*run)(contents);
    gettimeofday(&now, NULL);

    if (!r) {
      goto END;
    }

    timeval_subtract(&interval, &now, &loop);
    timeval_add(&(result->real_run), &interval, &(result->real_run));

    if (usecBufSize == 0) {
      unsigned long *ptr = (unsigned long*) realloc(usecBuf, (result->loops + 10000 * timeout->tv_sec) * sizeof(unsigned long));
      if (!ptr) {
        errno = ENOMEM;
        goto END;
      }
      usecBuf = ptr;
      usecBufSize = 10000 * timeout->tv_sec;
    }
    usecBuf[result->loops] = to_usec(&interval);
    usecBufSize --;

    result->loops++;
    result->input_bytes += contents->size;
    result->output_bytes += r->size;
    destroy_contents(r);
  } while (timeval_before_timeout(timeout, &now, &start));

  unsigned long avg;
  avg = to_usec(&(result->real_run)) / result->loops;

  unsigned long sumPow = 0;
  for (unsigned long i = 0; i < result->loops; i++) {
    sumPow += (usecBuf[i] - avg) * (usecBuf[i] - avg);
  }
  result->sd = sqrt(sumPow/result->loops);

  ret = 0;

END:
  if (usecBuf) {
    free(usecBuf);
    usecBuf = NULL;
  }
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