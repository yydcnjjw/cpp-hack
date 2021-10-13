#include <rx/observable.hpp>
#include <iostream>
// template <typename Executor, typename ValueType> struct TimerObserver {
//   using value_type = ValueType;
//   using executor_type = Executor;
//   using self_type = TimerObserver<Executor, value_type>;

//   TimerObserver(boost::asio::io_context &io_ctx)
//       : io_ctx_(io_ctx), timer_(io_ctx) {}
//   virtual ~TimerObserver() {}

//   void on_next(rx::async_context yield, value_type const &v) {
//     timer_.expires_after(std::chrono::milliseconds(500));
//     timer_.async_wait(
//         std::bind(&self_type::print, this, yield, v, std::placeholders::_1));
//   }

//   void print(rx::async_context yield, value_type v,
//              boost::system::error_code ec) {
//     std::cout << v << std::endl;
//     yield();
//   }

//   void on_completed() {
//     std::cout << "on_complete" << std::endl;
//     return promise_resolve(io_ctx_);
//   }

//   void on_error(boost::system::error_code const &ec) {
//     std::cout << "on_error: " << ec << std::endl;
//     return promise_resolve(io_ctx_);
//   }

//   executor_type &io_ctx_;
//   boost::asio::system_timer timer_;
// };

// struct Test {
//   Test(Executor &io_ctx)
//       : ob(std::make_shared<TimerObserver<Executor, long>>(io_ctx)) {}

//   void on_next(long v) { return ob->on_next(v); }

//   void on_error(boost::system::error_code const &ec) {
//     return ob->on_error(ec);
//   }

//   void on_completed() { return ob->on_completed(); }

//   std::shared_ptr<TimerObserver<Executor, long>> ob;
// };

int main(int argc, char *argv[]) {
  boost::asio::io_context io_ctx;

  rx::observable<void, void>::interval(io_ctx, std::chrono::milliseconds(500))
      .map<long>([&io_ctx](long const &v) {
        return v * v;
      })
      // .filter([&io_ctx](long const &v) {
      //   if (v % 2 == 0) {
      //     return promise_resolve(io_ctx, false);
      //   } else {
      //     return promise_resolve(io_ctx, true);
      //   }
      // })
      // .filter_map([&io_ctx](long const &v) {
      //   boost::optional<long> result;
      //   if (v % 2 == 0) {
      //     result = v * v;
      //   } else {
      //     result = boost::none;
      //   }
      //   return promise_resolve(io_ctx, result);
      // })
    .subscribe([](rx::async_context<void> yield, long v) {
      std::cout << v << std::endl;
    });

  io_ctx.run();
  return 0;
}
