#include <boost/asio.hpp>
#include <rx/source.hpp>

#include <core/future.hpp>

namespace rx {

namespace source {

class interval : public source_base<long> {
  using timer_type = boost::asio::system_timer;
  using duration_type = timer_type::duration;

  struct inner : std::enable_shared_from_this<inner> {
    using ptr = std::shared_ptr<inner>;
    using coroutine_type = boost::asio::coroutine;

    template <typename Executor>
    inner(Executor &ex, duration_type const &p)
        : timer_(ex), period_(p), counter_(0) {}

    template <typename Subscriber>
    void bind_this(Subscriber o, Future<void> fut) {
      fut.then(std::bind(&inner::operator()<Subscriber>, shared_from_this(), o,
                         boost::system::error_code()))
          .error(std::bind(&inner::operator()<Subscriber>, shared_from_this(),
                           o, std::placeholders::_1));
    }

    template <typename Subscriber>
    void operator()(Subscriber o, future_error const &ec) {
      BOOST_ASIO_CORO_REENTER(coro_) {
        for (;;) {
          if (!o.is_subscribed()) {
            break;
          }

          if (ec) {
            BOOST_ASIO_CORO_YIELD bind_this(o, o.on_error(ec));
            break;
          }

          timer_.expires_after(period_);

          BOOST_ASIO_CORO_YIELD timer_.async_wait(
              std::bind(&inner::operator()<Subscriber>, shared_from_this(), o,
                        std::placeholders::_1));

          BOOST_ASIO_CORO_YIELD bind_this(o, o.on_next(counter_++));
        }
      }
    }

    coroutine_type coro_;
    timer_type timer_;
    duration_type period_;
    long counter_;
  };

public:
  template <typename Executor>
  interval(Executor &ex, duration_type const &p)
      : inner_(std::make_shared<inner>(ex, p)) {}

  template <typename Subscriber> void on_subscribe(Subscriber o) {
    // TODO: verify Subscriber
    (*inner_)(o, {});
  }

private:
  inner::ptr inner_;
};

} // namespace source

} // namespace rx
