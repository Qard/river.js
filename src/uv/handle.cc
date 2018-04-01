#include "uv/handle.hh"

int Handle::is_active() {
  return uv_is_active(reinterpret_cast<uv_handle_t*>(this));
}

int Handle::is_closing() {
  return uv_is_closing(reinterpret_cast<uv_handle_t*>(this));
}

void Handle::close(uv_close_cb callback) {
  uv_close(reinterpret_cast<uv_handle_t*>(this), callback);
}

void Handle::ref() {
  uv_ref(reinterpret_cast<uv_handle_t*>(this));
}

void Handle::unref() {
  uv_unref(reinterpret_cast<uv_handle_t*>(this));
}

int Handle::has_ref() {
  return uv_has_ref(reinterpret_cast<uv_handle_t*>(this));
}

Loop* Handle::get_loop() {
  return reinterpret_cast<Loop*>(uv_handle_get_loop(reinterpret_cast<uv_handle_t*>(this)));
}

uv_handle_t* Handle::as_handle() {
  return reinterpret_cast<uv_handle_t*>(this);
}
