#pragma once

#include <core/future.hpp>
#include <rx/subscriber.hpp>
#include <rx/type.hpp>

namespace rx {

template <typename T, typename Selector> struct map_operator {
  using source_value_type = T;
  using select_type = Selector;
  using future_type =
      decltype((*(select_type *)nullptr)(*(source_value_type *)nullptr));
  using output_type = typename future_type::result_type;

  select_type selector_;

  map_operator(select_type s) : selector_(std::move(s)) {}

  template <typename Subscriber> struct map_observer {
    using self_type = map_observer<Subscriber>;
    using dest_type = Subscriber;
    using executor_type = typename dest_type::executor_type;

    executor_type &ex_;
    dest_type dest_;
    select_type selector_;
    map_observer(executor_type &ex, dest_type dest, select_type selector)
        : ex_(ex), dest_(std::move(dest)), selector_(std::move(selector)) {}

    struct map_op : public std::enable_shared_from_this<map_op> {
      using parent_type = self_type;
      map_op(parent_type *parent, Promise<executor_type, void> promise,
             Future<output_type> fut)
          : parent_(parent), promise_(std::move(promise)), fut_(fut) {
      }

      void error(future_error ec) { promise_.error(ec); }

      void operator()(output_type v) {
        BOOST_ASIO_CORO_REENTER(coro_) {

          BOOST_ASIO_CORO_YIELD fut_
              .then(std::bind(&map_op::operator(), this->shared_from_this(),
                              std::placeholders::_1))
              .error(std::bind(&map_op::error, this->shared_from_this(),
                               std::placeholders::_1));

          BOOST_ASIO_CORO_YIELD parent_->dest_.on_next(v)
              .then(std::bind(&map_op::operator(), this->shared_from_this(), v))
              .error(std::bind(&map_op::error, this->shared_from_this(),
                               std::placeholders::_1));

          promise_.value();
        }
      }

      boost::asio::coroutine coro_;

      parent_type *parent_;
      Promise<executor_type, void> promise_;
      Future<output_type> fut_;
    };

    template <typename V> Future<void> on_next(V &&v) {

      Promise<executor_type, void> promise(ex_);

      (*std::make_shared<map_op>(this, promise, selector_(v)))({});

      return promise.future();
    }

    Future<void> on_error(error_type e) { return dest_.on_error(e); }

    Future<void> on_completed() { return dest_.on_completed(); }

    static subscriber<executor_type, source_value_type,
                      observer<T, observer<output_type>>>
    make(dest_type dest, select_type s) {
      auto subscription = dest.get_subscription();
      auto &ex = dest.get_executor();
      return subscriber<executor_type, source_value_type,
                        observer<T, observer<output_type>>>(
          ex, std::move(subscription),
          observer<output_type>(self_type(ex, std::move(dest), std::move(s))));
    }
  };

  template <typename Subscriber,
            typename Executor = typename Subscriber::executor_type>
  subscriber<Executor, source_value_type, observer<T, observer<output_type>>>
  operator()(Subscriber dest) {
    return map_observer<Subscriber>::make(dest, selector_);
  }
};

} // namespace rx
