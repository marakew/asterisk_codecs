
CC=gcc

CODECS=
CODECS+=codec_g729a.so

CFLAGS=
CFLAGS+=-fPIC
#CFLAGS+=-g -O0
CFLAGS+=-O3
CFLAGS+=-I$(ASTDIR)/include

G729AOBJS=$(patsubst %.c,%.o,$(wildcard g729a/*.c))

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $<

codec_g729a.o : codec_g729a.c
	$(CC) -c -o $@ $(CFLAGS) $<

all: $(CODECS)

clean:
	rm -rf *.so *.o

codec_g729a.so: codec_g729a.o $(G729AOBJS)
	$(CC) -shared -o $@ codec_g729a.o $(G729AOBJS)