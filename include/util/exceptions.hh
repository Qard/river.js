#pragma once

#include <v8.h>

#include "util/convert.hh"

v8::Local<v8::Value> Throw(v8::Isolate* isolate, const std::string& message);

void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch);
