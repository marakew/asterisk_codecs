
CC=gcc

CODECS=
CODECS+=codec_g729a.so

CFLAGS=
CFLAGS+=-fPIC
#CFLAGS+=-g -O0
CFLAGS+=-O3
CFLAGS+=-I$(ASTDIR)/include

G729AOBJS=$(patsubst %.c,%.o,$(wildcard g729a/*.c))

all: $(CODECS)

g729a/%.o: g729a/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

codec_g729a.o : codec_g729a.c
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	@rm -rf *.so *.o $(G729AOBJS)

codec_g729a.so: $(G729AOBJS) codec_g729a.o
	$(CC) -shared -o $@ codec_g729a.o $(G729AOBJS)