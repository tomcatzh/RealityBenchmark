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

const EVP_MD *md;

static CONTENTS* mdContent(const CONTENTS* data) {
    EVP_MD_CTX ctx;
    CONTENTS *mdResult = NULL;
    int i = 0;

    mdResult = calloc(1, sizeof(CONTENTS));
    assert(mdResult);
    mdResult->body = malloc(EVP_MAX_MD_SIZE);
    assert(mdResult->body);

    unsigned int md_len;

    EVP_MD_CTX_init(&ctx);

    i = EVP_DigestInit_ex(&ctx, md, NULL);
    assert(i==1);

    i = EVP_DigestUpdate(&ctx, data->body, data->size);
    assert(i==1);

    i = EVP_DigestFinal_ex(&ctx, mdResult->body, &md_len);
    assert(i==1);
    mdResult->size = md_len;

    EVP_MD_CTX_cleanup(&ctx);

    return mdResult;
}

static void printUsage() {
  fprintf(stderr,
          "Usage: md_bench \n"
          "[-r seconds <seconds, default is 3>]\n"
          "[-t threads <threads, default is logic cpu cores>]\n"
          "[-m <digestname>, should be md5, sha1, sha224, sha256, sha512, dss, dss1, mdc2, ripemd160, default is sha256]\n"
          "[-v <verbose json output>] [-f <formated json output>]\n"
          "-u size <use random data block, size can use K, M, G>|file|url\n");
}

int main(int argc, char **argv) {
    int ret = -1;
    CONTENTS *contents = NULL;
    CONTENTS *mdResult = NULL;

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

    OpenSSL_add_all_digests();

    while ((c = getopt(argc, argv, "r:t:m:vfu:")) != -1) {
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
        case 'm':
            md = EVP_get_digestbyname(optarg);
            if (!md) {
                printUsage();
                goto END;
            }
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

    mdResult = mdContent(contents);

    TEST *t = testNew();
    testSetThreads(t, threads);
    testSetTimeout(t, &timeout);
    testAddRun(t, &mdContent);
    testSetInput(t, contents);
    testSetTesting(t, mdResult);

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
    if (mdResult) {
        destroyContents(mdResult);
        free(mdResult);
        mdResult = NULL;
    }
    return ret;
}