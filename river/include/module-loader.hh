#pragma once

#include <cstdio>
#include <assert.h>
#include <unordered_map>
#include <unistd.h>
#include <fstream>

#include <v8.h>

#include <uvv8/uvv8.hh>

//
// Modules
//

namespace Modules {
  void Init(v8::Local<v8::Context> context);
  void Dispose(v8::Local<v8::Context> context);

  v8::MaybeLocal<v8::Value> runModule(v8::Local<v8::Context> context, const std::string& name);
  v8::MaybeLocal<v8::Value> runScript(v8::Local<v8::Context> context, const std::string& file_name);
};
