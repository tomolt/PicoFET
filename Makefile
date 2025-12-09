.POSIX:

CC=arm-none-eabi-gcc
RM=rm -f
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic
CPPFLAGS=
LDFLAGS=--specs=nosys.specs
LIBS=

SOURCES=\
	src/cmd.c\
	src/feta.c\
	src/jtaglib.c\
	# end of sources list

OBJECTS=$(SOURCES:.c=.o)

.PHONY: all clean

all: FETA_pico.elf

clean:
	$(RM) $(OBJECTS) FETA_pico.elf

FETA_pico.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $(@:.o=.c) $(CPPFLAGS)

src/cmd.o: src/status.h src/jtaglib.h src/jtdev.h src/util.h
src/feta.o: src/util.h
src/jtaglib.o: src/jtaglib.h src/jtdev.h src/util.h src/eem_defs.h src/status.h
