#include "signal.hh"

Signal::Signal(uv_loop_t* loop) {
  uv_signal_init(loop, this);
}

Signal::~Signal() {
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

int Signal::start(uv_signal_cb callback, int signum) {
  return uv_signal_start(this, callback, signum);
}

int Signal::once(uv_signal_cb callback, int signum) {
  return uv_signal_start_oneshot(this, callback, signum);
}

int Signal::stop() {
  return uv_signal_stop(this);
}
