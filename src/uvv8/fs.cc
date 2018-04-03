#include "uvv8/fs.hh"
#include <iostream>

// TODO:
// - scandir + scandir_next -- Some sort of iterator?

using namespace v8;

// TODO: Move to util...
template<typename I, typename O>
O get_arg_or(const FunctionCallbackInfo<v8::Value>& args, int n, O other) {
  auto isolate = Isolate::GetCurrent();
  return args.Length() > n
    ? Convert(isolate, args[n].As<I>()).FromMaybe(other)
    : other;
}

// TODO: Convert result integer to exception
Local<Value> result_error(int result) {
  auto isolate = Isolate::GetCurrent();
  std::string message;
  message += uv_err_name(result);
  message += ": ";
  message += uv_strerror(result);
  auto error = String::NewFromUtf8(isolate, message.c_str());
  return Exception::Error(error);
}

// Make timespec array
Local<Array> makeTimeSpec(uv_timespec_t timespec) {
  auto isolate = Isolate::GetCurrent();
  auto context = isolate->GetCurrentContext();
  auto times = Array::New(isolate);
  times->Set(context, 0, Number::New(isolate, timespec.tv_sec)).ToChecked();
  times->Set(context, 1, Number::New(isolate, timespec.tv_nsec)).ToChecked();
  return times;
}

// Make stats
Local<Object> makeStats(uv_stat_t statbuf) {
  auto isolate = Isolate::GetCurrent();
  auto stats = Object::New(isolate);
  stats->Set(
    String::NewFromUtf8(isolate, "st_dev"),
    Number::New(isolate, statbuf.st_dev)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_mode"),
    Number::New(isolate, statbuf.st_mode)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_nlink"),
    Number::New(isolate, statbuf.st_nlink)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_uid"),
    Number::New(isolate, statbuf.st_uid)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_gid"),
    Number::New(isolate, statbuf.st_gid)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_rdev"),
    Number::New(isolate, statbuf.st_rdev)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_ino"),
    Number::New(isolate, statbuf.st_ino)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_size"),
    Number::New(isolate, statbuf.st_size)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_blksize"),
    Number::New(isolate, statbuf.st_blksize)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_blocks"),
    Number::New(isolate, statbuf.st_blocks)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_flags"),
    Number::New(isolate, statbuf.st_flags)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_gen"),
    Number::New(isolate, statbuf.st_gen)
  );

  stats->Set(
    String::NewFromUtf8(isolate, "st_atim"),
    makeTimeSpec(statbuf.st_atim)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_mtim"),
    makeTimeSpec(statbuf.st_mtim)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_ctim"),
    makeTimeSpec(statbuf.st_ctim)
  );
  stats->Set(
    String::NewFromUtf8(isolate, "st_birthtim"),
    makeTimeSpec(statbuf.st_birthtim)
  );
  return stats;
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
    FsOpenTask(Isolate* isolate, std::string& path, int flags = 0, int mode = 0)
      : FsTask(isolate), _path(path), _flags(flags), _mode(mode) {}

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
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto flags = get_arg_or<Integer>(args, 1, UV_FS_O_RDONLY);
      auto mode = get_arg_or<Integer>(args, 2, 0);
      auto task = new FsOpenTask(isolate, path, flags, mode);
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
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
  int _offset;

  uv_buf_t _iov;

  uv_buf_t buffer() {
    return _iov;
  }

  public:
    FsReadTask(Isolate* isolate, int64_t fd, int size = 1024, int offset = -1)
      : FsTask(isolate), _fd(fd), _offset(offset) {
        char buffer[size];
        _iov = uv_buf_init(buffer, sizeof(buffer));
      }

    void Execute() {
      uv_fs_read(uv_default_loop(), &_request, _fd, &_iov, 1, _offset, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsReadTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
        _iov = uv_buf_init(strdup(buf.c_str()), buf.size());
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
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto data = Convert(isolate, args[1].As<String>()).ToChecked();
      auto offset = get_arg_or<Integer>(args, 2, -1);

      auto task = new FsWriteTask(isolate, fd, data, offset);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// unlink
//
class FsUnlinkTask : public FsTask {
  std::string _path;

  public:
    FsUnlinkTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_unlink(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsUnlinkTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsUnlinkTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// mkdir
//
class FsMkdirTask : public FsTask {
  std::string _path;
  int _mode;

  public:
    FsMkdirTask(Isolate* isolate, std::string& path, int mode)
      : FsTask(isolate), _path(path), _mode(mode) {}

    void Execute() {
      uv_fs_mkdir(uv_default_loop(), &_request, _path.c_str(), _mode, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsMkdirTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      if (args.Length() < 2) {
        Throw(isolate, "Expected atleast 2 arguments");
        return;
      }

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto mode = Convert(isolate, args[1].As<Integer>()).ToChecked();

      auto task = new FsMkdirTask(isolate, path, mode);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// mkdtemp
//
class FsMkdtempTask : public FsTask {
  std::string _path;

  public:
    FsMkdtempTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_mkdtemp(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsMkdtempTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        auto path = request->path;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        auto data = String::NewFromUtf8(isolate, path);
        promise->Resolve(context, data).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsMkdtempTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// rmdir
//
class FsRmdirTask : public FsTask {
  std::string _path;

  public:
    FsRmdirTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_rmdir(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsRmdirTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsRmdirTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// stat
//
class FsStatTask : public FsTask {
  std::string _path;

  public:
    FsStatTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_stat(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsStatTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        promise->Resolve(context, makeStats(request->statbuf)).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsStatTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// fstat
//
class FsFstatTask : public FsTask {
  int _fd;

  public:
    FsFstatTask(Isolate* isolate, int fd)
      : FsTask(isolate), _fd(fd) {}

    void Execute() {
      uv_fs_fstat(uv_default_loop(), &_request, _fd, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFstatTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        promise->Resolve(context, makeStats(request->statbuf)).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto fd = Convert(isolate, args[0].As<Integer>()).ToChecked();

      auto task = new FsFstatTask(isolate, fd);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// lstat
//
class FsLstatTask : public FsTask {
  std::string _path;

  public:
    FsLstatTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_lstat(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsLstatTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        promise->Resolve(context, makeStats(request->statbuf)).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsLstatTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// rename
//
class FsRenameTask : public FsTask {
  std::string _path;
  std::string _new_path;

  public:
    FsRenameTask(Isolate* isolate, std::string& path, std::string& new_path)
      : FsTask(isolate), _path(path), _new_path(new_path) {}

    void Execute() {
      uv_fs_rename(uv_default_loop(), &_request, _path.c_str(), _new_path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsRenameTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto new_path = Convert(isolate, args[1].As<String>()).ToChecked();

      auto task = new FsRenameTask(isolate, path, new_path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// fsync
//
class FsFsyncTask : public FsTask {
  int _fd;

  public:
    FsFsyncTask(Isolate* isolate, int fd)
      : FsTask(isolate), _fd(fd) {}

    void Execute() {
      uv_fs_fsync(uv_default_loop(), &_request, _fd, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFsyncTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto task = new FsFsyncTask(isolate, fd);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// fdatasync
//
class FsFdatasyncTask : public FsTask {
  int _fd;

  public:
    FsFdatasyncTask(Isolate* isolate, int fd)
      : FsTask(isolate), _fd(fd) {}

    void Execute() {
      uv_fs_fsync(uv_default_loop(), &_request, _fd, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFdatasyncTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto task = new FsFdatasyncTask(isolate, fd);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// ftruncate
//
class FsFtruncateTask : public FsTask {
  int _fd;
  int _offset;

  public:
    FsFtruncateTask(Isolate* isolate, int fd, int offset)
      : FsTask(isolate), _fd(fd), _offset(offset) {}

    void Execute() {
      uv_fs_ftruncate(uv_default_loop(), &_request, _fd, _offset, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFtruncateTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto offset = Convert(isolate, args[1].As<Integer>()).ToChecked();

      auto task = new FsFtruncateTask(isolate, fd, offset);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// copyfile
//
class FsCopyfileTask : public FsTask {
  std::string _path;
  std::string _new_path;
  int _flags;

  public:
    FsCopyfileTask(Isolate* isolate, std::string& path, std::string& new_path, int flags)
      : FsTask(isolate), _path(path), _new_path(new_path), _flags(flags) {}

    void Execute() {
      uv_fs_copyfile(uv_default_loop(), &_request, _path.c_str(), _new_path.c_str(), _flags, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsCopyfileTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto new_path = Convert(isolate, args[1].As<String>()).ToChecked();
      auto flags = get_arg_or<Integer>(args, 2, 0);

      auto task = new FsCopyfileTask(isolate, path, new_path, flags);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// sendfile
//
class FsSendfileTask : public FsTask {
  int _fd;
  int _new_fd;
  int _offset;
  int _length;

  public:
    FsSendfileTask(Isolate* isolate, int fd, int new_fd, int offset, int length)
      : FsTask(isolate), _fd(fd), _new_fd(new_fd), _offset(offset), _length(length) {}

    void Execute() {
      uv_fs_sendfile(uv_default_loop(), &_request, _new_fd, _fd, _offset, _length, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsSendfileTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto new_fd = Convert(isolate, args[1].As<Integer>()).ToChecked();
      auto offset = Convert(isolate, args[2].As<Integer>()).ToChecked();
      auto length = Convert(isolate, args[3].As<Integer>()).ToChecked();

      auto task = new FsSendfileTask(isolate, fd, new_fd, offset, length);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// access
//
class FsAccessTask : public FsTask {
  std::string& _path;
  int _mode;

  public:
    FsAccessTask(Isolate* isolate, std::string& path, int mode)
      : FsTask(isolate), _path(path), _mode(mode) {}

    void Execute() {
      uv_fs_access(uv_default_loop(), &_request, _path.c_str(), _mode, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsAccessTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        promise->Resolve(context, Number::New(isolate, result)).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto mode = Convert(isolate, args[1].As<Integer>()).ToChecked();

      auto task = new FsAccessTask(isolate, path, mode);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// chmod
//
class FsChmodTask : public FsTask {
  std::string& _path;
  int _mode;

  public:
    FsChmodTask(Isolate* isolate, std::string& path, int mode)
      : FsTask(isolate), _path(path), _mode(mode) {}

    void Execute() {
      uv_fs_chmod(uv_default_loop(), &_request, _path.c_str(), _mode, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsChmodTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto mode = Convert(isolate, args[1].As<Integer>()).ToChecked();

      auto task = new FsChmodTask(isolate, path, mode);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// fchmod
//
class FsFchmodTask : public FsTask {
  int _fd;
  int _mode;

  public:
    FsFchmodTask(Isolate* isolate, int fd, int mode)
      : FsTask(isolate), _fd(fd), _mode(mode) {}

    void Execute() {
      uv_fs_fchmod(uv_default_loop(), &_request, _fd, _mode, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFchmodTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto mode = Convert(isolate, args[1].As<Integer>()).ToChecked();

      auto task = new FsFchmodTask(isolate, fd, mode);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// utime
//
class FsUtimeTask : public FsTask {
  std::string& _path;
  double _atime;
  double _mtime;

  public:
    FsUtimeTask(Isolate* isolate, std::string& path, double atime, double mtime)
      : FsTask(isolate), _path(path), _atime(atime), _mtime(mtime) {}

    void Execute() {
      uv_fs_utime(uv_default_loop(), &_request, _path.c_str(), _atime, _mtime, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsUtimeTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto atime = Convert(isolate, args[1].As<Number>()).ToChecked();
      auto mtime = Convert(isolate, args[2].As<Number>()).ToChecked();

      auto task = new FsUtimeTask(isolate, path, atime, mtime);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// futime
//
class FsFutimeTask : public FsTask {
  int _fd;
  double _atime;
  double _mtime;

  public:
    FsFutimeTask(Isolate* isolate, int fd, double atime, double mtime)
      : FsTask(isolate), _fd(fd), _atime(atime), _mtime(mtime) {}

    void Execute() {
      uv_fs_futime(uv_default_loop(), &_request, _fd, _atime, _mtime, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFutimeTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto atime = Convert(isolate, args[1].As<Number>()).ToChecked();
      auto mtime = Convert(isolate, args[2].As<Number>()).ToChecked();

      auto task = new FsFutimeTask(isolate, fd, atime, mtime);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// link
//
class FsLinkTask : public FsTask {
  std::string& _path;
  std::string& _new_path;

  public:
    FsLinkTask(Isolate* isolate, std::string& path, std::string& new_path)
      : FsTask(isolate), _path(path), _new_path(new_path) {}

    void Execute() {
      uv_fs_link(uv_default_loop(), &_request, _path.c_str(), _new_path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsLinkTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto new_path = Convert(isolate, args[1].As<String>()).ToChecked();

      auto task = new FsLinkTask(isolate, path, new_path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// symlink
//
class FsSymlinkTask : public FsTask {
  std::string& _path;
  std::string& _new_path;
  int _flags;

  public:
    FsSymlinkTask(Isolate* isolate, std::string& path, std::string& new_path, int flags = 0)
      : FsTask(isolate), _path(path), _new_path(new_path), _flags(flags) {}

    void Execute() {
      uv_fs_symlink(uv_default_loop(), &_request, _path.c_str(), _new_path.c_str(), _flags, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsSymlinkTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto new_path = Convert(isolate, args[1].As<String>()).ToChecked();
      auto flags = get_arg_or<Integer>(args, 2, 0);

      auto task = new FsSymlinkTask(isolate, path, new_path, flags);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// readlink
//
class FsReadlinkTask : public FsTask {
  std::string& _path;

  public:
    FsReadlinkTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_readlink(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsReadlinkTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        auto path = static_cast<const char*>(request->ptr);
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        auto result_path = Convert(isolate, path).ToLocalChecked();
        promise->Resolve(context, result_path).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsReadlinkTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// realpath
//
class FsRealpathTask : public FsTask {
  std::string& _path;

  public:
    FsRealpathTask(Isolate* isolate, std::string& path)
      : FsTask(isolate), _path(path) {}

    void Execute() {
      uv_fs_realpath(uv_default_loop(), &_request, _path.c_str(), [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsRealpathTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        auto path = static_cast<const char*>(request->ptr);
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
          return;
        }

        auto result_path = Convert(isolate, path).ToLocalChecked();
        promise->Resolve(context, result_path).FromMaybe(false);
        delete task;
      });
    }

    static void Call(const FunctionCallbackInfo<v8::Value>& args) {
      auto isolate = Isolate::GetCurrent();
      auto context = isolate->GetCurrentContext();
      HandleScope scope(isolate);
      Context::Scope context_scope(context);

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();

      auto task = new FsRealpathTask(isolate, path);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// chown
//
class FsChownTask : public FsTask {
  std::string& _path;
  int _uid;
  int _gid;

  public:
    FsChownTask(Isolate* isolate, std::string& path, int uid, int gid)
      : FsTask(isolate), _path(path), _uid(uid), _gid(gid) {}

    void Execute() {
      uv_fs_chown(uv_default_loop(), &_request, _path.c_str(), _uid, _gid, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsChownTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        auto path = static_cast<const char*>(request->ptr);
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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

      auto path = Convert(isolate, args[0].As<String>()).ToChecked();
      auto uid = Convert(isolate, args[1].As<Integer>()).ToChecked();
      auto gid = Convert(isolate, args[2].As<Integer>()).ToChecked();

      auto task = new FsChownTask(isolate, path, uid, gid);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// fchown
//
class FsFchownTask : public FsTask {
  int _fd;
  int _uid;
  int _gid;

  public:
    FsFchownTask(Isolate* isolate, int fd, int uid, int gid)
      : FsTask(isolate), _fd(fd), _uid(uid), _gid(gid) {}

    void Execute() {
      uv_fs_fchown(uv_default_loop(), &_request, _fd, _uid, _gid, [](uv_fs_t* request) {
        auto isolate = Isolate::GetCurrent();
        auto context = isolate->GetCurrentContext();
        HandleScope scope(isolate);

        auto task = static_cast<FsFchownTask*>(request->data);
        auto promise = task->GetResolver();

        auto result = request->result;
        auto path = static_cast<const char*>(request->ptr);
        uv_fs_req_cleanup(request);

        if (result < 0) {
          promise->Reject(context, result_error(result)).FromMaybe(false);
          delete task;
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
      auto uid = Convert(isolate, args[1].As<Integer>()).ToChecked();
      auto gid = Convert(isolate, args[2].As<Integer>()).ToChecked();

      auto task = new FsFchownTask(isolate, fd, uid, gid);
      args.GetReturnValue().Set(task->Run());
    }
};

//
// Initialize V8 bindings
//
void Fs::Init(Local<Object> exports) {
  auto isolate = Isolate::GetCurrent();

  // Constants
  auto constants = Object::New(isolate);
  exports->Set(
    String::NewFromUtf8(isolate, "constants"),
    constants
  );

  #define UV_CONSTANT(name) \
  constants->Set(\
    String::NewFromUtf8(isolate, #name),\
    Integer::New(isolate, UV_FS_O_##name)\
  );
  UV_CONSTANT(APPEND)
  UV_CONSTANT(CREAT)
  UV_CONSTANT(DIRECT)
  UV_CONSTANT(DIRECTORY)
  UV_CONSTANT(DSYNC)
  UV_CONSTANT(EXCL)
  UV_CONSTANT(EXLOCK)
  UV_CONSTANT(NOATIME)
  UV_CONSTANT(NOCTTY)
  UV_CONSTANT(NOFOLLOW)
  UV_CONSTANT(NONBLOCK)
  UV_CONSTANT(RANDOM)
  UV_CONSTANT(RDONLY)
  UV_CONSTANT(RDWR)
  UV_CONSTANT(SEQUENTIAL)
  UV_CONSTANT(SHORT_LIVED)
  UV_CONSTANT(SYMLINK)
  UV_CONSTANT(SYNC)
  UV_CONSTANT(TEMPORARY)
  UV_CONSTANT(TRUNC)
  UV_CONSTANT(WRONLY)
  #undef UV_CONSTANT

  #define CONSTANT(name) \
  constants->Set(\
    String::NewFromUtf8(isolate, #name),\
    Integer::New(isolate, name)\
  );
  // File access
  CONSTANT(F_OK)
  CONSTANT(R_OK)
  CONSTANT(W_OK)
  CONSTANT(X_OK)

  // File open
  CONSTANT(O_RDONLY)
  CONSTANT(O_WRONLY)
  CONSTANT(O_RDWR)
  CONSTANT(O_CREAT)
  CONSTANT(O_EXCL)
  CONSTANT(O_NOCTTY)
  CONSTANT(O_TRUNC)
  CONSTANT(O_APPEND)
  CONSTANT(O_DIRECTORY)
  #ifdef O_NOATIME
  CONSTANT(O_NOATIME)
  #endif
  CONSTANT(O_NOFOLLOW)
  CONSTANT(O_SYNC)
  CONSTANT(O_DSYNC)
  CONSTANT(O_SYMLINK)
  #ifdef O_DIRECT
  CONSTANT(O_DIRECT)
  #endif
  CONSTANT(O_NONBLOCK)

  // File type
  CONSTANT(S_IFMT)
  CONSTANT(S_IFREG)
  CONSTANT(S_IFDIR)
  CONSTANT(S_IFCHR)
  CONSTANT(S_IFBLK)
  CONSTANT(S_IFIFO)
  CONSTANT(S_IFLNK)
  CONSTANT(S_IFSOCK)

  // File mode
  CONSTANT(S_IRWXU)
  CONSTANT(S_IRUSR)
  CONSTANT(S_IWUSR)
  CONSTANT(S_IXUSR)
  CONSTANT(S_IRWXG)
  CONSTANT(S_IRGRP)
  CONSTANT(S_IWGRP)
  CONSTANT(S_IXGRP)
  CONSTANT(S_IRWXO)
  CONSTANT(S_IROTH)
  CONSTANT(S_IWOTH)
  CONSTANT(S_IXOTH)
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
  exports->Set(
    String::NewFromUtf8(isolate, "unlink"),
    FunctionTemplate::New(isolate, FsUnlinkTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "mkdir"),
    FunctionTemplate::New(isolate, FsMkdirTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "mkdtemp"),
    FunctionTemplate::New(isolate, FsMkdtempTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "rmdir"),
    FunctionTemplate::New(isolate, FsRmdirTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "stat"),
    FunctionTemplate::New(isolate, FsStatTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "fstat"),
    FunctionTemplate::New(isolate, FsFstatTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "lstat"),
    FunctionTemplate::New(isolate, FsLstatTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "rename"),
    FunctionTemplate::New(isolate, FsRenameTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "fsync"),
    FunctionTemplate::New(isolate, FsFsyncTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "fdatasync"),
    FunctionTemplate::New(isolate, FsFdatasyncTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "ftruncate"),
    FunctionTemplate::New(isolate, FsFtruncateTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "copyfile"),
    FunctionTemplate::New(isolate, FsCopyfileTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "sendfile"),
    FunctionTemplate::New(isolate, FsSendfileTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "access"),
    FunctionTemplate::New(isolate, FsAccessTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "chmod"),
    FunctionTemplate::New(isolate, FsChmodTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "fchmod"),
    FunctionTemplate::New(isolate, FsFchmodTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "utime"),
    FunctionTemplate::New(isolate, FsUtimeTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "futime"),
    FunctionTemplate::New(isolate, FsFutimeTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "link"),
    FunctionTemplate::New(isolate, FsLinkTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "symlink"),
    FunctionTemplate::New(isolate, FsSymlinkTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "readlink"),
    FunctionTemplate::New(isolate, FsReadlinkTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "realpath"),
    FunctionTemplate::New(isolate, FsRealpathTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "chown"),
    FunctionTemplate::New(isolate, FsChownTask::Call)->GetFunction()
  );
  exports->Set(
    String::NewFromUtf8(isolate, "fchown"),
    FunctionTemplate::New(isolate, FsFchownTask::Call)->GetFunction()
  );
}
