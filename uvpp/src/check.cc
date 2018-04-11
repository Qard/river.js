#include "check.hh"

Check::Check(uv_loop_t* loop) {
  uv_check_init(loop, this);
}

Check::~Check() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Check::start(uv_check_cb callback) {
  return uv_check_start(this, callback);
}

int Check::stop() {
  return uv_check_stop(this);
}
