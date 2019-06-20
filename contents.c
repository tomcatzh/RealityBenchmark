#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>

#include <curl/curl.h>

#include "contents.h"

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  size_t realsize = size * nmemb;
  CONTENTS *mem = (CONTENTS *)userp;

  unsigned char *ptr =
      (unsigned char *)realloc(mem->body, mem->size + realsize + 1);
  assert(ptr);

  mem->body = ptr;
  memcpy(&(mem->body[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->body[mem->size] = 0;

  return realsize;
}

static CONTENTS *getContentsUrl(const char *url) {
  CURL *curl_handle;
  CURLcode res;

  CONTENTS *result = calloc(1, sizeof(CONTENTS));
  assert(result);

  curl_global_init(CURL_GLOBAL_ALL);

  curl_handle = curl_easy_init();

  curl_easy_setopt(curl_handle, CURLOPT_URL, url);

  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)result);

  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK) {
    destroyContents(result);
    return NULL;
  }

  curl_easy_cleanup(curl_handle);

  curl_global_cleanup();

  return result;
}

CONTENTS *randomContents(const size_t size) {
  CONTENTS *result;
  result = calloc(1, sizeof(CONTENTS));
  assert(result);

  result->body = (unsigned char*)malloc(size);
  assert(result->body);
  result->size = size;

  int fd = open("/dev/urandom", O_RDONLY);
  assert(fd >= 0);
  ssize_t ret = read(fd, result->body, result->size);
  assert(ret == result->size);
  close(fd);

  return result;
}

CONTENTS *getContents(const char *url) {
  CONTENTS *result = NULL;

  FILE *f = fopen(url, "r");
  if (f) {
    result = calloc(1, sizeof(CONTENTS));
    assert(result);

    fseek(f, 0, SEEK_END);
    result->size = ftell(f);
    fseek(f, 0, SEEK_SET);

    result->body = (unsigned char *)malloc(result->size );
    assert(result->body);
    fread(result->body, 1, result->size, f);

    fclose(f);
  } else {
    result = getContentsUrl(url);
  }

  return result;
}

int destroyContents(CONTENTS *file) {
  if (file != NULL) {
    if (file->body != NULL) {
      free(file->body);
    }
  }
  return 0;
}


CONTENTS* cloneContents(CONTENTS *source) {
  assert(source != NULL);

  CONTENTS *result = NULL;
  result = calloc(1, sizeof(CONTENTS));
  assert(result);
  result->body = (unsigned char*) malloc(source->size);
  memcpy(result->body, source->body, source->size);
  result->size = source->size;

  return result;
}

int compareContents(const CONTENTS *x, const CONTENTS *y) {
  assert(x);
  assert(y);

  return (x->size == y->size) ? memcmp(x->body, y->body, x->size) : 1;
}