#pragma once

#include <uv.h>

#include "uv/handle.hh"

struct Signal : public uv_signal_t, public Handle {
  Signal(uv_loop_t*);
  ~Signal();

  int start(uv_signal_cb, int);
  int once(uv_signal_cb, int);
  int stop();
};
