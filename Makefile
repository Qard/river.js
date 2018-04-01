#
# NOTE:
# - *_MAIN are separate from *_SRCS to make it easier to build as libraries
#
CC = gcc
CXX = g++
RM = rm -f

#
# UV-specific configs
#
UV_SRCS = src/uv/loop.cc \
	src/uv/handle.cc \
	src/uv/timer.cc \
	src/uv/prepare.cc \
	src/uv/check.cc \
	src/uv/idle.cc \
	src/uv/async.cc \
	src/uv/poll.cc \
	src/uv/signal.cc \
	src/uv/process.cc \
	src/uv/stream.cc \
	src/uv/tcp.cc

UV_EXE = $(BIN)/uv
UV_MAIN = src/uv/main.cc
UV_OBJS = $(subst .cc,.o,$(UV_MAIN) $(UV_SRCS))

UV_LDLIBS = -luv

#
# UVV8-specific configs
#
UVV8_SRCS = src/uvv8/fs.cc

UVV8_EXE = $(BIN)/uvv8
UVV8_MAIN = src/uvv8/main.cc
UVV8_OBJS = $(subst .cc,.o,$(UVV8_MAIN) $(UVV8_SRCS))

#
# River-specific configs
#
RIVER_SRCS = $(UV_SRCS) \
	$(UVV8_SRCS) \
	src/util/convert.cc \
	src/util/exceptions.cc \
	src/module-loader.cc \
	src/river.cc

RIVER_EXE = $(BIN)/river
RIVER_MAIN = src/main.cc
RIVER_OBJS = $(subst .cc,.o,$(RIVER_MAIN) $(RIVER_SRCS))

# TODO: Should there be a separate V8 target?
V8_LDLIBS = -lv8 -lv8_libplatform -lv8_libbase
RIVER_LDLIBS = $(UV_LDLIBS) $(V8_LDLIBS)

#
# General configs
#
BIN = bin
CPPFLAGS = -std=c++1z -Iinclude
LDFLAGS =

#
# Default task builds river
#
all: $(RIVER_EXE)

#
# UV-specific tasks
#
$(UV_EXE): $(UV_OBJS) $(BIN)
	$(CXX) $(LDFLAGS) -o $@ $(UV_OBJS) $(UV_LDLIBS)

.PHONY: test-uv
test-uv: $(UV_EXE)
	./$(UV_EXE)

#
# UVV8-specific tasks
#
$(UVV8_EXE): $(UVV8_OBJS) $(BIN)
	$(CXX) $(LDFLAGS) -o $@ $(UVV8_OBJS) $(UV_LDLIBS) $(V8_LDLIBS)

.PHONY: test-uvv8
test-uvv8: $(UVV8_EXE)
	./$(UVV8_EXE)

#
# River-specific tasks
#
$(RIVER_EXE): $(RIVER_OBJS) $(BIN)
	$(CXX) $(LDFLAGS) -o $@ $(RIVER_OBJS) $(RIVER_LDLIBS)

.PHONY: test
test: $(RIVER_EXE)
	./$(RIVER_EXE) test/hello.js

#
# General tasks
#
%.o: %.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BIN):
	mkdir -p ./$(BIN)

clean:
	$(RM) $(RIVER_OBJS) $(UV_OBJS) $(UVV8_OBJS)

distclean: clean
	$(RM) $(BIN)
