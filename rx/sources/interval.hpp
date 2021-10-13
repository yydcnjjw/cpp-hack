#include <boost/asio.hpp>
#include <rx/async_context.hpp>
#include <rx/source.hpp>
#include <rx/type.hpp>

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
    async_context<void> bind_context(Subscriber &&o) {
      return make_async_context<void>(std::bind(&inner::operator()<Subscriber>,
                                                shared_from_this(), o,
                                                std::placeholders::_1));
    }

    template <typename Subscriber>
    void on_error(Subscriber &&o, rx::error_type e) {
      o.on_error(bind_context(std::forward<Subscriber>(o)), e);
    }

    template <typename Subscriber> void on_next(Subscriber &&o, value_type v) {
      o.on_next(bind_context(std::forward<Subscriber>(o)), v);
    }

    template <typename Subscriber>
    void operator()(Subscriber &&o, boost::system::error_code const &ec) {
      BOOST_ASIO_CORO_REENTER(coro_) {
        for (;;) {
          if (!o.is_subscribed()) {
            break;
          }

          if (ec) {
            BOOST_ASIO_CORO_YIELD on_error(std::forward<Subscriber>(o), ec);
            break;
          }

          timer_.expires_after(period_);

          BOOST_ASIO_CORO_YIELD timer_.async_wait(
              std::bind(&inner::operator()<Subscriber>, shared_from_this(),
                        std::move(o), std::placeholders::_1));

          BOOST_ASIO_CORO_YIELD on_next(std::forward<Subscriber>(o),
                                        counter_++);
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

  template <typename Subscriber> void on_subscribe(Subscriber &&o) {
    // TODO: verify Subscriber
    (*inner_)(std::forward<Subscriber>(o), {});
  }

private:
  inner::ptr inner_;
};

} // namespace source

} // namespace rx
