#include "uvv8/util/convert.hh"

//
// Strings
//
v8::MaybeLocal<v8::String> Convert(v8::Isolate* isolate, const char* string) {
  return v8::String::NewFromUtf8(isolate, string);
}
v8::MaybeLocal<v8::String> Convert(v8::Isolate* isolate, const std::string& string) {
  return v8::String::NewFromUtf8(isolate, string.c_str());
}
v8::Maybe<std::string> Convert(v8::Isolate* isolate, v8::Local<v8::String> string) {
  v8::String::Utf8Value utf8(isolate, string);
  if (*utf8 == NULL) return v8::Nothing<std::string>();
  return v8::Just<std::string>(*utf8);
}

//
// Numbers
//
v8::MaybeLocal<v8::Number> Convert(v8::Isolate* isolate, double value) {
  return v8::Number::New(isolate, value);
}
v8::Maybe<double> Convert(v8::Isolate* isolate, v8::Local<v8::Number> value) {
  return value->NumberValue(isolate->GetCurrentContext());
}
v8::MaybeLocal<v8::Integer> Convert(v8::Isolate* isolate, int32_t value) {
  return v8::Integer::New(isolate, value);
}
v8::Maybe<int32_t> Convert(v8::Isolate* isolate, v8::Local<v8::Int32> value) {
  return value->Int32Value(isolate->GetCurrentContext());
}
v8::MaybeLocal<v8::Integer> Convert(v8::Isolate* isolate, uint32_t value) {
  return v8::Integer::NewFromUnsigned(isolate, value);
}
v8::Maybe<uint32_t> Convert(v8::Isolate* isolate, v8::Local<v8::Uint32> value) {
  return value->Uint32Value(isolate->GetCurrentContext());
}
v8::MaybeLocal<v8::Integer> Convert(v8::Isolate* isolate, int64_t value) {
  return v8::Integer::New(isolate, value);
}
v8::Maybe<int64_t> Convert(v8::Isolate* isolate, v8::Local<v8::Integer> value) {
  return value->IntegerValue(isolate->GetCurrentContext());
}

//
// Booleans
//
v8::MaybeLocal<v8::Boolean> Convert(v8::Isolate* isolate, bool value) {
  return v8::Boolean::New(isolate, value);
}
v8::Maybe<bool> Convert(v8::Isolate* isolate, v8::Local<v8::Boolean> value) {
  return value->BooleanValue(isolate->GetCurrentContext());
}
