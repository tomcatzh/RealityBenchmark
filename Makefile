CC=gcc
CFLAGS=-I. -Wall -g -I/usr/local/opt/openssl/include
DEPS = contents.h misc.h benchmark.h external/cJSON.h
TARGET = zlib_bench aes_bench
LIBS = -lcurl -lz -pthread -lm -lcrypto -L/usr/local/opt/openssl/lib
COMMON_OBJS = contents.o misc.o benchmark.o external/cJSON.o
ZLIB_OBJS = zlib_bench.o
AES_OBJS = aes_bench.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(TARGET)

zlib_bench: $(ZLIB_OBJS) $(COMMON_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes_bench: $(AES_OBJS) $(COMMON_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET) *.o core