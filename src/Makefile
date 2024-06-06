# X-BASIC to C converter bas2c
# Copyright (c) 2024 Yuichi Nakamura (@yunkya2)
#
# The MIT License (MIT)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

CC = g++
CFLAGS = -g -MMD -MP
LDFLAGS =
TARGET = bas2c

ifdef X68k
CC = m68k-xelf-g++
CFLAGS = -O3 -MMD -MP -DX68k
LDFLAGS = -s
TARGET = bas2c.x
endif

GIT_REPO_VERSION=$(shell git describe --tags --always)
CFLAGS += -DGIT_REPO_VERSION=\"$(GIT_REPO_VERSION)\"

all: $(TARGET) bas2c.def

test: bas2c.def
	$(MAKE) -f Makefile.test

SRCS = main.cpp keyword.cpp namespace.cpp variable.cpp token.cpp tokengen.cpp \
       bas2c.cpp bas2cstmt.cpp bas2cexpr.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

bas2c.def: ../bas2c.def
	cat $^ | iconv -f utf-8 -t cp932 | sed 's/$$/\r/' > $@

RELFILE = bas2c-$(GIT_REPO_VERSION)

release: clean
	$(MAKE) X68k=1
	cat ../README-bas2c.txt | iconv -f utf-8 -t cp932 | sed 's/$$/\r/' > README.txt
	rm -f $(RELFILE).zip
	zip -r $(RELFILE).zip README.txt bas2c.x bas2c.def

clean:
	-rm -rf $(TARGET) $(OBJS) $(DEPS) *.x* $(SRCS:.cpp=) bas2c.def

.PHONY: all clean test release
