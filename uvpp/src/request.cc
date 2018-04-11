#include "request.hh"

int Request::cancel() {
  return uv_cancel(reinterpret_cast<uv_req_t*>(this));
}
