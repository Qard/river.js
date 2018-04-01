#include "uv/async.hh"

Async::Async(uv_loop_t* loop, uv_async_cb callback) {
  uv_async_init(loop, this, callback);
}

Async::~Async() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Async::send() {
  return uv_async_send(this);
}
