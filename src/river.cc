#include "river.hh"

using namespace v8;

void River::main(int argc, char** argv) {
  // Initialize V8.
  V8::InitializeICUDefaultLocation(argv[0]);
  V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<Platform> platform = platform::NewDefaultPlatform(
    0,
    platform::IdleTaskSupport::kEnabled
  );
  V8::InitializePlatform(platform.get());
  V8::Initialize();
  V8::SetFlagsFromCommandLine(&argc, argv, true);
  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
  Isolate* isolate = Isolate::New(create_params);

  // ES Module loading hooks
  // isolate->SetHostImportModuleDynamicallyCallback(HostImportModuleDynamically);
  isolate->SetHostInitializeImportMetaObjectCallback(HostInitializeImportMetaObject);

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

    InitializeModuleData(context);

    // TODO: Figure out where exports is coming from...
    // maybe use "global" for now?
    Local<Object> global = context->Global();

    Fs::Init(global);

    global->SetAccessor(
      context,
      String::NewFromUtf8(isolate, "cwd"),
      [](Local< Name > property, const PropertyCallbackInfo< Value > &info) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);

        std::string cwd_string;
        size_t size = 1024;
        char buf[size];
        if (uv_cwd(buf, &size) == UV_ENOBUFS) {
          char buf2[size];
          uv_cwd(buf2, &size);
          cwd_string = buf2;
        } else {
          cwd_string = buf;
        }

        auto cwd = String::NewFromUtf8(isolate, cwd_string.c_str());
        info.GetReturnValue().Set(cwd);
      }
    );

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

    // Convert the result to an UTF8 string and print it.
    Local<Value> result;
    if (!runModule(context, full_path).ToLocal(&result)) {
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
  }
  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  V8::Dispose();
  V8::ShutdownPlatform();
};
