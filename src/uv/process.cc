#include "uv/process.hh"

ProcessOptions::ProcessOptions(std::initializer_list<const char*> list) {
  memset(this, 0, sizeof(*this));
  int i = 0;
  int n = list.size() + 1;

  char** args = new char*[n];
  for (auto elem : list) {
    args[i++] = strdup(elem);
  }
  args[i++] = NULL;

  this->flags = 0;
  this->file = args[0];
  this->args = args;
}

Process::Process(uv_loop_t* loop, const uv_process_options_t* options) {
  int r = uv_spawn(loop, this, options);
  if (r) {
    fprintf(stderr, "ProcessError: %s\n", uv_strerror(r));
  }
}

Process::~Process() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Process::pid() {
  return uv_process_get_pid(this);
}

int Process::kill(int signum) {
  return uv_process_kill(this, signum);
}

int Process::kill(int pid, int signum) {
  return uv_kill(pid, signum);
}

void Process::disable_stdio() {
  return uv_disable_stdio_inheritance();
}
