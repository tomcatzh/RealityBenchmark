#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "contents.h"
#include "zlib_bench.h"
#include "misc.h"
#include "benchmark.h"

static const char *ratio(long numerator, long denominator, char *buf,
                         size_t bufSize) {
  snprintf(buf, bufSize, "%.2lf%%",
           ((double)numerator / (double)denominator) * 100);
  return buf;
}

static void printUsage() {
	fprintf(stderr,
	        "Usage: zlib_bench [-r <seconds, default is 3>] "
	        "[-t <threads, default is logic cpu cores>] file|url\n");
}

int main(int argc, char **argv) {
  int ret = -1;
  CONTENTS *contents = NULL;
  CONTENTS *deflatedContent = NULL;
  CONTENTS *inflatedContent = NULL;
  char buf[1024];

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  unsigned int threads = 0;

  int index;
  int c;
  opterr = 0;

  while ((c = getopt(argc, argv, "r:t:")) != -1) {
    switch (c) {
    case 'r':
    	timeout.tv_sec = atoi(optarg);
    	break;
    case 't':
    	threads = atoi(optarg);
    	break;
    case '?':
    	printUsage();
    	goto END;
    }
  }

  if (timeout.tv_sec == 0) {
  	timeout.tv_sec = 3;
  }

  if (threads == 0) {
  	threads = sysconf(_SC_NPROCESSORS_ONLN);
  	if (threads == 0) 
  		threads = 2;
  }

  index = optind;
  if (index >= argc) {
  	printUsage();
  	goto END;
  }

  contents = getContents(argv[index]);

  if (contents == NULL) {
    fprintf(stderr, "Get content error\n");
    goto END;
  } else if (contents->size == 0) {
    fprintf(stderr, "Empty content to zip\n");
    goto END;
  }

  deflatedContent = deflateContentBestCompression(contents);
  if (deflatedContent == NULL) {
    fprintf(stderr, "Error compress data.\n");
    goto END;
  }

  inflatedContent = inflateContent(deflatedContent);
  if (inflatedContent == NULL) {
    fprintf(stderr, "Error decompress data.\n");
    goto END;
  }

  if (contents->size != inflatedContent->size ||
      memcmp(contents->body, inflatedContent->body, contents->size) != 0) {
    fprintf(stderr, "Decompressed content not match!\n");
    goto END;
  }

  printf(
      "Got %lu bytes content, compressed best to %lu bytes (%s), decompressed "
      "success.\n",
      contents->size, deflatedContent->size,
      ratio(deflatedContent->size, contents->size, buf, sizeof(buf)));

  TEST *t = testNew();
  testSetThreads(t, 2);
  testSetTimeout(t, &timeout);
  testAddRun(t, &deflateContentBestCompression);
  testAddRun(t, &inflateContent);
  testSetInput(t, contents);
  testSetTesting(t, contents);

  RESULT *r = testRun(t);

  printf("%d\n", isResultSuccess(r));
  printf("%d\n", isResultFixed(r));
  printf("%d\n", isResultCorrect(r));
  printf("%lu\n", resultRealTime(r, NULL));
  printf("%lu\n", resultRealTimeByRun(r, 0, NULL));
  printf("%lu\n", resultRealTimeByRun(r, 1, NULL));
  printf("%lu %lu\n", resultTotalInputByRun(r, 0), resultTotalOutputByRun(r, 1));

  printf("%lu\n", resultThreadLoops(r, 0, 1, 1));
  printf("%lu\n", resultThreadLoops(r, 1, 1, 1));

  resultDestory(r);
  
  ret = 0;

END:
  if (inflatedContent) {
    destroyContents(inflatedContent);
    free(inflatedContent);
    inflatedContent = NULL;
  }
  if (deflatedContent) {
    destroyContents(deflatedContent);
    free(deflatedContent);
    deflatedContent = NULL;
  }
  if (contents) {
    destroyContents(contents);
    free(contents);
    contents = NULL;
  }
  return ret;
}