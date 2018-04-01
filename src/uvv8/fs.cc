#include "uvv8/fs.hh"
#include <iostream>

using namespace v8;

// TODO: Move to util...
template<typename I, typename O>
O get_arg_or(const FunctionCallbackInfo<v8::Value>& args, int n, O other) {
  auto isolate = Isolate::GetCurrent();
  return args.Length() > n
    ? Convert(isolate, args[n].As<I>()).FromMaybe(other)
    : other;
}

//
// Generic async task for all uv_fs_t-based calls
//
class FsTask : public AsyncTask {
  protected:
    uv_fs_t _request;
  public:
    FsTask(Isolate* isolate) : AsyncTask(isolate) {
      _request.data = static_cast<void*>(this);
    }
};


//
// open
//
class FsOpenTask : public FsTask {
  std::string _path;
  int _mode;
  int _flags;
  public:
    FsOpenTask(Isolate* isolate, std::string& path, int mode = UV_FS_O_RDONLY, int flags = 0)
      : FsTask(isolate), _path(path), _mode(mode), _flags(flags) {}

    void Execute() {
      uv_fs_open(uv_default_loop(), &_request, _path.c_str(), _flags, _mode, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsOpenTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          // Reject with an error instance rather than just a string?
          auto error = String::NewFromUtf8(isolate, uv_strerror(result));
          promise->Reject(context, error).FromMaybe(false);
          return;
        }

        auto fd = Number::New(isolate, result);
        promise->Resolve(context, fd).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto mode = get_arg_or<Integer>(args, 1, UV_FS_O_RDONLY);
      auto flags = get_arg_or<Integer>(args, 2, 0);
      auto task = new FsOpenTask(isolate, path, mode, flags);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// close
//
class FsCloseTask : public FsTask {
  int _fd;
  public:
    FsCloseTask(Isolate* isolate, int fd)
      : FsTask(isolate), _fd(fd) {}

    void Execute() {
      uv_fs_close(uv_default_loop(), &_request, _fd, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsCloseTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          // Reject with an error instance rather than just a string?
          auto error = String::NewFromUtf8(isolate, uv_strerror(result));
          promise->Reject(context, error).FromMaybe(false);
          uv_fs_req_cleanup(request);
          delete request;
          return;
        }

        promise->Resolve(context, Undefined(isolate)).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto fd = Convert(isolate, args[0].As<Integer>()).ToChecked();
      auto task = new FsCloseTask(isolate, fd);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// read
//
class FsReadTask : public FsTask {
  int _fd;
  int _size;
  int _offset;

  uv_buf_t _iov;

  uv_buf_t buffer() {
    return _iov;
  }

  public:
    FsReadTask(Isolate* isolate, int64_t fd, int size = 1024, int offset = -1)
      : FsTask(isolate), _fd(fd), _size(size), _offset(offset) {}

    void Execute() {
      char buffer[_size];
      _iov = uv_buf_init(buffer, sizeof(buffer));

      uv_fs_read(uv_default_loop(), &_request, _fd, &_iov, 1, _offset, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsReadTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          // Reject with an error instance rather than just a string?
          auto error = String::NewFromUtf8(isolate, uv_strerror(result));
          promise->Reject(context, error).FromMaybe(false);
          return;
        }

        auto buf = task->buffer();
        buf.len = result;

        // TODO: Create buffer type...
        // auto data = ArrayBuffer::New(isolate, buf.base, buf.len);
        auto data = String::NewFromUtf8(
          isolate,
          buf.base,
          NewStringType::kNormal,
          buf.len
        ).ToLocalChecked();

        promise->Resolve(context, data).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto fd = Convert(isolate, args[0].As<Integer>()).ToChecked();
      auto size = get_arg_or<Integer>(args, 1, 1024);
      auto offset = get_arg_or<Integer>(args, 2, -1);

      auto task = new FsReadTask(isolate, fd, size, offset);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// write
//
class FsWriteTask : public FsTask {
  int _fd;
  int _offset;

  uv_buf_t _iov;

  uv_buf_t buffer() {
    return _iov;
  }

  public:
    FsWriteTask(Isolate* isolate, int64_t fd, std::string& buf, int offset = -1)
      : FsTask(isolate), _fd(fd), _offset(offset) {
        char* chars = strdup(buf.c_str());
        _iov = uv_buf_init(chars, sizeof(chars));
      }

    void Execute() {
      uv_fs_write(uv_default_loop(), &_request, _fd, &_iov, 1, _offset, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsWriteTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          // Reject with an error instance rather than just a string?
          auto error = String::NewFromUtf8(isolate, uv_strerror(result));
          promise->Reject(context, error).FromMaybe(false);
          return;
        }

        auto buf = task->buffer();
        buf.len = result;

        // TODO: Create buffer type...
        // auto data = ArrayBuffer::New(isolate, buf.base, buf.len);
        auto data = String::NewFromUtf8(
          isolate,
          buf.base,
          NewStringType::kNormal,
          buf.len
        ).ToLocalChecked();

        promise->Resolve(context, data).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto fd = Convert(isolate, args[0].As<Integer>()).ToChecked();
      auto data = Convert(isolate, args[1].As<String>()).ToChecked();
      auto offset = get_arg_or<Integer>(args, 2, -1);

      auto task = new FsWriteTask(isolate, fd, data, offset);
      args.GetReturnValue().Set(task->Run());
    }
};

void Fs::Init(Local<Object> exports) {
  auto isolate = Isolate::GetCurrent();

  // Constants
  auto constants = Object::New(isolate);
  exports->Set(
    String::NewFromUtf8(isolate, "constants"),
    constants
  );

  #define CONSTANT(name) \
  constants->Set(\
    String::NewFromUtf8(isolate, #name),\
    Integer::New(isolate, UV_FS_O_##name)\
  );
  CONSTANT(APPEND)
  CONSTANT(CREAT)
  CONSTANT(DIRECT)
  CONSTANT(DIRECTORY)
  CONSTANT(DSYNC)
  CONSTANT(EXCL)
  CONSTANT(EXLOCK)
  CONSTANT(NOATIME)
  CONSTANT(NOCTTY)
  CONSTANT(NOFOLLOW)
  CONSTANT(NONBLOCK)
  CONSTANT(RANDOM)
  CONSTANT(RDONLY)
  CONSTANT(RDWR)
  CONSTANT(SEQUENTIAL)
  CONSTANT(SHORT_LIVED)
  CONSTANT(SYMLINK)
  CONSTANT(SYNC)
  CONSTANT(TEMPORARY)
  CONSTANT(TRUNC)
  CONSTANT(WRONLY)
  #undef CONSTANT

  // Functions
  exports->Set(
    String::NewFromUtf8(isolate, "open"),
    FunctionTemplate::New(isolate, FsOpenTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "close"),
    FunctionTemplate::New(isolate, FsCloseTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "read"),
    FunctionTemplate::New(isolate, FsReadTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "write"),
    FunctionTemplate::New(isolate, FsWriteTask::Call)->GetFunction()
  );
}
