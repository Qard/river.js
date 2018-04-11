#include <river/river.hh>

using namespace v8;

void River::main(int argc, char** argv) {
  // Initialize V8.
  V8::InitializeICUDefaultLocation(argv[0]);
  V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<Platform> platform = std::unique_ptr<Platform>(
    platform::CreateDefaultPlatform(0, platform::IdleTaskSupport::kEnabled)
  );
  V8::InitializePlatform(platform.get());
  V8::Initialize();
  V8::SetFlagsFromCommandLine(&argc, argv, true);
  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
  Isolate* isolate = Isolate::New(create_params);

  platform::EnsureEventLoopInitialized(platform.get(), isolate);
  {
    Isolate::Scope isolate_scope(isolate);
    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);
    // Create a new context.
    Local<Context> context = Context::New(isolate);
    // Enter the context for compiling and running the hello world script.
    Context::Scope context_scope(context);
    // Run everything in a try/catch
    TryCatch try_catch(isolate);

    Modules::Init(context);

    // TODO: Figure out where exports is coming from...
    // maybe use "global" for now?
    Local<Object> global = context->Global();

    // Namespace fs functions in fs object
    // TODO: This should be an es module instead,
    // when I figure out how to make them natively.
    auto fs = Object::New(isolate);
    global->Set(String::NewFromUtf8(isolate, "fs"), fs);
    Fs::Init(fs);

    // Add `cwd` property to global
    // TODO: Find a better place for this?
    global->SetAccessor(
      context,
      String::NewFromUtf8(isolate, "cwd"),
      [](Local<Name> property, const PropertyCallbackInfo<Value> &info) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);

        auto cwd = String::NewFromUtf8(isolate, GetWorkingDirectory().c_str());
        info.GetReturnValue().Set(cwd);
      }
    ).ToChecked();

    // Create global print function
    // TODO: Create `console` object
    global->Set(
      String::NewFromUtf8(isolate, "print"),
      FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);

        auto count = args.Length();
        for (int i = 0; i < count; i++) {
          String::Utf8Value utf8(args[i]);
          std::cout << *utf8 << " ";
        }
        std::cout << std::endl;
      })->GetFunction()
    );

    // Create a string containing the JavaScript source code.
    std::string file_path(argv[1]);
    auto full_path = IsAbsolutePath(file_path)
      ? file_path
      : NormalizePath(file_path, GetWorkingDirectory());

    // Run resolver
    // ResolveRelative(file_path, GetWorkingDirectory());

    // Convert the result to an UTF8 string and print it.
    Local<Value> result;
    if (!Modules::runModule(context, full_path).ToLocal(&result)) {
      Throw(isolate, "failed to run module");
      return;
    }

    if (try_catch.HasCaught()) {
      ReportException(isolate, &try_catch);
    } else {
      String::Utf8Value utf8(result);
      printf("%s\n", *utf8);
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    Modules::Dispose(context);
  }
  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  V8::Dispose();
  V8::ShutdownPlatform();
};
