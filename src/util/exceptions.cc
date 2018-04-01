#include "util/exceptions.hh"

v8::Local<v8::Value> Throw(v8::Isolate* isolate, const std::string& message) {
  return isolate->ThrowException(Convert(isolate, message).ToLocalChecked());
}

void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
  using namespace v8;
  HandleScope handle_scope(isolate);
  Local<Context> context = isolate->GetCurrentContext();
  bool enter_context = context.IsEmpty();
  if (enter_context) {
    // context = Local<Context>::New(isolate, evaluation_context_);
    context->Enter();
  }
  // Converts a V8 value to a C string.
  auto ToCString = [](const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
  };

  v8::String::Utf8Value exception(isolate, try_catch->Exception());
  const char* exception_string = ToCString(exception);
  Local<Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    printf("%s\n", exception_string);
  } else if (message->GetScriptOrigin().Options().IsWasm()) {
    // Print wasm-function[(function index)]:(offset): (message).
    int function_index = message->GetLineNumber(context).FromJust() - 1;
    int offset = message->GetStartColumn(context).FromJust();
    printf("wasm-function[%d]:%d: %s\n", function_index, offset,
           exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(isolate,
                                   message->GetScriptOrigin().ResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber(context).FromMaybe(-1);
    printf("%s:%i: %s\n", filename_string, linenum, exception_string);
    Local<String> sourceline;
    if (message->GetSourceLine(context).ToLocal(&sourceline)) {
      // Print line of source code.
      v8::String::Utf8Value sourcelinevalue(isolate, sourceline);
      const char* sourceline_string = ToCString(sourcelinevalue);
      printf("%s\n", sourceline_string);
      // Print wavy underline (GetUnderline is deprecated).
      int start = message->GetStartColumn(context).FromJust();
      for (int i = 0; i < start; i++) {
        printf(" ");
      }
      int end = message->GetEndColumn(context).FromJust();
      for (int i = start; i < end; i++) {
        printf("^");
      }
      printf("\n");
    }
  }
  Local<Value> stack_trace_string;
  if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
      stack_trace_string->IsString()) {
    v8::String::Utf8Value stack_trace(isolate,
                                      Local<String>::Cast(stack_trace_string));
    printf("%s\n", ToCString(stack_trace));
  }
  printf("\n");
  if (enter_context) context->Exit();
}
