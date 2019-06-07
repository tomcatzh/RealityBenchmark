CC=gcc
CFLAGS=-I. -Wall -g
DEPS = contents.h zlib_bench.h
TARGET = zlib_bench
LIBS = -lcurl -lz -pthread -lm
OBJS = contents.o main.o zlib_bench.o misc.o benchmark.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

zlib_bench: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET) *.o core