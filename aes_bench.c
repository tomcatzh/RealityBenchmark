#include <stdio.h>
#include <assert.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "contents.h"
#include "benchmark.h"
#include "misc.h"
#include "external/cJSON.h"

#define keyLength128Bit 16
#define keyLength192Bit 24
#define keyLength256Bit 32

static unsigned char key[32];
static unsigned char iv[16];

#define cipherMode128CBC (1 << 0)
#define cipherMode192CBC (1 << 1)
#define cipherMode256CBC (1 << 2)
#define cipherMode128CFB (1 << 3)
#define cipherMode192CFB (1 << 4)
#define cipherMode256CFB (1 << 5)
#define cipherMode128OFB (1 << 6)
#define cipherMode192OFB (1 << 7)
#define cipherMode256OFB (1 << 8)
#define cipherMode128CTR (1 << 9)
#define cipherMode192CTR (1 << 10)
#define cipherMode256CTR (1 << 11)

unsigned int cipherMode = 0;
unsigned int keyLength = 0;

static void init() {
  int fd = open("/dev/urandom", O_RDONLY);
  assert(fd >= 0);

  ssize_t ret;

  ret = read(fd, key, keyLength);
  assert(ret == keyLength);

  ret = read(fd, iv, 16);
  assert(ret == 16);

  close(fd);
}

static CONTENTS* decryptContent(const CONTENTS* data) {
  EVP_CIPHER_CTX *ctx;

  ctx = EVP_CIPHER_CTX_new();
  assert(ctx);

  int i = 0;
  switch (cipherMode) {
  case cipherMode128CBC:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
    break;
  case cipherMode192CBC:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_192_cbc(), NULL, key, iv);
    break;
  case cipherMode256CBC:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    break;
  case cipherMode128CFB:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, key, iv);
    break;
  case cipherMode192CFB:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_192_cfb(), NULL, key, iv);
    break;
  case cipherMode256CFB:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, key, iv);
    break;
  case cipherMode128OFB:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_128_ofb(), NULL, key, iv);
    break;
  case cipherMode192OFB:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_192_ofb(), NULL, key, iv);
    break;
  case cipherMode256OFB:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_256_ofb(), NULL, key, iv);
    break;
  case cipherMode128CTR:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
    break;
  case cipherMode192CTR:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_192_ctr(), NULL, key, iv);
    break;
  case cipherMode256CTR:
    i = EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
    break;
  }
  assert(i==1);

  CONTENTS* ret = NULL;
  ret = calloc(1, sizeof(CONTENTS));
  assert(ret);
  ret->body = (unsigned char*)malloc(data->size);

  int len;

  i = EVP_DecryptUpdate(ctx, ret->body, &len, data->body, data->size);
  assert(i==1);
  ret->size = len;

  i = EVP_DecryptFinal_ex(ctx, ret->body + len, &len);
  assert(i==1);
  ret->size += len;

  EVP_CIPHER_CTX_free(ctx);

  return ret;
}

static CONTENTS *encryptContent(const CONTENTS* data) {
  EVP_CIPHER_CTX *ctx;

  ctx = EVP_CIPHER_CTX_new();
  assert(ctx);

  int i = 0;
  switch (cipherMode) {
  case cipherMode128CBC:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
    break;
  case cipherMode192CBC:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_192_cbc(), NULL, key, iv);
    break;
  case cipherMode256CBC:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    break;
  case cipherMode128CFB:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, key, iv);
    break;
  case cipherMode192CFB:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_192_cfb(), NULL, key, iv);
    break;
  case cipherMode256CFB:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, key, iv);
    break;
  case cipherMode128OFB:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_128_ofb(), NULL, key, iv);
    break;
  case cipherMode192OFB:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_192_ofb(), NULL, key, iv);
    break;
  case cipherMode256OFB:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_256_ofb(), NULL, key, iv);
    break;
  case cipherMode128CTR:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
    break;
  case cipherMode192CTR:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_192_ctr(), NULL, key, iv);
    break;
  case cipherMode256CTR:
    i = EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
    break;
  }
  assert(i==1);

  CONTENTS* ret = NULL;
  ret = calloc(1, sizeof(CONTENTS));
  assert(ret);
  ret->body = (unsigned char*)malloc(data->size + keyLength - 1);

  int len;

  i = EVP_EncryptUpdate(ctx, ret->body, &len, data->body, data->size);
  assert(i==1);
  ret->size = len;

  i = EVP_EncryptFinal_ex(ctx, ret->body + len, &len);
  assert(i==1);
  ret->size += len;

  EVP_CIPHER_CTX_free(ctx);

  return ret;
}

static void printUsage() {
  fprintf(stderr,
          "Usage: aes_bench \n"
          "[-r seconds <seconds, default is 3>]\n"
          "[-t threads <threads, default is logic cpu cores>]\n"
          "[-k <key length>, should be 128, 192 or 256, default is 128]\n"
          "[-c <cipher mode>, should be CBC, CFB, OFB or CTR, default is CBC]\n"
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
  char mode[4];
  mode[0] = '\0';

  int index;
  int c;
  opterr = 0;

  while ((c = getopt(argc, argv, "r:t:vfu:k:c:")) != -1) {
    switch (c) {
    case 'r':
      timeout.tv_sec = atoi(optarg);
      break;
    case 't':
      threads = atoi(optarg);
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
    case 'k':
      keyLength = atoi(optarg);
      break;
    case 'c':
      strncpy(mode, optarg, sizeof(mode));
      mode[3] = '\0';
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

  switch (keyLength) {
  case 128:
  case 0:
    keyLength = keyLength128Bit;
    break;
  case 192:
    keyLength = keyLength192Bit;
    break;
  case 256:
    keyLength = keyLength256Bit;
    break;
  default:
    printUsage();
    goto END;
  }

  if (mode[0] == '\0' || strcmp(mode, "CBC") == 0) {
    switch (keyLength) {
    case keyLength128Bit:
      cipherMode = cipherMode128CBC;
      break;
    case keyLength192Bit:
      cipherMode = cipherMode192CBC;
      break;
    case keyLength256Bit:
      cipherMode = cipherMode256CBC;
      break;
    }
  } else if (strcmp(mode, "CFB") == 0) {
    switch (keyLength) {
    case keyLength128Bit:
      cipherMode = cipherMode128CFB;
      break;
    case keyLength192Bit:
      cipherMode = cipherMode192CFB;
      break;
    case keyLength256Bit:
      cipherMode = cipherMode256CFB;
      break;
    }
  } else if (strcmp(mode, "OFB") == 0) {
    switch (keyLength) {
    case keyLength128Bit:
      cipherMode = cipherMode128OFB;
      break;
    case keyLength192Bit:
      cipherMode = cipherMode192OFB;
      break;
    case keyLength256Bit:
      cipherMode = cipherMode256OFB;
      break;
    }
  } else if (strcmp(mode, "CTR") == 0) {
    switch (keyLength) {
    case keyLength128Bit:
      cipherMode = cipherMode128CTR;
      break;
    case keyLength192Bit:
      cipherMode = cipherMode192CTR;
      break;
    case keyLength256Bit:
      cipherMode = cipherMode256CTR;
      break;
    }
  } else {
    printUsage();
    goto END;
  }

  init();

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
  testAddRun(t, &encryptContent);
  testAddRun(t, &decryptContent);
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