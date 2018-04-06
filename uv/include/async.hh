#pragma once

#include <uv.h>

#include "uv/handle.hh"

struct Async : public uv_async_t, public Handle {
  Async(uv_loop_t*, uv_async_cb);
  ~Async();

  int send();
};
