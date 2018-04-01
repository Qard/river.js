#include "module-loader.hh"

//
// Modules
//

using namespace v8;

bool IsAbsolutePath(const std::string& path) {
  #if defined(_WIN32) || defined(_WIN64)
    // TODO(adamk): This is an incorrect approximation, but should
    // work for all our test-running cases.
    return path.find(':') != std::string::npos;
  #else
    return path[0] == '/';
  #endif
}

std::string GetWorkingDirectory() {
  #if defined(_WIN32) || defined(_WIN64)
    char system_buffer[MAX_PATH];
    // TODO(adamk): Support Unicode paths.
    DWORD len = GetCurrentDirectoryA(MAX_PATH, system_buffer);
    CHECK_GT(len, 0);
    return system_buffer;
  #else
    char curdir[PATH_MAX];
    CHECK_NOT_NULL(getcwd(curdir, PATH_MAX));
    return curdir;
  #endif
}

// Returns the directory part of path, without the trailing '/'.
std::string DirName(const std::string& path) {
  DCHECK(IsAbsolutePath(path));
  size_t last_slash = path.find_last_of('/');
  DCHECK(last_slash != std::string::npos);
  return path.substr(0, last_slash);
}

// Resolves path to an absolute path if necessary, and does some
// normalization (eliding references to the current directory
// and replacing backslashes with slashes).
std::string NormalizePath(const std::string& path,
                          const std::string& dir_name) {
  std::string result;
  if (IsAbsolutePath(path)) {
    result = path;
  } else {
    result = dir_name + '/' + path;
  }
  std::replace(result.begin(), result.end(), '\\', '/');
  size_t i;
  while ((i = result.find("/./")) != std::string::npos) {
    result.erase(i, 2);
  }
  return result;
}

void InitializeModuleData(Local<Context> context) {
  context->SetAlignedPointerInEmbedderData(
    kModuleDataIndex,
    new ModuleData(context->GetIsolate())
  );
}

ModuleData* GetModuleDataFromContext(Local<Context> context) {
  return static_cast<ModuleData*>(
    context->GetAlignedPointerFromEmbedderData(kModuleDataIndex)
  );
}

void DisposeModuleData(Local<Context> context) {
  delete GetModuleDataFromContext(context);
  context->SetAlignedPointerInEmbedderData(kModuleDataIndex, nullptr);
}

const std::string& ReadFile(const std::string& path) {
  std::ifstream it(path.c_str());
  const std::string* contents = new std::string(
    (std::istreambuf_iterator<char>(it)),
    std::istreambuf_iterator<char>()
  );
  return *contents;
}

MaybeLocal<Module> compileModule(Local<Context> context, const std::string& file_name, const std::string& source) {
  auto isolate = context->GetIsolate();

  // Script type script origin
  ScriptOrigin script_origin(
    Convert(isolate, file_name).ToLocalChecked(),
    Local<Integer>(),
    Local<Integer>(),
    Local<Boolean>(),
    Local<Integer>(),
    Local<Value>(),
    Local<Boolean>(),
    Local<Boolean>(),
    True(isolate)
  );

  // Script source definition
  ScriptCompiler::Source script_source(Convert(isolate, source).ToLocalChecked(), script_origin);

  // Compile the source code.
  return ScriptCompiler::CompileModule(isolate, &script_source);
}

MaybeLocal<Module> FetchModuleTree(Local<Context> context, const std::string& file_name) {
  DCHECK(IsAbsolutePath(file_name));
  Isolate* isolate = context->GetIsolate();
  auto source_text = ReadFile(file_name.c_str());
  if (source_text.size() == 0) {
    std::string msg = "Error reading: " + file_name;
    Throw(isolate, msg);
    return MaybeLocal<Module>();
  }

  Local<Module> module;
  if (!compileModule(context, file_name, source_text).ToLocal(&module)) {
    return MaybeLocal<Module>();
  }

  // Store compiled module for use later...
  ModuleData* d = GetModuleDataFromContext(context);

  CHECK(d->modules.insert(std::make_pair(
    file_name,
    Global<Module>(isolate, module)
  )).second);

  CHECK(d->specifiers.insert(std::make_pair(
    Global<Module>(isolate, module),
    file_name
  )).second);

  std::string dir_name = DirName(file_name);

  for (int i = 0, length = module->GetModuleRequestsLength(); i < length; ++i) {
    Local<String> name = module->GetModuleRequest(i);
    std::string absolute_path = NormalizePath(
      Convert(isolate, name).ToChecked(),
      dir_name
    );
    if (!d->modules.count(absolute_path)) {
      // If any descendant imports are absent, fail this module
      if (FetchModuleTree(context, absolute_path).IsEmpty()) {
        return MaybeLocal<Module>();
      }
    }
  }

  return module;
}

MaybeLocal<Module> ResolveModuleCallback(Local<Context> context, Local<String> specifier, Local<Module> referrer) {
  Isolate* isolate = context->GetIsolate();
  ModuleData* d = GetModuleDataFromContext(context);


  // Get the normalized specifier for the referrer module
  auto specifier_it = d->specifiers.find(Global<Module>(isolate, referrer));
  CHECK(specifier_it != d->specifiers.end());

  // Determine path relative to referrer module
  std::string absolute_path = NormalizePath(
    Convert(isolate, specifier).ToChecked(),
    DirName(specifier_it->second)
  );

  // Find module matching the normalized specifier
  auto module_it = d->modules.find(absolute_path);
  CHECK(module_it != d->modules.end());

  return module_it->second.Get(isolate);
}

MaybeLocal<Value> runModule(Local<Context> context, const std::string& name) {
  auto isolate = context->GetIsolate();

  // Compile module
  Local<Module> module;
  if (!FetchModuleTree(context, name).ToLocal(&module)) {
    return Throw(isolate, "failed to compile module");
  }

  if (!module->InstantiateModule(context, ResolveModuleCallback).FromMaybe(false)) {
    return Throw(isolate, "failed to instantiate module");
  }

  // Run the script to get the result.
  return module->Evaluate(context);
}

MaybeLocal<Script> compileScript(Local<Context> context, const std::string& file_name, const std::string& source) {
  auto isolate = context->GetIsolate();

  // Script type script origin
  ScriptOrigin script_origin(Convert(isolate, file_name).ToLocalChecked());

  // Script source definition
  ScriptCompiler::Source script_source(Convert(isolate, source).ToLocalChecked(), script_origin);

  // Compile the source code.
  return ScriptCompiler::Compile(context, &script_source);
}

MaybeLocal<Value> runScript(Local<Context> context, const std::string& file_name) {
  auto isolate = context->GetIsolate();

  auto source = ReadFile(file_name.c_str());

  // Compile the source code.
  Local<Script> script;
  if (!compileScript(context, file_name, source).ToLocal(&script)) {
    return Throw(isolate, "failed to compile script");
  }

  // Run the script to get the result.
  return script->Run(context);
}

void HostInitializeImportMetaObject(Local<Context> context, Local<Module> module, Local<Object> meta) {
  Isolate* isolate = context->GetIsolate();
  HandleScope handle_scope(isolate);

  ModuleData* d = GetModuleDataFromContext(context);
  auto specifier_it = d->specifiers.find(Global<Module>(isolate, module));
  CHECK(specifier_it != d->specifiers.end());

  Local<String> url_key = Convert(isolate, "url").ToLocalChecked();
  Local<String> url = Convert(isolate, specifier_it->second.c_str()).ToLocalChecked();

  meta->CreateDataProperty(context, url_key, url).ToChecked();
}
