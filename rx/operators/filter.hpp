#pragma once

#include <core/future.hpp>
#include <rx/subscriber.hpp>
#include <rx/type.hpp>

namespace rx {

template <typename T, typename Predicate> struct filter_operator {
  using source_value_type = T;
  using predicate_type = Predicate;

  predicate_type pred_;

  filter_operator(predicate_type s) : pred_(std::move(s)) {}

  template <typename Subscriber> struct filter_observer {
    using self_type = filter_observer<Subscriber>;
    using dest_type = Subscriber;
    using executor_type = typename dest_type::executor_type;

    executor_type &ex_;
    dest_type dest_;
    predicate_type pred_;
    filter_observer(executor_type &ex, dest_type dest, predicate_type selector)
        : ex_(ex), dest_(std::move(dest)), pred_(std::move(selector)) {}

    template <typename ValueType>
    struct filter_op
        : public std::enable_shared_from_this<filter_op<ValueType>> {
      using parent_type = self_type;
      using source_value_type = typename std::decay<ValueType>::type;
      filter_op(parent_type *parent, Promise<executor_type, void> promise,
                ValueType v)
          : parent_(parent), promise_(std::move(promise)), v_(v) {}

      void error(future_error ec) { promise_.error(ec); }

      void operator()(bool test) {
        BOOST_ASIO_CORO_REENTER(coro_) {

          BOOST_ASIO_CORO_YIELD parent_->pred_(v_)
              .then(std::bind(&filter_op::operator(), this->shared_from_this(),
                              std::placeholders::_1))
              .error(std::bind(&filter_op::error, this->shared_from_this(),
                               std::placeholders::_1));

          if (!test) {
            BOOST_ASIO_CORO_YIELD parent_->dest_.on_next(v_)
                .then(std::bind(&filter_op::operator(),
                                this->shared_from_this(), !test))
                .error(std::bind(&filter_op::error, this->shared_from_this(),
                                 std::placeholders::_1));
          }

          promise_.value();
        }
      }

      boost::asio::coroutine coro_;

      source_value_type v_;
      parent_type *parent_;
      Promise<executor_type, void> promise_;
    };

    template <typename V> Future<void> on_next(V &&v) {

      Promise<executor_type, void> promise(ex_);

      (*std::make_shared<filter_op<V>>(this, promise, std::forward<V>(v)))({});

      return promise.future();
    }

    Future<void> on_error(error_type e) { return dest_.on_error(e); }

    Future<void> on_completed() { return dest_.on_completed(); }

    static subscriber<executor_type, source_value_type,
                      observer<T, observer<source_value_type>>>
    make(dest_type dest, predicate_type s) {
      auto subscription = dest.get_subscription();
      auto &ex = dest.get_executor();
      return subscriber<executor_type, source_value_type,
                        observer<T, observer<source_value_type>>>(
          ex, std::move(subscription),
          observer<source_value_type>(
              self_type(ex, std::move(dest), std::move(s))));
    }
  };

  template <typename Subscriber,
            typename Executor = typename Subscriber::executor_type>
  subscriber<Executor, source_value_type,
             observer<T, observer<source_value_type>>>
  operator()(Subscriber dest) {
    return filter_observer<Subscriber>::make(dest, pred_);
  }
};

} // namespace rx
