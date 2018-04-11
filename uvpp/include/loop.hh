#pragma once

#include <uv.h>

#include "timer.hh"
#include "prepare.hh"
#include "check.hh"
#include "idle.hh"
#include "async.hh"
#include "poll.hh"
#include "signal.hh"
#include "process.hh"
#include "stream.hh"
#include "tcp.hh"

struct Timer;
struct Prepare;
struct Check;
struct Idle;
struct Async;
struct Poll;
struct Signal;
struct Process;
struct Stream;
struct TCP;

struct Loop : public uv_loop_t {
  Loop();
  ~Loop();

  static Loop* default_loop();
  void run(uv_run_mode mode = UV_RUN_DEFAULT);
  int alive();
  void stop();
  void close();

  uint64_t now();
  void update_time();

  void walk(uv_walk_cb, void* arg = nullptr);

  Timer* timer();
  Prepare* prepare();
  Check* check();
  Idle* idle();
  Async* async(uv_async_cb);
  Poll* poll(int);
  // Poll* poll(uv_os_sock_t);
  Signal* signal();
  Process* process(const uv_process_options_t*);
  TCP* tcp();
};
