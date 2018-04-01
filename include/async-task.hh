#pragma once

#include <v8.h>

class AsyncTask {
  v8::Persistent<v8::Promise::Resolver> _persistent;
  v8::Isolate* _isolate;
  protected:
    AsyncTask(v8::Isolate* isolate) : _isolate(isolate) {
      auto context = isolate->GetCurrentContext();
      auto resolver = v8::Promise::Resolver::New(context);
      _persistent.Reset(isolate, resolver.ToLocalChecked());
    }

    v8::Local<v8::Promise::Resolver> GetResolver() {
      return v8::Local<v8::Promise::Resolver>::New(_isolate, _persistent);
    }

    v8::Local<v8::Promise> GetPromise() {
      return this->GetResolver()->GetPromise();
    }

  public:
    // Sub-classes just need to implement this method.
    // NOTE: Execute is public so Run can just be a convenience method.
    // This allows things like deferred calling of Execute.
    virtual void Execute() = 0;

    v8::Local<v8::Promise> Run() {
      this->Execute();
      return this->GetPromise();
    }
};
