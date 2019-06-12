#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include "misc.h"
#include "contents.h"

#include "benchmark.h"

TEST *testNew() {
  TEST* t = (TEST *)calloc(1, sizeof(TEST));
  assert(t);

  return t;
}

void testSetTimeout(TEST *t, struct timeval* timeout) {
  t->timeout.tv_sec = timeout->tv_sec;
  t->timeout.tv_usec = timeout->tv_usec;
}

void testSetThreads(TEST *t, unsigned int threads) {
  t->threads = threads;
}

void testAddRun(TEST *t, CONTENTS* (*run)(const CONTENTS*)) {
  t->run = realloc(t->run, sizeof(run) * (t->runCount + 1));
  assert(t->run);

  t->run[t->runCount] = run;
  t->runCount ++;
}

void testSetTesting(TEST *t, const CONTENTS* data) {
  t->verifyData = data;
}

void testSetInput(TEST *t, const CONTENTS* input) {
  t->input = input;
}

void testDestory(TEST *t) {
  return;
}

size_t runTotalInput(RUN* runs, unsigned int run) {
  int id = 0;

  for (RUN* r = runs; r; r = r->next) {
    if (id == run) return r->inputBytes;
  }
  assert(0);

  return 0;
}

size_t loopTotalInput(LOOP *loops, unsigned int run) {
  size_t total = 0;

  for (LOOP *l = loops; l; l = l->next) {
    total += runTotalInput(l->runs, run);
  }

  return total;
}

size_t resultTotalInputByRun(RESULT *results, unsigned int run) {
  size_t total = 0;
  
  for (RESULT* r = results; r; r = r->next) {
     total += loopTotalInput(r->loops, run);
  }

  return total;
}

size_t runTotalOutput(RUN* runs, unsigned int run) {
  int id = 0;

  for (RUN* r = runs; r; r = r->next) {
    if (id == run) return r->outputBytes;
  }

  return 0;
}

size_t loopTotalOutput(LOOP *loops, unsigned int run) {
  size_t total = 0;

  for (LOOP *l = loops; l; l = l->next) {
    total += runTotalOutput(l->runs, run);
  }

  return total;
}

size_t resultTotalOutputByRun(RESULT *results, unsigned int run) {
  size_t total = 0;

  for (RESULT* r = results; r; r = r->next) {
     total += loopTotalOutput(r->loops, run);
  }

  return total;
}

static int isLoopCorrect(LOOP* loops) {
  return loops->correct;
}

void runTotalTime(RUN* runs, unsigned int run, struct timeval *time) {
  unsigned int id = 0;
  for (RUN *r = runs; r; r = r->next) {
    if (run == ~0 || run == id) timevalAddAdd(time, &(r->interval));
    id ++;
  }
}

unsigned long loopTotalTime(LOOP* loops, unsigned int run, 
                          struct timeval *totalTime) {
  assert(totalTime);
  totalTime->tv_sec = 0;
  totalTime->tv_usec = 0;

  for (LOOP* l = loops; l; l = l->next) {
    runTotalTime(l->runs, run, totalTime);
  }

  return timevalToUsec(totalTime);
}

unsigned long resultRealTimeByRun(RESULT* results, unsigned int run,
                                  struct timeval *totalTime) {
  struct timeval t;
  unsigned long maxTime = 0; 

  for (RESULT* r = results; r; r = r->next) {
    unsigned long time = loopTotalTime(r->loops, run, &t);
    if (time > maxTime) {
      maxTime = time;
      if (totalTime) {
        totalTime->tv_sec = t.tv_sec;
        totalTime->tv_usec = t.tv_usec;
      }
    }
  }
  return maxTime;
}

unsigned long resultRealTime(RESULT* results, struct timeval *totalTime) {
  return resultRealTimeByRun(results, ~0, totalTime);
}

int isResultCorrect(RESULT* results) {
  for (RESULT *re = results; re; re = re->next) {
    if (!isLoopCorrect(re->loops)) return 0;
  }
  return 1;
}

int runCheckSample(RUN *runs, size_t *samples) {
  assert(samples);

  for (RUN* r = runs; r; r = r->next) {
    if (r->outputBytes != *(samples++)) return 0;
  }

  return 1;
}

size_t *runSizeSample(RUN* runs) {
  size_t *samples = NULL;
  unsigned int count = 0;

  for (RUN* r = runs; r; r = r->next) {
    samples = (size_t*)realloc(samples, sizeof(size_t) * (count + 1));
    samples[count] = r->outputBytes;
    count ++;
  }

  return samples;
}

int isLoopResultFixed(LOOP* loops) {
  int ret = 1;
  size_t *samples = runSizeSample(loops->runs);

  for (LOOP *l = loops->next; l; l = l->next) {
    if (!runCheckSample(l->runs, samples)) { 
      ret = 0;
      break;
    }
  }

  if (samples) {
    free(samples);
  }
  
  return ret;
}

int isResultFixed(RESULT* results) {
  for (RESULT *re = results; re; re = re->next) {
    if (!isLoopResultFixed(re->loops)) return 0;
  }
  return 1;
}

static int isRunSuccess(RUN* run) {
  for (RUN *ru = run; ru; ru = ru->next) {
    if (!ru->success) return 0;
  }
  return 1;
}

static int isLoopSuccess(LOOP* loop) {
  for (LOOP *l = loop; l; l = l->next) {
    if (!isRunSuccess(l->runs)) return 0;
  }
  return 1;
}

int isResultSuccess(RESULT* result) {
  for (RESULT *re = result; re; re = re->next) {
    if (!isLoopSuccess(re->loops)) return 0;
  }
  return 1;
}

static unsigned long getLoops(LOOP* loop, int successOnly, int testedOnly) {
  unsigned long count = 0;
  for (LOOP *l = loop; l; l = l->next) {
    if ( (!successOnly || isRunSuccess(l->runs))
        && (!testedOnly || l->correct) ) {
      count ++;
    }
  }
  return count;
}

unsigned long resultThreadLoops(RESULT* result, unsigned int thread,
                            int successOnly, int testedOnly) {
  unsigned int currentThread = 0;

  for (RESULT *re = result; re; re = re->next) {
    if (currentThread == thread) {
      return getLoops(re->loops, successOnly, testedOnly);
    }

    currentThread ++;
  }
  assert(0);
  return 0;
}

static void runDestory(RUN *runs) {
  RUN *fru;

  for (RUN *ru = runs; ru; ) {
      fru = ru;
      ru = ru->next;
      free(fru);
    }
}

static void loopDestory(LOOP *loops) {
  LOOP *fl;

  for (LOOP *l = loops; l; ) {
    runDestory(l->runs);

    fl = l;
    l = l->next;
    free(fl);
  }
}

void resultDestory(RESULT *result) {
  RESULT *fre;

  for (RESULT *re = result; re; ) {
    loopDestory(re->loops);

    fre = re;
    re = re->next;
    free(fre);
  }
  return;
}

static void *loopThread(void *arg) {
  TEST *t = (TEST *)arg;

  const CONTENTS* input;
  CONTENTS *output, *needFree;

  struct timeval start, loopTime, now;
  gettimeofday(&start, NULL);

  LOOP *headLoop, *loop, *newLoop;
  RUN *headRun, *run, *newRun;

  loop = headLoop = NULL;

  do {
    newLoop = (LOOP *)calloc(1, sizeof(LOOP));
    assert(newLoop);
    if (!headLoop) headLoop = newLoop;
    if (loop) loop->next = newLoop;
    loop = newLoop;

    input = t->input;
    output = needFree = NULL;
    run = headRun = NULL;

    for (unsigned int i = 0; i < t->runCount; i ++) {
      newRun = (RUN *)calloc(1, sizeof(RUN));
      if (!headRun) headRun = newRun;
      if (run) run->next = newRun;
      run = newRun;

      if (needFree) {
        destroyContents(needFree);
        needFree = NULL;
      }
      if (output) {
        needFree = output;
        output = NULL;
      }

      gettimeofday(&loopTime, NULL);
      if (input) {
        output = (*(t->run[i]))(input);
      }
      gettimeofday(&now, NULL);

      if (output) {
        run->inputBytes = input->size;
        run->outputBytes = output->size;
        run->success = 1;
      }

      timevalSubtract(&(run->interval), &now, &loopTime);
      
      input = output;
    }

    if (needFree) {
      destroyContents(needFree);
    }

    if (output) {
      if (t->verifyData) {
        loop->correct = !(compareContents(t->verifyData, output));
      } else {
        loop->correct = 1;
      }

      destroyContents(output);
    }

    loop->runs = headRun;
  } while (timevalBeforeTimeout(&(t->timeout), &now, &start));

  return headLoop;
}

RESULT *testRun(TEST *t) {
  assert(t->run);
  assert(t->threads);

  pthread_t *pids = malloc(sizeof(pthread_t) * t->threads);
  assert(pids);

  for (unsigned int i = 0; i < t->threads; i ++) {
    printf ("thread start %d\n", i);
    pthread_create(pids+i, NULL, loopThread, t);
  }

  RESULT *headResult, *result, *newResult;
  headResult = result = NULL;

  for (unsigned int i = 0; i < t->threads; i ++) {
    LOOP* loops = NULL;
    pthread_join(pids[i], (void *)&loops);
    printf ("thread finish %d, %lu\n", i, getLoops(loops, 1, 1));

    newResult = (RESULT *)calloc(1, sizeof(RESULT));
    assert(newResult);
    if (!headResult) headResult = newResult;
    if (result) result->next = newResult;
    result = newResult;

    result->loops = loops;
    result->thread = i;
  }

  return headResult;
}