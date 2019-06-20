#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <zlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "contents.h"
#include "benchmark.h"
#include "misc.h"
#include "external/cJSON.h"

static int level = -1;

static CONTENTS *deflateContent(const CONTENTS *data) {
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

static CONTENTS *inflateContent(const CONTENTS *data) {
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

static void printUsage() {
  fprintf(stderr,
          "Usage: zlib_bench \n"
          "[-r seconds <seconds, default is 3>]\n"
          "[-t threads <threads, default is logic cpu cores>]\n"
          "[-l level <levels, compress level 1-9, default is -1(6)>]\n"
          "[-v <verbose json output>] [-f <formated json output>]\n"
          "-u size <use random data block, size can use K, M, G>|file|url\n");
}

int main(int argc, char **argv) {
  int ret = -1;
  CONTENTS *contents = NULL;

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  unsigned int threads = 0;
  int verbose = 0;
  int formated = 0;
  size_t randomSize = 0;

  int index;
  int c;
  opterr = 0;

  while ((c = getopt(argc, argv, "r:t:l:vfu:")) != -1) {
    switch (c) {
    case 'r':
      timeout.tv_sec = atoi(optarg);
      break;
    case 't':
      threads = atoi(optarg);
      break;
    case 'l':
      level = atoi(optarg);
      break;
    case 'u':
      randomSize = parseHumanSize(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    case 'f':
      formated = 1;
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

  if (randomSize) {
    contents = randomContents(randomSize);
  } else {
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
  }
  

  TEST *t = testNew();
  testSetThreads(t, threads);
  testSetTimeout(t, &timeout);
  testAddRun(t, &deflateContent);
  testAddRun(t, &inflateContent);
  testSetInput(t, contents);
  testSetTesting(t, contents);

  RESULT *r = testRun(t);
  assert(r);

  cJSON *json = NULL;
  if (verbose) {
    json = resultToJSONVerbose(r);
  } else {
    json = resultToJSON(r);
  }
  assert(json);

  char *jsonString = NULL;
  if (formated) {
    jsonString = cJSON_Print(json);
  } else {
    jsonString = cJSON_PrintUnformatted(json);
  }
  assert(jsonString);

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