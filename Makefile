
linux := false
macosx := false

os := $(shell uname -s)
ifeq ($(strip $(os)),Darwin)
  macosx := true
endif
ifeq ($(strip $(os)),Linux)
  linux := true
endif

CFLAGS += -O0 -Wall -g -c
CFLAGS += -I.

#CFLAGS += -ISDL-1.2.8/include
#LDFLAGS += -LSDL-1.2.8/lib -lSDL -lSDLmain

CC := gcc
LD := gcc

ifeq ($(strip $(macosx)),true)
  LDFLAGS += -framework Carbon -framework IOKit
  MANYMOUSEJNILIB := libManyMouse.jnilib
else
  LDFLAGS += -ldl
endif

ifeq ($(strip $(linux)),true)
  CFLAGS += -I/usr/src/linux/include
endif

BASEOBJS := linux_evdev.o macosx_hidmanager.o windows_wminput.o x11_xinput.o manymouse.o

.PHONY: clean all

all: detect_mice test_manymouse_stdio test_manymouse_sdl mmpong manymousepong

clean:
	rm -rf *.o *.obj *.exe *.class *.jnilib example/*.o example/*.obj test_manymouse_stdio test_manymouse_stdio detect_mice mmpong manymousepong

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


# Java support ...

.PHONY: java
java: $(MANYMOUSEJNILIB) ManyMouse.class ManyMouseEvent.class TestManyMouse.class

ManyMouse.class: contrib/java/ManyMouse.java $(MANYMOUSEJNILIB)
	javac -d . -classpath contrib/java $<

ManyMouseEvent.class: contrib/java/ManyMouseEvent.java ManyMouse.class
	javac -d . -classpath contrib/java $<

TestManyMouse.class: contrib/java/TestManyMouse.java ManyMouse.class ManyMouseEvent.class
	javac -d . $<


ifeq ($(strip $(macosx)),true)
ManyMouseJava.o: contrib/java/ManyMouseJava.c
	$(CC) $(CFLAGS) -o $@ $< -I/System/Library/Frameworks/JavaVM.framework/Headers

$(MANYMOUSEJNILIB): $(BASEOBJS) ManyMouseJava.o
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^ -bundle -framework JavaVM 

endif

# end of Makefile ...

