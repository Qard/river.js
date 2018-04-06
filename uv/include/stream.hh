#pragma once

#include <uv.h>

#include "uv/handle.hh"

struct Stream : public Handle {
  int shutdown(uv_shutdown_t*, uv_shutdown_cb);
  int listen(int, uv_connection_cb);
  int accept(uv_stream_t*);
  int read_start(uv_alloc_cb, uv_read_cb);
  int read_stop();
  int write(uv_write_t*, const uv_buf_t[], unsigned int, uv_write_cb);
  int write2(uv_write_t*, const uv_buf_t[], unsigned int, uv_stream_t*, uv_write_cb);
  int try_write(const uv_buf_t[], unsigned int);
  int is_readable();
  int is_writable();
  int set_blocking(int blocking);
  size_t get_write_queue_size();
  uv_stream_t* as_stream();
};
