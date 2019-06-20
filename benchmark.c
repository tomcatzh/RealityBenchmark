#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>

#include "misc.h"
#include "contents.h"
#include "external/cJSON.h"

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

static unsigned long getLoops(const LOOP* loop) {
  unsigned long count = 0;
  for (const LOOP *l = loop; l; l = l->next) {
    count ++;
  }
  return count;
}

static unsigned long resultTotalLoops(const RESULT* result) {
  unsigned long total = 0;
  for (const RESULT *re = result; re; re = re->next) {
    total += getLoops(re->loops);
  }
  return total;
}

static unsigned int resultThreads(const RESULT* result) {
  unsigned int count = 0;
  for (const RESULT *re = result; re; re = re->next) {
    count ++;
  }
  return count;
}

static unsigned long resultAvgLoop(const RESULT* result) {
  return resultTotalLoops(result) / resultThreads(result);
}

static double resultStdevLoop(const RESULT* results) {
  unsigned long sumPow = 0;
  unsigned long avg = resultAvgLoop(results);
  unsigned int count = 0;

  for (const RESULT *re = results; re; re = re->next) {
    unsigned long loops = getLoops(re->loops);
    sumPow += (loops - avg) * (loops - avg);
    count ++;
  }

  return sqrt((double)sumPow/(double)count);
}

static unsigned long resultThreadLoops(const RESULT* result, const unsigned int thread) {
  unsigned int currentThread = 0;

  for (const RESULT *re = result; re; re = re->next) {
    if (currentThread == thread) {
      return getLoops(re->loops);
    }

    currentThread ++;
  }
  assert(0);
  return 0;
}


static size_t runTotalInput(const RUN* runs, const unsigned int run) {
  int id = 0;

  for (const RUN* r = runs; r; r = r->next) {
    if (id == run) return r->inputBytes;
    id ++;
  }
  assert(0);

  return 0;
}

static size_t loopTotalInput(const LOOP *loops, const unsigned int run) {
  size_t total = 0;

  for (const LOOP *l = loops; l; l = l->next) {
    total += runTotalInput(l->runs, run);
  }

  return total;
}

static size_t resultTotalInputByRun(const RESULT *results, const unsigned int run) {
  size_t total = 0;
  
  for (const RESULT* r = results; r; r = r->next) {
     total += loopTotalInput(r->loops, run);
  }

  return total;
}

static size_t runTotalOutput(const RUN* runs, const unsigned int run) {
  int id = 0;

  for (const RUN* r = runs; r; r = r->next) {
    if (id == run) return r->outputBytes;
    id ++;
  }
  assert(0);

  return 0;
}

static size_t loopTotalOutput(const LOOP *loops, const unsigned int run) {
  size_t total = 0;

  for (const LOOP *l = loops; l; l = l->next) {
    total += runTotalOutput(l->runs, run);
  }

  return total;
}

static size_t resultTotalOutputByRun(const RESULT *results, const unsigned int run) {
  size_t total = 0;

  for (const RESULT* r = results; r; r = r->next) {
     total += loopTotalOutput(r->loops, run);
  }

  return total;
}

static int isLoopCorrect(const LOOP* loops) {
  for (const LOOP* l = loops; l; l = l->next) {
    if (!l->correct) return 0;
  }
  return 1;
}

static void runTotalTime(RUN* runs, unsigned int run, struct timeval *time) {
  assert(time);
  unsigned int id = 0;
  for (RUN *r = runs; r; r = r->next) {
    if (run == ~0 || run == id) timevalAddAdd(time, &(r->interval));
    id ++;
  }
}

static unsigned long loopTotalTime(const LOOP* loops, const unsigned int run, 
                          struct timeval *totalTime) {
  assert(totalTime);
  totalTime->tv_sec = 0;
  totalTime->tv_usec = 0;

  for (const LOOP* l = loops; l; l = l->next) {
    runTotalTime(l->runs, run, totalTime);
  }

  return timevalToUsec(totalTime);
}

static double resultAvgIntervalByRun(const RESULT* results, const unsigned int run) {
  struct timeval t;
  unsigned long total = 0;
  unsigned long loops = 0;
  unsigned int id = 0;

  for (const RESULT* r = results; r; r = r->next) {
    total += loopTotalTime(r->loops, run, &t);
    loops += resultThreadLoops(results, id);
  }

  return (double)total / (double)loops;
}

static double resultAvgInterval(const RESULT* r) {
  return resultAvgIntervalByRun(r, ~0);
}

static unsigned long loopSumPow(LOOP* loops, unsigned int run, unsigned long avg) {
  unsigned long usec = 0;
  unsigned long sumPow = 0;

  struct timeval t;

  for (LOOP* l = loops; l; l = l->next) {
    t.tv_sec = 0;
    t.tv_usec = 0;
    
    runTotalTime(l->runs, run, &t);
    usec = timevalToUsec(&t);

    sumPow += (usec - avg) * (usec - avg);
  }

  return sumPow;
}

static double resultStdevIntervalByRun(const RESULT* results, unsigned int run) {
  unsigned long avg = resultAvgIntervalByRun(results, run);
  unsigned long sumPow = 0;
  unsigned long loops = 0;
  unsigned int id = 0;

  for (const RESULT* r = results; r; r = r->next) {
    sumPow += loopSumPow(r->loops, run, avg);
    loops += resultThreadLoops(results, id);
    id++;
  }

  return sqrt((double)sumPow/(double)loops);
}

static double resultStdevInterval(const RESULT* r) {
  return resultStdevIntervalByRun(r, ~0);
}

static unsigned long resultRealTimeByRun(const RESULT* results, const unsigned int run,
                                  struct timeval *totalTime) {
  struct timeval t;
  unsigned long maxTime = 0; 

  for (const RESULT* r = results; r; r = r->next) {
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

static unsigned long resultRealTime(const RESULT* results, struct timeval *totalTime) {
  return resultRealTimeByRun(results, ~0, totalTime);
}

static int isResultCorrect(const RESULT* results) {
  for (const RESULT *re = results; re; re = re->next) {
    if (!isLoopCorrect(re->loops)) return 0;
  }
  return 1;
}

static int runCheckOutputSample(RUN *runs, size_t *samples) {
  assert(samples);

  for (RUN* r = runs; r; r = r->next) {
    if (r->outputBytes != *(samples++)) return 0;
  }

  return 1;
}

static size_t *runOutputSample(const RUN* runs) {
  size_t *samples = NULL;
  unsigned int count = 0;

  for (const RUN* r = runs; r; r = r->next) {
    samples = (size_t*)realloc(samples, sizeof(size_t) * (count + 1));
    samples[count] = r->outputBytes;
    count ++;
  }

  return samples;
}

static size_t *runInputSample(RUN* runs) {
  size_t *samples = NULL;
  unsigned int count = 0;

  for (RUN* r = runs; r; r = r->next) {
    samples = (size_t*)realloc(samples, sizeof(size_t) * (count + 1));
    samples[count] = r->inputBytes;
    count ++;
  }

  return samples;
}

static size_t resultSampleInputByRun(const RESULT *r, const unsigned int run) {
  size_t *samples = runInputSample(r->loops->runs);
  assert (samples);

  unsigned int ret = samples[run];

  free(samples);

  return ret;
}

static size_t resultSampleOutputByRun(const RESULT *r, const unsigned int run) {
  size_t *samples = runOutputSample(r->loops->runs);
  assert (samples);

  unsigned int ret = samples[run];

  free(samples);

  return ret;
}

static int isLoopResultFixed(const LOOP* loops) {
  int ret = 1;
  size_t *samples = runOutputSample(loops->runs);
  assert(samples);

  for (const LOOP *l = loops->next; l; l = l->next) {
    if (!runCheckOutputSample(l->runs, samples)) { 
      ret = 0;
      break;
    }
  }

  free(samples);  
  return ret;
}

static int isResultFixed(const RESULT* results) {
  for (const RESULT *re = results; re; re = re->next) {
    if (!isLoopResultFixed(re->loops)) return 0;
  }
  return 1;
}

static int isRunSuccess(const RUN* run) {
  for (const RUN *ru = run; ru; ru = ru->next) {
    if (!ru->success) return 0;
  }
  return 1;
}

static int isLoopSuccess(const LOOP* loop) {
  for (const LOOP *l = loop; l; l = l->next) {
    if (!isRunSuccess(l->runs)) return 0;
  }
  return 1;
}

static int isResultSuccess(const RESULT* result) {
  for (const RESULT *re = result; re; re = re->next) {
    if (!isLoopSuccess(re->loops)) return 0;
  }
  return 1;
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

cJSON* resultToJSON(const RESULT *results) {
  cJSON *resultsJSON = cJSON_CreateObject();
  assert(resultsJSON);

  cJSON_AddBoolToObject(resultsJSON, "allSuccess", isResultSuccess(results));
  cJSON_AddBoolToObject(resultsJSON, "allCorrect", isResultCorrect(results));
  cJSON_AddBoolToObject(resultsJSON, "allFixed", isResultFixed(results));
  cJSON_AddNumberToObject(resultsJSON, "threads", resultThreads(results));
  cJSON_AddNumberToObject(resultsJSON, "totalLoops", resultTotalLoops(results));
  cJSON_AddNumberToObject(resultsJSON, "avgLoops", resultAvgLoop(results));
  cJSON_AddNumberToObject(resultsJSON, "stdevLoops", resultStdevLoop(results));
  cJSON_AddNumberToObject(resultsJSON, "time", resultRealTime(results, NULL));
  cJSON_AddNumberToObject(resultsJSON, "avgInterval", resultAvgInterval(results));
  cJSON_AddNumberToObject(resultsJSON, "stdevInterval", resultStdevInterval(results));

  cJSON *runsJSON = cJSON_CreateArray();
  assert(runsJSON);

  unsigned int id = 0;
  for (RUN *r = results->loops->runs; r; r = r->next) {
    cJSON *run = cJSON_CreateObject();

    cJSON_AddNumberToObject(run, "input", resultSampleInputByRun(results, id));
    cJSON_AddNumberToObject(run, "output", resultSampleOutputByRun(results, id));
    cJSON_AddNumberToObject(run, "totalInput", resultTotalInputByRun(results, id));
    cJSON_AddNumberToObject(run, "totalOutput", resultTotalOutputByRun(results, id));
    cJSON_AddNumberToObject(run, "avgInterval", resultAvgIntervalByRun(results, id));
    cJSON_AddNumberToObject(run, "stdevInterval", resultStdevIntervalByRun(results, id));
    cJSON_AddItemToObject(runsJSON, "runs", run);

    id ++;
  }
  cJSON_AddItemToObject(resultsJSON, "runs", runsJSON);

  return resultsJSON;
}

static cJSON *runsToJSONVerbose(const RUN* runs) {
  cJSON *runsJSON = cJSON_CreateArray();
  assert(runsJSON);

  for (const RUN *r = runs; r; r = r->next) {
    cJSON *run = cJSON_CreateObject();
    assert(run);

    cJSON_AddBoolToObject(run, "success", r->success);
    cJSON_AddNumberToObject(run, "time", timevalToUsec(&(r->interval)));
    cJSON_AddNumberToObject(run, "input", r->inputBytes);
    cJSON_AddNumberToObject(run, "output", r->outputBytes);

    cJSON_AddItemToArray(runsJSON, run);
  }

  return runsJSON;
}

static cJSON *loopsToJSONVerbose(const LOOP* loops) {
  cJSON *loopsJSON = cJSON_CreateArray();
  assert(loopsJSON);

  for (const LOOP *l = loops; l; l = l->next) {
    cJSON *loop = cJSON_CreateObject();
    assert(loop);

    cJSON_AddBoolToObject(loop, "correct", l->correct);
    cJSON_AddItemToObject(loop, "runs", runsToJSONVerbose(l->runs));

    cJSON_AddItemToArray(loopsJSON, loop);
  }

  return loopsJSON;
}

cJSON *resultToJSONVerbose(const RESULT *results) {
  cJSON *resultsJSON = cJSON_CreateArray();
  assert(resultsJSON);

  for (const RESULT *re = results; re; re = re->next) {
    cJSON_AddItemToArray(resultsJSON, loopsToJSONVerbose(re->loops));
  }

  return resultsJSON;
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
    pthread_create(pids+i, NULL, loopThread, t);
  }

  RESULT *headResult, *result, *newResult;
  headResult = result = NULL;

  for (unsigned int i = 0; i < t->threads; i ++) {
    LOOP* loops = NULL;
    pthread_join(pids[i], (void *)&loops);

    newResult = (RESULT *)calloc(1, sizeof(RESULT));
    assert(newResult);
    if (!headResult) headResult = newResult;
    if (result) result->next = newResult;
    result = newResult;

    result->loops = loops;
  }

  return headResult;
}