#include "stream.hh"

int Stream::shutdown(uv_shutdown_t* request, uv_shutdown_cb callback) {
  return uv_shutdown(request, reinterpret_cast<uv_stream_t*>(this), callback);
}

int Stream::listen(int backlog, uv_connection_cb callback) {
  return uv_listen(reinterpret_cast<uv_stream_t*>(this), backlog, callback);
}

// TODO: Should "this" be server or client here?
int Stream::accept(uv_stream_t* client) {
  return uv_accept(reinterpret_cast<uv_stream_t*>(this), client);
}

int Stream::read_start(uv_alloc_cb alloc_cb, uv_read_cb read_cb) {
  return uv_read_start(reinterpret_cast<uv_stream_t*>(this), alloc_cb, read_cb);
}

int Stream::read_stop() {
  return uv_read_stop(reinterpret_cast<uv_stream_t*>(this));
}

int Stream::write(uv_write_t* request, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb callback) {
  return uv_write(request, reinterpret_cast<uv_stream_t*>(this), bufs, nbufs, callback);
}

int Stream::write2(uv_write_t* request, const uv_buf_t bufs[], unsigned int nbufs, uv_stream_t* send_handle, uv_write_cb callback) {
  return uv_write2(request, reinterpret_cast<uv_stream_t*>(this), bufs, nbufs, send_handle, callback);
}

int Stream::try_write(const uv_buf_t bufs[], unsigned int nbufs) {
  return uv_try_write(reinterpret_cast<uv_stream_t*>(this), bufs, nbufs);
}

int Stream::is_readable() {
  return uv_is_readable(reinterpret_cast<const uv_stream_t*>(this));
}

int Stream::is_writable() {
  return uv_is_writable(reinterpret_cast<const uv_stream_t*>(this));
}

int Stream::set_blocking(int blocking) {
  return uv_stream_set_blocking(reinterpret_cast<uv_stream_t*>(this), blocking);
}

size_t Stream::get_write_queue_size() {
  return uv_stream_get_write_queue_size(reinterpret_cast<const uv_stream_t*>(this));
}

uv_stream_t* Stream::as_stream() {
  return reinterpret_cast<uv_stream_t*>(this);
}
