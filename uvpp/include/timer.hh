#pragma once

#include <iostream>

#include <uv.h>

#include "handle.hh"

struct Timer : public uv_timer_t, public Handle {
  Timer(uv_loop_t* loop);
  ~Timer();

  void start(uv_timer_cb, uint64_t, uint64_t repeat = 0);
  void stop();
  void again();
  void set_repeat(uint64_t);
  uint64_t get_repeat();
};
