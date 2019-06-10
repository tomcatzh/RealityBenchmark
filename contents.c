#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <curl/curl.h>

#include "contents.h"

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  size_t realsize = size * nmemb;
  CONTENTS *mem = (CONTENTS *)userp;

  unsigned char *ptr =
      (unsigned char *)realloc(mem->body, mem->size + realsize + 1);
  if (ptr == NULL) {
    errno = ENOMEM;
    return 0;
  }

  mem->body = ptr;
  memcpy(&(mem->body[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->body[mem->size] = 0;

  return realsize;
}

CONTENTS *get_contents_url(const char *url) {
  CURL *curl_handle;
  CURLcode res;

  CONTENTS *result = calloc(1, sizeof(CONTENTS));

  curl_global_init(CURL_GLOBAL_ALL);

  curl_handle = curl_easy_init();

  curl_easy_setopt(curl_handle, CURLOPT_URL, url);

  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)result);

  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK) {
    destroy_contents(result);
    return NULL;
  }

  curl_easy_cleanup(curl_handle);

  curl_global_cleanup();

  return result;
}

CONTENTS *get_contents(const char *url) {
  CONTENTS *result = NULL;

  FILE *f = fopen(url, "r");
  if (f) {
    result = calloc(1, sizeof(CONTENTS));

    fseek(f, 0, SEEK_END);
    result->size = ftell(f);
    fseek(f, 0, SEEK_SET);

    result->body = (unsigned char *)malloc(result->size + 1);
    fread(result->body, 1, result->size, f);

    fclose(f);

    result->body[result->size] = 0;
  } else {
    result = get_contents_url(url);
  }

  return result;
}

void destroy_contents(CONTENTS *file) {
  if (file != NULL) {
    if (file->body != NULL) {
      free(file->body);
    }
  }
}