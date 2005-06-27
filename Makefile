
macosx := true

CFLAGS += -O0 -Wall -g -c

CFLAGS += -I.

CC := gcc
LD := gcc

ifeq ($(strip $(macosx)),true)
  LDFLAGS += -framework Carbon -framework IOKit
endif

BASEOBJS := linux_evdev.o macosx_hidmanager.o multimouse.o

.PHONY: clean all

all: detect_mice test_multimouse_stdio test_multimouse_sdl

clean:
	rm -f *.o example/*.o test_multimouse_stdio test_multimouse_stdio detect_mice

%.o : %c
	$(CC) $(CFLAGS) -o $@ $+

example/test_multimouse_sdl.o : example/test_multimouse_sdl.c
	$(CC) $(CFLAGS) -o $@ $+ `sdl-config --cflags`

detect_mice: $(BASEOBJS) example/detect_mice.o
	$(LD) $(LDFLAGS) -o $@ $+

test_multimouse_stdio: $(BASEOBJS) example/test_multimouse_stdio.o
	$(LD) $(LDFLAGS) -o $@ $+

test_multimouse_sdl: $(BASEOBJS) example/test_multimouse_sdl.o
	$(LD) $(LDFLAGS) -o $@ $+ `sdl-config --libs`

# end of Makefile ...

