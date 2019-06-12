#ifndef __REALITY_BENCHMARK_H
#define __REALITY_BENCHMARK_H

#include <sys/time.h>

#include "contents.h"

struct b_run {
  struct timeval interval;
  int success;
  size_t inputBytes;
  size_t outputBytes;
  struct b_run *next;
};
typedef struct b_run RUN;

struct b_loop {
  RUN* runs;
  int correct;
  struct b_loop *next;
};
typedef struct b_loop LOOP;

struct b_result {
  LOOP* loops;
  unsigned int thread;
  struct b_result* next;
};
typedef struct b_result RESULT;

void resultDestory(RESULT* r);

unsigned long resultThreadLoops(RESULT* r, unsigned int thread);
unsigned long resultAvgLoop(RESULT* r);
double resultStdevLoop(RESULT* results);

int isResultSuccess(RESULT* r);
int isResultFixed(RESULT* r);
int isResultCorrect(RESULT* r);

unsigned long resultRealTime(RESULT* r, struct timeval *totalTime);
unsigned long resultRealTimeByRun(RESULT* r, unsigned int run,
                                  struct timeval *totalTime);
unsigned long resultAvgInterval(RESULT* r);
double resultStdevInterval(RESULT* r);
unsigned long resultAvgIntervalByRun(RESULT* r, unsigned int run);
double resultStdevIntervalByRun(RESULT* r, unsigned int run);

size_t resultTotalInputByRun(RESULT *r, unsigned int run);
size_t resultTotalOutputByRun(RESULT *r, unsigned int run);
size_t resultSampleInputByRun(RESULT *r, unsigned int run);
size_t resultSampleOutputByRun(RESULT *r, unsigned int run);

struct b_test {
  struct timeval timeout;
  unsigned int threads;
  CONTENTS* (**run)(const CONTENTS*);
  unsigned int runCount;
  const CONTENTS* input;
  const CONTENTS* verifyData;
};
typedef struct b_test TEST;

TEST *testNew();
void testDestory(TEST *t);

void testSetTimeout(TEST *t, struct timeval* timeout);
void testSetThreads(TEST *t, unsigned int threads);
void testAddRun(TEST *t, CONTENTS* (*run)(const CONTENTS*));
void testSetTesting(TEST *t, const CONTENTS* data);
void testSetInput(TEST *t, const CONTENTS* input);

RESULT *testRun(TEST *t);

#endif