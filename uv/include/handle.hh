#pragma once

#include <uv.h>

struct Loop;

struct Handle {
  int is_active();
  int is_closing();
  void close(uv_close_cb);
  void ref();
  void unref();
  int has_ref();
  // TODO: Probably don't actually need this...
  Loop* get_loop();
  uv_handle_t* as_handle();
};
