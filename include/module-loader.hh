#pragma once

#include <cstdio>
#include <assert.h>
#include <unordered_map>
#include <unistd.h>
#include <fstream>

#include <v8.h>

#include "util/check.hh"
#include "util/convert.hh"
#include "util/exceptions.hh"

//
// Modules
//

bool IsAbsolutePath(const std::string& path);

std::string GetWorkingDirectory();

// Returns the directory part of path, without the trailing '/'.
std::string DirName(const std::string& path);

// Resolves path to an absolute path if necessary, and does some
// normalization (eliding references to the current directory
// and replacing backslashes with slashes).
std::string NormalizePath(const std::string& path,
                          const std::string& dir_name);

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

void InitializeModuleData(v8::Local<v8::Context> context);

ModuleData* GetModuleDataFromContext(v8::Local<v8::Context> context);

void DisposeModuleData(v8::Local<v8::Context> context);

const std::string& ReadFile(const std::string& path);

v8::MaybeLocal<v8::Module> compileModule(v8::Local<v8::Context> context, const std::string& file_name, const std::string& source);

v8::MaybeLocal<v8::Module> FetchModuleTree(v8::Local<v8::Context> context, const std::string& file_name);

v8::MaybeLocal<v8::Module> ResolveModuleCallback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::Module> referrer);

v8::MaybeLocal<v8::Value> runModule(v8::Local<v8::Context> context, const std::string& name);

v8::MaybeLocal<v8::Script> compileScript(v8::Local<v8::Context> context, const std::string& file_name, const std::string& source);

v8::MaybeLocal<v8::Value> runScript(v8::Local<v8::Context> context, const std::string& file_name);

void HostInitializeImportMetaObject(v8::Local<v8::Context> context, v8::Local<v8::Module> module, v8::Local<v8::Object> meta);
