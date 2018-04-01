#pragma once

#include <uv.h>

#include "uv/handle.hh"

struct Check : public uv_check_t, public Handle {
  Check(uv_loop_t* loop);
  ~Check();

  int start(uv_check_cb cb);
  int stop();
};
