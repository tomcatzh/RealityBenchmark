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

#define keyLength128Bit 16
#define keyLength192Bit 24
#define keyLength256Bit 32

static unsigned char key[32];
static unsigned char iv[16];
static unsigned char aad[32];

#define cipherModeCBC (1 << 0)
#define cipherModeCFB (1 << 1)
#define cipherModeOFB (1 << 2)
#define cipherModeCTR (1 << 3)
#define cipherModeGCM (1 << 4)
#define cipherModeCCM (1 << 5)

static unsigned int cipherMode = 0;
static unsigned int keyLength = 0;
static int tagLength = 12;
static int ivLength = 7;

static void init() {
  int fd = open("/dev/urandom", O_RDONLY);
  assert(fd >= 0);

  ssize_t ret;

  ret = read(fd, key, keyLength);
  assert(ret == keyLength);

  ret = read(fd, iv, sizeof(iv));
  assert(ret == sizeof(iv));

  if (cipherMode == cipherModeGCM || cipherMode == cipherModeCCM) {
    ret = read(fd, aad, sizeof(aad));
    assert(ret == sizeof(aad));
  }

  close(fd);
}

static CONTENTS* decryptContent(const CONTENTS* data) {
  EVP_CIPHER_CTX *ctx;

  ctx = EVP_CIPHER_CTX_new();
  assert(ctx);

  int i = 0;

  int len;
  size_t dataLength = data->size;

  switch (cipherMode) {
  case cipherModeCBC:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_192_cbc(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        break;
    }
    assert(i==1);
    break;
  case cipherModeCFB:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_192_cfb(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, key, iv);
        break;
    }
    assert(i==1);
    break;
  case cipherModeOFB:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_128_ofb(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_192_ofb(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_256_ofb(), NULL, key, iv);
        break;
    }
    assert(i==1);
    break;
  case cipherModeCTR:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_192_ctr(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
        break;
    }
    assert(i==1);
    break;
  case cipherModeGCM:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
        break;
      case keyLength192Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_192_gcm(), NULL, NULL, NULL);
        break;
      case keyLength256Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
        break;
    }
    assert(i==1);

    dataLength -= tagLength;

    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, ivLength, NULL);
    assert(i==1);

    i = EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);
    assert(i==1);

    i = EVP_DecryptUpdate(ctx, NULL, &len, aad, sizeof(aad));
    assert(i==1);

    break;
  case cipherModeCCM:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_128_ccm(), NULL, NULL, NULL);
        break;
      case keyLength192Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_192_ccm(), NULL, NULL, NULL);
        break;
      case keyLength256Bit:
        i = EVP_DecryptInit_ex(ctx, EVP_aes_256_ccm(), NULL, NULL, NULL);
        break;
    }
    assert(i==1);

    dataLength -= tagLength;

    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, ivLength, NULL);
    assert(i==1);

    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, tagLength, data->body + dataLength);
    assert(i==1);

    i = EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);
    assert(i==1);

    i = EVP_DecryptUpdate(ctx, NULL, &len, NULL, dataLength);
    assert(i==1);

    i = EVP_DecryptUpdate(ctx, NULL, &len, aad, sizeof(aad));
    assert(i==1);

    break;
  }

  CONTENTS* ret = NULL;
  ret = calloc(1, sizeof(CONTENTS));
  assert(ret);
  ret->body = (unsigned char*)malloc(dataLength);
  assert(ret->body);

  i = EVP_DecryptUpdate(ctx, ret->body, &len, data->body, dataLength);
  assert(i==1);
  ret->size = len;

  if (cipherMode == cipherModeGCM) {
    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tagLength, data->body + dataLength);
    assert(i==1);
  }

  if (cipherMode != cipherModeCCM) {
    i = EVP_DecryptFinal_ex(ctx, ret->body + len, &len);
    assert(i==1);
    ret->size += len;
  }

  EVP_CIPHER_CTX_free(ctx);

  return ret;
}

static CONTENTS *encryptContent(const CONTENTS* data) {
  EVP_CIPHER_CTX *ctx;

  ctx = EVP_CIPHER_CTX_new();
  assert(ctx);

  int len;
  size_t dataLength = data->size + keyLength - 1;

  int i = 0;
  switch (cipherMode) {
  case cipherModeCBC:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_192_cbc(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        break;
    }
    break;
  case cipherModeCFB:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_192_cfb(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, key, iv);
        break;
    }
    break;
  case cipherModeOFB:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_192_cfb(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, key, iv);
        break;
    }
    break;
  case cipherModeCTR:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
        break;
      case keyLength192Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_192_ctr(), NULL, key, iv);
        break;
      case keyLength256Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
        break;
    }
    break;
  case cipherModeGCM:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
        break;
      case keyLength192Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_192_gcm(), NULL, NULL, NULL);
        break;
      case keyLength256Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
        break;
    }

    dataLength += tagLength;

    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, ivLength, NULL);
    assert(i==1);

    i = EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
    assert(i==1);

    i = EVP_EncryptUpdate(ctx, NULL, &len, aad, sizeof(aad));
    assert(i==1);

    break;
  case cipherModeCCM:
    switch (keyLength) {
      case keyLength128Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_128_ccm(), NULL, NULL, NULL);
        break;
      case keyLength192Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_192_ccm(), NULL, NULL, NULL);
        break;
      case keyLength256Bit:
        i = EVP_EncryptInit_ex(ctx, EVP_aes_256_ccm(), NULL, NULL, NULL);
        break;
    }

    dataLength += tagLength;

    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, ivLength, NULL);
    assert(i==1);

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, tagLength, NULL);

    i = EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
    assert(i==1);

    i = EVP_EncryptUpdate(ctx, NULL, &len, NULL, data->size);
    assert(i==1);

    i = EVP_EncryptUpdate(ctx, NULL, &len, aad, sizeof(aad));
    assert(i==1);

    break;
  }
  assert(i==1);

  CONTENTS* ret = NULL;
  ret = calloc(1, sizeof(CONTENTS));
  assert(ret);
  ret->body = (unsigned char*)malloc(dataLength);

  i = EVP_EncryptUpdate(ctx, ret->body, &len, data->body, data->size);
  assert(i==1);
  ret->size = len;

  i = EVP_EncryptFinal_ex(ctx, ret->body + len, &len);
  assert(i==1);
  ret->size += len;

  switch (cipherMode)
  {
  case cipherModeGCM:
    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tagLength, ret->body + ret->size);
    assert(i==1);
    ret->size += tagLength;
    break;
  case cipherModeCCM:
    i = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, tagLength, ret->body + ret->size);
    assert(i==1);
    ret->size += tagLength;
    break;
  }

  EVP_CIPHER_CTX_free(ctx);

  return ret;
}

static void printUsage() {
  fprintf(stderr,
          "Usage: aes_bench \n"
          "[-r seconds <seconds, default is 3>]\n"
          "[-t threads <threads, default is logic cpu cores>]\n"
          "[-k <key length>, should be 128, 192 or 256, default is 128]\n"
          "[-c <cipher mode>, should be CBC, CFB, OFB, GCM, CCM or CTR, default is CBC]\n"
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
    cipherMode = cipherModeCBC;
  } else if (strcmp(mode, "CFB") == 0) {
    cipherMode = cipherModeCFB;
  } else if (strcmp(mode, "OFB") == 0) {
    cipherMode = cipherModeOFB;
  } else if (strcmp(mode, "CTR") == 0) {
    cipherMode = cipherModeCTR;
  } else if (strcmp(mode, "GCM") == 0) {
    cipherMode = cipherModeGCM;
    ivLength = 12;
    tagLength = 16;
  } else if (strcmp(mode, "CCM") == 0) {
    cipherMode = cipherModeCCM;
    ivLength = 7;
    tagLength = 12;
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

  printResult(r, verbose, formated);

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