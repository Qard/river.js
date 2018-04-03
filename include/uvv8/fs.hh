#pragma once

#include <string>
#include <unistd.h>

#include <uv.h>
#include <v8.h>

#include "util/convert.hh"
#include "util/exceptions.hh"
#include "async-task.hh"

struct Fs {
  static void Init(v8::Local<v8::Object>);
};
