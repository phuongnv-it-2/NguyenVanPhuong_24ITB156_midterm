# Makefile của myls - bản ls(1) rút gọn
CC       ?= gcc
CPPFLAGS += -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
            -D_FILE_OFFSET_BITS=64
CFLAGS   ?= -g -O2
CFLAGS   += -std=c11 -Wall -Wextra -Wpedantic

TARGET   = myls
SRC      = $(wildcard src/*.c)
OBJ      = $(patsubst src/%.c,build/%.o,$(SRC))
DEP      = $(OBJ:.o=.d)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

test: $(TARGET)
	sh tests/run_tests.sh

clean:
	rm -rf build $(TARGET)

-include $(DEP)
