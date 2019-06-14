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
#include "external/cJSON.h"

static void printUsage() {
	fprintf(stderr,
	        "Usage: zlib_bench [-r <seconds, default is 3>] "
	        "[-t <threads, default is logic cpu cores>] file|url\n");
}

int main(int argc, char **argv) {
  int ret = -1;
  CONTENTS *contents = NULL;

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

  TEST *t = testNew();
  testSetThreads(t, threads);
  testSetTimeout(t, &timeout);
  testAddRun(t, &deflateContentBestCompression);
  testAddRun(t, &inflateContent);
  testSetInput(t, contents);
  testSetTesting(t, contents);

  RESULT *r = testRun(t);

  cJSON *json = resultToJSON(r);
  char* jsonString = cJSON_Print(json);

  printf("%s\n", jsonString);

  cJSON_Delete(json);
  free(jsonString);

  resultDestory(r);
  
  ret = 0;

END:
  if (contents) {
    destroyContents(contents);
    free(contents);
    contents = NULL;
  }
  return ret;
}