#pragma once

#include <initializer_list>

#include <uv.h>

#include "uv/handle.hh"

struct ProcessOptions : public uv_process_options_t {
  ProcessOptions(std::initializer_list<const char*>);
};

struct Process : public uv_process_t, public Handle {
  Process(uv_loop_t*, const uv_process_options_t*);
  ~Process();

  int pid();
  int kill(int signum);

  static int kill(int pid, int signum);
  static void disable_stdio();
};
