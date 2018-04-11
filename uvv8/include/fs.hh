#pragma once

#include <string.h>
#include <unistd.h>

#include <uv.h>
#include <v8.h>

#include "uvv8/util/convert.hh"
#include "uvv8/util/exceptions.hh"
#include "uvv8/async-task.hh"

struct Fs {
  static void Init(v8::Local<v8::Object>);
};
