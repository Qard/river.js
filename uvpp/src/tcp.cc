#include "tcp.hh"

TCP::TCP(uv_loop_t* loop, unsigned int flags) {
  if (flags != 0) {
    uv_tcp_init_ex(loop, this, flags);
  } else {
    uv_tcp_init(loop, this);
  }
}

TCP::~TCP() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int TCP::open(uv_os_sock_t socket) {
  return uv_tcp_open(this, socket);
}

int TCP::nodelay(int enable) {
  return uv_tcp_nodelay(this, enable);
}

int TCP::keepalive(int enable, unsigned int delay) {
  return uv_tcp_keepalive(this, enable, delay);
}

int TCP::simultaneous_accepts(int enable) {
  return uv_tcp_simultaneous_accepts(this, enable);
}

int TCP::bind(const struct sockaddr* socket, unsigned int flags) {
  return uv_tcp_bind(this, socket, flags);
}

int TCP::getsockname(struct sockaddr* address, int* namelen) {
  return uv_tcp_getsockname(this, address, namelen);
}

int TCP::getpeername(struct sockaddr* address, int* namelen) {
  return uv_tcp_getpeername(this, address, namelen);
}

int TCP::connect(uv_connect_t* request, const struct sockaddr* address, uv_connect_cb callback) {
  return uv_tcp_connect(request, this, address, callback);
}
