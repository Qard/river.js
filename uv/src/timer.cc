#include "uv/timer.hh"

Timer::Timer(uv_loop_t* loop) {
  uv_timer_init(loop, this);
}

Timer::~Timer() {
	this->stop();
  if (!this->is_closing()) {
    this->close([](uv_handle_t* handle){});
  }
}

void Timer::start(uv_timer_cb callback, uint64_t timeout, uint64_t repeat) {
  uv_timer_start(this, callback, timeout, repeat);
}

void Timer::stop() {
  uv_timer_stop(this);
}

void Timer::again() {
  uv_timer_again(this);
}

void Timer::set_repeat(uint64_t repeat) {
  uv_timer_set_repeat(this, repeat);
}

uint64_t Timer::get_repeat() {
  return uv_timer_get_repeat(this);
}
