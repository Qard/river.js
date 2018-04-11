#include "poll.hh"

Poll::Poll(uv_loop_t* loop, int fd) {
  uv_poll_init(loop, this, fd);
}

// Poll::Poll(uv_loop_t* loop, uv_os_sock_t socket) {
//   uv_poll_init_socket(loop, this, socket);
// }

Poll::~Poll() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Poll::start(int events, uv_poll_cb callback) {
  return uv_poll_start(this, events, callback);
}

int Poll::stop() {
  return uv_poll_stop(this);
}
