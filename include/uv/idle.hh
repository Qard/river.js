#pragma once

#include <uv.h>

#include "uv/handle.hh"

struct Idle : public uv_idle_t, public Handle {
  Idle(uv_loop_t* loop);
  ~Idle();

  int start(uv_idle_cb cb);
  int stop();
};
