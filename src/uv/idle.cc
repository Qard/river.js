#include "uv/idle.hh"

Idle::Idle(uv_loop_t* loop) {
  uv_idle_init(loop, this);
}

Idle::~Idle() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Idle::start(uv_idle_cb callback) {
  return uv_idle_start(this, callback);
}

int Idle::stop() {
  return uv_idle_stop(this);
}
