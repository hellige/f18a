CC = gcc
DEBUG = 
CFLAGS = -ggdb3 -std=gnu99 -O3 -Wall -Wextra -pedantic $(DEBUG) $(PLATCFLAGS)

LIBS = -lncurses

PLATCFLAGS = 
PLATLDFLAGS = 
PLATLIBS = 

system := $(shell uname)
ifeq ($(system),Linux)
    PLATCFLAGS = -fdiagnostics-show-option -fpic -DF18A_LINUX
    PLATLIBS = # -lrt
endif
ifeq ($(system),Darwin)
    DARWIN_ARCH = x86_64 # i386
    PLATCFLAGS = -fPIC -DF18A_MACOSX # -arch $(DARWIN_ARCH)
    PLATLDFLAGS = # -arch $(DARWIN_ARCH)
endif

MAIN_DIR = emulator
MAIN_S = debugger.c emulator.c f18a.c opcodes.c terminal.c
MAIN_O = $(patsubst %.c,out/%.o,$(MAIN_S))

ALL_O = $(MAIN_O)
ALL_T = f18a


default: all

all: $(ALL_T)

f18a: $(MAIN_O)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(PLATLDFLAGS) $^ $(LIBS)

$(MAIN_O):out/%.o: $(MAIN_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $(CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" \
	    -MT"$(@:%.o=%.d)" $<

clean:
	-rm -f $(ALL_T) $(ALL_O)

spotless: clean
	-rm -rf out

-include $(ALL_O:.o=.d)

.PHONY: default all clean spotless
