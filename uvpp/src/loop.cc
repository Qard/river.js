#include "loop.hh"

Loop::Loop() {
  uv_loop_init(this);
}

Loop::~Loop() {
  this->stop();
  this->close();
}

Loop* Loop::default_loop() {
  return static_cast<Loop*>(uv_default_loop());
}

void Loop::run(uv_run_mode mode) {
  uv_run(this, mode);
}

int Loop::alive() {
  return uv_loop_alive(this);
}

void Loop::stop() {
  uv_stop(this);
}

void Loop::close() {
  uv_loop_close(this);
}

uint64_t Loop::now() {
  return uv_now(this);
}

void Loop::update_time() {
  uv_update_time(this);
}

// TODO:
// - Figure out how to make capturing closures work
// - Abstract Handle type?
void Loop::walk(uv_walk_cb fn, void* arg) {
  uv_walk(this, fn, arg);
}

//
// Factories
//
Timer* Loop::timer() {
  return new Timer(this);
}
Prepare* Loop::prepare() {
  return new Prepare(this);
}
Check* Loop::check() {
  return new Check(this);
}
Idle* Loop::idle() {
  return new Idle(this);
}
Async* Loop::async(uv_async_cb callback) {
  return new Async(this, callback);
}
Poll* Loop::poll(int fd) {
  return new Poll(this, fd);
}
// Poll* Loop::poll(uv_os_sock_t socket) {
//   return new Poll(this, socket);
// }
Signal* Loop::signal() {
  return new Signal(this);
}
Process* Loop::process(const uv_process_options_t* options) {
  return new Process(this, options);
}
TCP* Loop::tcp() {
  return new TCP(this);
}
