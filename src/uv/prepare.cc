#include "uv/prepare.hh"

Prepare::Prepare(uv_loop_t* loop) {
  uv_prepare_init(loop, this);
}

Prepare::~Prepare() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Prepare::start(uv_prepare_cb callback) {
  return uv_prepare_start(this, callback);
}

int Prepare::stop() {
  return uv_prepare_stop(this);
}
