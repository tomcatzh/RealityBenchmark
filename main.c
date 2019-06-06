#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "contents.h"
#include "zlib_bench.h"
#include "misc.h"

static const char *ratio(int numerator, int denominator, char *buf,
                         size_t bufSize) {
  snprintf(buf, bufSize, "%.2lf%%",
           ((double)numerator / (double)denominator) * 100);
  return buf;
}

static const char *humanSize(unsigned long bytes, char *output,
                             size_t bufSize) {
  char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
  char length = sizeof(suffix) / sizeof(suffix[0]);

  int i = 0;
  double dblBytes = bytes;

  if (bytes > 1024) {
    for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024)
      dblBytes = bytes / 1024.0;
  }

  sprintf(output, "%.02lf %s", dblBytes, suffix[i]);
  return output;
}

static int resultSingleDecompress(RESULT *r) {
  double usec = r->real_run.tv_sec * 1000000 + r->real_run.tv_usec;

  char totalInput[32];
  humanSize(r->input_bytes, totalInput, sizeof(totalInput));

  char secondInput[32];
  humanSize((unsigned long)(r->input_bytes / usec * 1000000), secondInput,
            sizeof(secondInput));

  char totalOutput[32];
  humanSize(r->output_bytes, totalOutput, sizeof(totalOutput));

  char secondOutput[32];
  humanSize((unsigned long)(r->output_bytes / usec * 1000000), secondOutput,
            sizeof(secondOutput));

  return printf( "\tTotal Ops: %lu (%.4f Ops/s), Total Input: %s (%s/s), Total Output: "
		           "%s (%s/s)\n",
		           r->loops, r->loops / usec * 1000000, totalInput, secondInput,
		           totalOutput, secondOutput);
}


static int resultSingleCompress(RESULT *r) {
	char ratioBuf[32];
  ratio(r->output_bytes, r->input_bytes, ratioBuf,
  	sizeof(ratioBuf));

  int len = printf("(Ratio: %s)", ratioBuf);
  len += resultSingleDecompress(r);
  
  return len;
}

static int resultMultiDecompress(RESULT* results, unsigned int threads) {
	struct timeval *maxRun = NULL;
	struct timeval totalRun;
	totalRun.tv_sec = 0;
	totalRun.tv_usec = 0;

	unsigned long totalInput = 0;
	unsigned long totalOutput = 0;
	unsigned long totalLoops = 0;

	for (int i = 0; i < threads; i ++) {
		RESULT *r = results + i;

		if (maxRun) {
			if (timeval_before(maxRun, &(r->real_run))) {
				maxRun = &(r->real_run);
			}
		} else {
			maxRun = &(r->real_run);
		}

		timeval_addadd(&totalRun, &(r->real_run));

		totalInput += r->input_bytes;
		totalOutput += r->output_bytes;
		totalLoops += r->loops;
	}

	double usec = maxRun->tv_sec * 1000000 + maxRun->tv_usec;

	char totalInputStr[32];
  humanSize(totalInput, totalInputStr, sizeof(totalInputStr));

  char secondInput[32];
  humanSize((unsigned long)(totalInput / usec * 1000000), secondInput,
            sizeof(secondInput));

  char preThreadSecondInput[32];
  humanSize(((unsigned long)(totalInput / usec * 1000000)) / threads,
  				preThreadSecondInput, sizeof(preThreadSecondInput));

  char totalOutputStr[32];
  humanSize(totalOutput, totalOutputStr, sizeof(totalOutputStr));

  char secondOutput[32];
  humanSize((unsigned long)(totalOutput / usec * 1000000), secondOutput,
            sizeof(secondOutput));

  char preThreadSecondOutput[32];
  humanSize(((unsigned long)(totalOutput / usec * 1000000)) / threads,
  				preThreadSecondOutput, sizeof(preThreadSecondOutput));

  return printf( "\tTotal Ops: %lu (%.4f Ops/s), Total Input: %s (%s/s), Total Output: "
		           "%s (%s/s)\n"
		           "\t\tPreThread: %.4f Ops/s, Input: %s/s, Output: %s/s\n",
		           totalLoops, totalLoops / usec * 1000000, totalInputStr, secondInput,
		           totalOutputStr, secondOutput,
		           totalLoops / (float)threads, preThreadSecondInput, preThreadSecondOutput);

	return 0;
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
  RESULT result;
  RESULT* results = NULL;
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

  contents = get_contents(argv[index]);

  if (contents == NULL) {
    fprintf(stderr, "Get content error\n");
    goto END;
  } else if (contents->size == 0) {
    fprintf(stderr, "Empty content to zip\n");
    goto END;
  }

  deflatedContent = deflate_content_best_compression(contents);
  if (deflatedContent == NULL) {
    fprintf(stderr, "Error compress data.\n");
    goto END;
  }

  inflatedContent = inflate_content(deflatedContent);
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

  printf ("\n\n---------------\n\n");

  results = calloc(threads, sizeof(RESULT));

  printf("Running default compression for %lu seconds:\n", timeout.tv_sec);
  printf("1 thread:\n");
  fflush(stdout);
  if (loop(&deflate_content_default_compression, contents, &timeout, &result)) {
    fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  resultSingleCompress(&result);

  printf("%u threads:\n", threads);
  fflush(stdout);
  if(thread_loop(&deflate_content_default_compression, contents, &timeout, threads, results)) {
  	fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  printf("\t");
  resultMultiDecompress(results, threads);

  printf ("\n\n---------------\n\n");

  printf("Running best speed for %lu seconds:\n", timeout.tv_sec);
  printf("1 thread:\n");
  fflush(stdout);
  if (loop(&deflate_content_best_speed, contents, &timeout, &result)) {
    fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  resultSingleCompress(&result);

  printf("%u threads:\n", threads);
  fflush(stdout);
  if(thread_loop(&deflate_content_best_speed, contents, &timeout, threads, results)) {
  	fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  printf("\t");
  resultMultiDecompress(results, threads);

  printf ("\n\n---------------\n\n");

  printf("Running best compression for %lu seconds:\n", timeout.tv_sec);
  printf("1 thread:\n");
  fflush(stdout);
  if (loop(&deflate_content_best_compression, contents, &timeout, &result)) {
    fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  resultSingleCompress(&result);

  printf("%u threads:\n", threads);
  fflush(stdout);
  if(thread_loop(&deflate_content_best_compression, contents, &timeout, threads, results)) {
  	fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  printf("\t");
  resultMultiDecompress(results, threads);

  printf ("\n\n---------------\n\n");

  printf("Running decompression for %lu seconds:\n", timeout.tv_sec);
  printf("1 thread:\n");
  fflush(stdout);
  if (loop(&inflate_content, deflatedContent, &timeout, &result)) {
    fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  printf("\t");
  resultSingleDecompress(&result);

  printf("%u threads:\n", threads);
  fflush(stdout);
  if(thread_loop(&inflate_content, deflatedContent, &timeout, threads, results)) {
  	fprintf(stderr, "Benchmark fail!\n");
    goto END;
  }
  printf("\t");
  resultMultiDecompress(results, threads);

  printf ("\n\n---------------\n\n");
  
  ret = 0;

END:
  if (inflatedContent) {
    destroy_contents(inflatedContent);
    inflatedContent = NULL;
  }
  if (deflatedContent) {
    destroy_contents(deflatedContent);
    deflatedContent = NULL;
  }
  if (contents) {
    destroy_contents(contents);
    contents = NULL;
  }
  if (results) {
  	free(results);
  	results = NULL;
  }
  return ret;
}