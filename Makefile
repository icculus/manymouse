# Set this variable if you need to.
WINDOWS_JDK_PATH := C:\\Program\ Files\\Java\\jdk1.6.0_02\\

linux := false
macosx := false
cygwin := false

uname_s := $(shell uname -s)
uname_o := $(shell uname -o)
ifeq ($(strip $(uname_s)),Darwin)
  macosx := true
endif
ifeq ($(strip $(uname_s)),Linux)
  linux := true
endif
ifeq ($(strip $(uname_o)),Cygwin)
  cygwin := true
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

JAVAC := javac

ifeq ($(strip $(linux)),true)
  CFLAGS += -I/usr/src/linux/include
endif

ifeq ($(strip $(cygwin)),true)
  CFLAGS += -mno-cygwin
  LDFLAGS += -mno-cygwin
  MANYMOUSEJNILIB := ManyMouse.dll
  JDKPATH := $(WINDOWS_JDK_PATH)
  JAVAC := $(JDKPATH)bin\\javac
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
	$(JAVAC) -d . -classpath contrib/java $<

ManyMouseEvent.class: contrib/java/ManyMouseEvent.java ManyMouse.class
	$(JAVAC) -d . -classpath contrib/java $<

TestManyMouse.class: contrib/java/TestManyMouse.java ManyMouse.class ManyMouseEvent.class
	$(JAVAC) -d . $<


ifeq ($(strip $(macosx)),true)
ManyMouseJava.o: contrib/java/ManyMouseJava.c
	$(CC) $(CFLAGS) -o $@ $< -I/System/Library/Frameworks/JavaVM.framework/Headers

$(MANYMOUSEJNILIB): $(BASEOBJS) ManyMouseJava.o
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^ -bundle -framework JavaVM 

endif


ifeq ($(strip $(cygwin)),true)
ManyMouseJava.o: contrib/java/ManyMouseJava.c
	$(CC) $(CFLAGS) -o $@ $< -I$(JDKPATH)include -I$(JDKPATH)include\\win32

$(MANYMOUSEJNILIB): $(BASEOBJS) ManyMouseJava.o
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^ -Wl,--add-stdcall-alias -shared

endif


# end of Makefile ...

