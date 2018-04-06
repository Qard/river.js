#pragma once

#include <uv.h>

#include "uv/handle.hh"
#include "uv/stream.hh"

struct TCP : public uv_tcp_t, public Stream {
  TCP(uv_loop_t*, unsigned int flags = 0);
  ~TCP();

  int open(uv_os_sock_t);
  int nodelay(int);
  int keepalive(int, unsigned int);
  int simultaneous_accepts(int);
  int bind(const struct sockaddr*, unsigned int flags = 0);
  int getsockname(struct sockaddr*, int*);
  int getpeername(struct sockaddr*, int*);
  int connect(uv_connect_t*, const struct sockaddr*, uv_connect_cb);
};
