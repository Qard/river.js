#pragma once

#include <uv.h>

#include "uv/handle.hh"

struct Prepare : public uv_prepare_t, public Handle {
  Prepare(uv_loop_t* loop);
  ~Prepare();

  int start(uv_prepare_cb cb);
  int stop();
};
