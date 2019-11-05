
CC=gcc

CODECS=
CODECS+=codec_g729a.so

CFLAGS=
CFLAGS+=-fPIC -pipe -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
#CFLAGS+=-g -O0
CFLAGS+=-O3
CFLAGS+=-I$(ASTDIR)/include

LIBG729A=g729a/g729ab.a

all: $(CODECS)

clean:
	@rm -rf *.so *.o
	! [ -d g729a ] || $(MAKE) -C g729a clean

$(LIBG729A):
	$(MAKE) -C g729a all

codec_g729a.so: codec_g729a.o $(LIBG729A)
	$(CC) -shared -Xlinker -x -o $@ $< $(LIBG729A)

codec_g729a.o : codec_g729a.c 
	$(CC) -c -o $@ $(CFLAGS) $<
