#pragma once
// TODO: Convert JS promise to a C++ alternative?

#include <string>

#include <v8.h>

#include "util/check.hh"

//
// Strings
//
v8::MaybeLocal<v8::String> Convert(v8::Isolate* isolate, const char* string);
v8::MaybeLocal<v8::String> Convert(v8::Isolate* isolate, const std::string& string);
v8::Maybe<std::string> Convert(v8::Isolate* isolate, v8::Local<v8::String> string);

//
// Numbers
//
v8::MaybeLocal<v8::Number> Convert(v8::Isolate* isolate, double value);
v8::Maybe<double> Convert(v8::Isolate* isolate, v8::Local<v8::Number> value);
v8::MaybeLocal<v8::Integer> Convert(v8::Isolate* isolate, int32_t value);
v8::Maybe<int32_t> Convert(v8::Isolate* isolate, v8::Local<v8::Int32> value);
v8::MaybeLocal<v8::Integer> Convert(v8::Isolate* isolate, uint32_t value);
v8::Maybe<uint32_t> Convert(v8::Isolate* isolate, v8::Local<v8::Uint32> value);
v8::MaybeLocal<v8::Integer> Convert(v8::Isolate* isolate, int64_t value);
v8::Maybe<int64_t> Convert(v8::Isolate* isolate, v8::Local<v8::Integer> value);

//
// Booleans
//
v8::MaybeLocal<v8::Boolean> Convert(v8::Isolate* isolate, bool value);
v8::Maybe<bool> Convert(v8::Isolate* isolate, v8::Local<v8::Boolean> value);
