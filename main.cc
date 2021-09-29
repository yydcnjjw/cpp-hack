#include <rx/observable.hpp>

template <typename Executor, typename ValueType> struct TimerObserver {
  using value_type = ValueType;
  using executor_type = Executor;
  using self_type = TimerObserver<Executor, value_type>;

  TimerObserver(boost::asio::io_context &io_ctx)
      : io_ctx_(io_ctx), timer_(io_ctx) {}
  virtual ~TimerObserver() {}

  Future<void> on_next(value_type const &v) {
    Promise<executor_type, void> promise(io_ctx_);

    timer_.expires_after(std::chrono::milliseconds(500));
    timer_.async_wait(
        std::bind(&self_type::print, this, promise, v, std::placeholders::_1));
    return promise.future();
  }

  void print(Promise<executor_type, void> promise, value_type v,
             boost::system::error_code ec) {
    std::cout << v << std::endl;
    promise.value();
  }

  Future<void> on_completed() {
    std::cout << "on_complete" << std::endl;
    return promise_resolve(io_ctx_);
  }

  Future<void> on_error(boost::system::error_code const &ec) {
    std::cout << "on_error: " << ec << std::endl;
    return promise_resolve(io_ctx_);
  }

  executor_type &io_ctx_;
  boost::asio::system_timer timer_;
};

template <typename Executor> struct Test {
  Test(Executor &io_ctx)
      : ob(std::make_shared<TimerObserver<Executor, long>>(io_ctx)) {}

  Future<void> on_next(long v) { return ob->on_next(v); }

  Future<void> on_error(boost::system::error_code const &ec) {
    return ob->on_error(ec);
  }

  Future<void> on_completed() { return ob->on_completed(); }

  std::shared_ptr<TimerObserver<Executor, long>> ob;
};

int main(int argc, char *argv[]) {
  boost::asio::io_context io_ctx;

  rx::observable<void, void>::interval(io_ctx, std::chrono::milliseconds(500))
      // .map([&io_ctx](long const &v) {
      //   if (v && v % 5 == 0) {
      //     return promise_error<long>(io_ctx,
      //                                boost::system::errc::make_error_code(
      //                                    boost::system::errc::broken_pipe));
      //   } else {
      //     return promise_resolve(io_ctx, v * v);
      //   }
      // })
      // .filter([&io_ctx](long const &v) {
      //   if (v % 2 == 0) {
      //     return promise_resolve(io_ctx, false);
      //   } else {
      //     return promise_resolve(io_ctx, true);
      //   }
      // })
      .filter_map([&io_ctx](long const &v) {
        boost::optional<long> result;
        if (v % 2 == 0) {
          result = v * v;
        } else {
          result = boost::none;
        }
        return promise_resolve(io_ctx, result);
      })
      .subscribe(io_ctx, Test<boost::asio::io_context>{io_ctx});

  io_ctx.run();
  return 0;
}
