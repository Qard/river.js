#include <river/module-loader.hh>
#include <iostream>

using namespace v8;

//
// Private
//
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
  ScriptCompiler::Source script_source(
    Convert(isolate, source).ToLocalChecked(),
    script_origin
  );

  // Compile the source code.
  return ScriptCompiler::CompileModule(isolate, &script_source);
}

MaybeLocal<Script> compileScript(Local<Context> context, const std::string& file_name, const std::string& source) {
  auto isolate = context->GetIsolate();

  // Script type script origin
  ScriptOrigin script_origin(
    Convert(isolate, file_name).ToLocalChecked()
  );

  // Script source definition
  ScriptCompiler::Source script_source(
    Convert(isolate, source).ToLocalChecked(),
    script_origin
  );

  // Compile the source code.
  return ScriptCompiler::Compile(context, &script_source);
}

class ModuleData {
  class ModuleHash {
    v8::Isolate* isolate_;
    public:
      explicit ModuleHash(v8::Isolate* isolate) : isolate_(isolate) {}
      size_t operator()(const v8::Global<v8::Module>& module) const {
        return module.Get(isolate_)->GetIdentityHash();
      }
  };

  public:
    explicit ModuleData(v8::Isolate* isolate)
      : specifiers(10, ModuleHash(isolate)) {}

    // Map from normalized module specifier to Module.
    std::unordered_map<std::string, v8::Global<v8::Module>> modules;
    // Map from Module to its URL as defined in the ScriptOrigin
    std::unordered_map<v8::Global<v8::Module>, std::string, ModuleHash> specifiers;
};

// Aligned pointer fields
enum {
  kModuleDataIndex
};

ModuleData* GetModuleDataFromContext(Local<Context> context) {
  return static_cast<ModuleData*>(
    context->GetAlignedPointerFromEmbedderData(kModuleDataIndex)
  );
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
  ModuleData* data = GetModuleDataFromContext(context);

  CHECK(data->modules.insert(std::make_pair(
    file_name,
    Global<Module>(isolate, module)
  )).second);

  CHECK(data->specifiers.insert(std::make_pair(
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

    std::cout
      << "name: " << Convert(isolate, name).ToChecked() << ", "
      << "absolute_path: " << absolute_path
    << std::endl;

    if (!data->modules.count(absolute_path)) {
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

// void HostInitializeImportMetaObject(Local<Context> context, Local<Module> module, Local<Object> meta) {
//   auto isolate = context->GetIsolate();
//   HandleScope handle_scope(isolate);
//
//   auto data = GetModuleDataFromContext(context);
//   auto specifier_it = data->specifiers.find(Global<Module>(isolate, module));
//   CHECK(specifier_it != data->specifiers.end());
//
//   auto url_key = Convert(isolate, "url").ToLocalChecked();
//   auto url = Convert(isolate, specifier_it->second.c_str()).ToLocalChecked();
//
//   meta->CreateDataProperty(context, url_key, url).ToChecked();
// }

//
// Public
//
namespace Modules {
  void Init(Local<Context> context) {
    auto isolate = context->GetIsolate();
    auto data = new ModuleData(isolate);
    context->SetAlignedPointerInEmbedderData(kModuleDataIndex, data);

    // ES Module loading hooks
    // isolate->SetHostImportModuleDynamicallyCallback(HostImportModuleDynamically);
    // isolate->SetHostInitializeImportMetaObjectCallback(HostInitializeImportMetaObject);
  }

  void Dispose(Local<Context> context) {
    delete GetModuleDataFromContext(context);
    context->SetAlignedPointerInEmbedderData(kModuleDataIndex, nullptr);
  }

  MaybeLocal<Value> runModule(Local<Context> context, const std::string& name) {
    auto isolate = context->GetIsolate();

    // Compile module
    Local<Module> module;
    if (!FetchModuleTree(context, name).ToLocal(&module)) {
      return Throw(isolate, "failed to compile module");
    }

    // Instantiate module
    if (!module->InstantiateModule(context, ResolveModuleCallback).FromMaybe(false)) {
      return Throw(isolate, "failed to instantiate module");
    }

    // Run the script to get the result.
    return module->Evaluate(context);
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
}
