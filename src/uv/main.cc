#include <vector>
#include "uv/loop.hh"

int main(int argc, char** argv) {
  auto loop = static_cast<Loop*>(uv_default_loop());
  // auto loop = new Loop();

  //
  // Idles run once per loop iteration, before prepares
  //
  auto idle = loop->idle();
  idle->start([](uv_idle_t* idle) {
    static_cast<Idle*>(idle)->stop();
    std::cout << "idle" << std::endl;
  });

  //
  // Prepares run once per loop iteration, before I/O
  //
  auto prepare = loop->prepare();
  prepare->start([](uv_prepare_t* prepare) {
    static_cast<Prepare*>(prepare)->stop();
    std::cout << "prepare" << std::endl;
  });

  //
  // Checks run once per loop iteration, after I/O
  //
  auto check = loop->check();
  check->start([](uv_check_t* check) {
    static_cast<Check*>(check)->stop();
    std::cout << "check" << std::endl;
  });

  //
  // Asyncs run eventually, after calling the `send()` method
  //
  auto async = loop->async([](uv_async_t* async) {
    static_cast<Async*>(async)->close([](uv_handle_t* handle){
      std::cout << "closed async" << std::endl;
    });
    std::cout << "async" << std::endl;
  });

  //
  // Poll waits for file descriptor availability
  //
  auto poll = loop->poll(0);
  poll->start(UV_READABLE | UV_WRITABLE | UV_DISCONNECT, [](uv_poll_t* poll, int status, int events) {
    std::cout << "poll" << std::endl;
    static_cast<Poll*>(poll)->stop();
  });

  //
  // Timer runs after a given timeout, and can be repeatable
  //
  auto timer = loop->timer();
  timer->data = static_cast<void*>(async);
  timer->start([](uv_timer_t* timer) {
    static_cast<Async*>(timer->data)->send();
    std::cout << "timer" << std::endl;
  }, 1000);

  //
  // Signal itercepts OS signals
  //
  auto signal = loop->signal();
  signal->start([](uv_signal_t* signal, int signum) {
    std::cout << "signal: " << signum << std::endl;
    exit(0);
  }, SIGINT);

  //
  // Child process
  //
  auto options = new ProcessOptions({ "/bin/echo", "child process" });

  std::vector<uv_stdio_container_t> list(3);
  // stdin
  list[0].flags = UV_IGNORE;
  // stdout
  list[1].flags = UV_INHERIT_FD;
  list[1].data.fd = 1;
  // stderr
  list[2].flags = UV_INHERIT_FD;
  list[2].data.fd = 2;

  options->stdio_count = list.size();
  options->stdio = list.data();

  auto process = loop->process(options);
  delete options;

  //
  // TCP echo server
  //
  auto tcp_server = loop->tcp();
  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", 3000, &addr);
  tcp_server->bind(reinterpret_cast<const struct sockaddr*>(&addr));
  tcp_server->listen(128, [](uv_stream_t* stream, int status) {
    auto server = reinterpret_cast<TCP*>(stream);
    auto client = static_cast<Loop*>(server->loop)->tcp();

    if (server->accept(client->as_stream()) == 0) {
      typedef struct {
        uv_write_t req;
        uv_buf_t buf;
      } write_req_t;

      client->read_start([](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = (char*) malloc(suggested_size);
        buf->len = suggested_size;
      }, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        auto client = reinterpret_cast<TCP*>(stream);
        if (nread > 0) {
          write_req_t* req = new write_req_t();
          req->buf = uv_buf_init(buf->base, nread);
          client->write((uv_write_t*) req, &req->buf, 1, [](uv_write_t* req, int status) {
            if (status) fprintf(stderr, "Write error %s\n", uv_strerror(status));
            write_req_t* wr = (write_req_t*) req;
            free(wr->buf.base);
            free(wr);
          });
          return;
        }

        if (nread < 0) {
          if (nread != UV_EOF) fprintf(stderr, "Read error %s\n", uv_err_name(nread));
          client->close([](uv_handle_t* handle){});
        }

        free(buf->base);
      });
    } else {
      client->close([](uv_handle_t* handle){});
    }
  });

  //
  // Run loop
  //
  std::cout << "Now quitting." << std::endl;
  loop->run();
  loop->close();

  // Cleanup
  delete idle;
  delete prepare;
  delete check;
  delete async;
  delete poll;
  delete timer;
  delete signal;
  delete process;
  delete tcp_server;
  delete loop;

  return 0;
};
