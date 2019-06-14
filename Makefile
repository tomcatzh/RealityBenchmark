CC=gcc
CFLAGS=-I. -Wall -g
DEPS = contents.h misc.h benchmark.h external/cJSON.h
TARGET = zlib_bench
LIBS = -lcurl -lz -pthread -lm
ZLIB_OBJS = contents.o zlib_bench.o misc.o benchmark.o external/cJSON.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

zlib_bench: $(ZLIB_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET) *.o core