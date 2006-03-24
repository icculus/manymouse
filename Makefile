
macosx := false

CFLAGS += -O0 -Wall -g -c
CFLAGS += -I. -I/usr/src/linux/include

#CFLAGS += -ISDL-1.2.8/include
#LDFLAGS += -LSDL-1.2.8/lib -lSDL -lSDLmain

CC := gcc
LD := gcc

ifeq ($(strip $(macosx)),true)
  LDFLAGS += -framework Carbon -framework IOKit
else
  LDFLAGS += -ldl
endif

BASEOBJS := linux_evdev.o macosx_hidmanager.o windows_wminput.o x11_xinput.o manymouse.o

.PHONY: clean all

all: detect_mice test_manymouse_stdio test_manymouse_sdl mmpong manymousepong

clean:
	rm -f *.o *.obj *.exe example/*.o example/*.obj test_manymouse_stdio test_manymouse_stdio detect_mice mmpong

%.o : %c
	$(CC) $(CFLAGS) -o $@ $<

example/test_manymouse_sdl.o : example/test_manymouse_sdl.c
	$(CC) $(CFLAGS) -o $@ $< `sdl-config --cflags`

example/mmpong.o : example/mmpong.c
	$(CC) $(CFLAGS) -o $@ $< `sdl-config --cflags`

example/manymousepong.o : example/manymousepong.c
	$(CC) $(CFLAGS) -o $@ $< `sdl-config --cflags`

detect_mice: $(BASEOBJS) example/detect_mice.o
	$(LD) $(LDFLAGS) -o $@ $+

test_manymouse_stdio: $(BASEOBJS) example/test_manymouse_stdio.o
	$(LD) $(LDFLAGS) -o $@ $+

test_manymouse_sdl: $(BASEOBJS) example/test_manymouse_sdl.o
	$(LD) $(LDFLAGS) -o $@ $+ `sdl-config --libs`

mmpong: $(BASEOBJS) example/mmpong.o
	$(LD) $(LDFLAGS) -o $@ $+ `sdl-config --libs`

manymousepong: $(BASEOBJS) example/manymousepong.o
	$(LD) $(LDFLAGS) -o $@ $+ `sdl-config --libs`

# end of Makefile ...

