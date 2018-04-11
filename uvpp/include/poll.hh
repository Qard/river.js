#pragma once

#include <uv.h>

#include "handle.hh"

struct Poll : public uv_poll_t, public Handle {
  Poll(uv_loop_t*, int);
  // Poll(uv_loop_t*, uv_os_sock_t);
  ~Poll();

  int start(int, uv_poll_cb);
  int stop();
};
