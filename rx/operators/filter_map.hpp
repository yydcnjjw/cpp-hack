#pragma once

#include <boost/optional.hpp>
#include <core/future.hpp>
#include <rx/subscriber.hpp>
#include <rx/type.hpp>

namespace rx {

template <typename T> struct is_optional : std::false_type {};
template <typename... T>
struct is_optional<boost::optional<T...>> : std::true_type {};

template <typename InputType, typename Selector> struct filter_map_operator {
  using input_type = InputType;
  using select_type = Selector;
  using future_type =
      decltype((*(select_type *)nullptr)(*(input_type *)nullptr));
  using future_result_type = typename future_type::result_type;

  static_assert(is_optional<future_result_type>::value,
                "Output must be boost::optional");
  using output_type = typename future_result_type::value_type;

  using observer_type = observer<input_type, observer<output_type>>;

  select_type selector_;

  filter_map_operator(select_type s) : selector_(std::move(s)) {}

  template <typename Subscriber> struct filter_map_observer {
    using self_type = filter_map_observer<Subscriber>;
    using dest_type = Subscriber;
    using executor_type = typename dest_type::executor_type;

    executor_type &ex_;
    dest_type dest_;
    select_type selector_;
    filter_map_observer(executor_type &ex, dest_type dest, select_type selector)
        : ex_(ex), dest_(std::move(dest)), selector_(std::move(selector)) {}

    struct filter_map_op : public std::enable_shared_from_this<filter_map_op> {
      using parent_type = self_type;
      filter_map_op(parent_type *parent, Promise<executor_type, void> promise,
                    Future<future_result_type> fut)
          : parent_(parent), promise_(std::move(promise)), fut_(fut) {}

      void error(future_error ec) { promise_.error(ec); }

      void operator()(future_result_type v) {
        BOOST_ASIO_CORO_REENTER(coro_) {

          BOOST_ASIO_CORO_YIELD fut_
              .then(std::bind(&filter_map_op::operator(),
                              this->shared_from_this(), std::placeholders::_1))
              .error(std::bind(&filter_map_op::error, this->shared_from_this(),
                               std::placeholders::_1));

          if (v) {
            BOOST_ASIO_CORO_YIELD parent_->dest_.on_next(*v)
                .then(std::bind(&filter_map_op::operator(),
                                this->shared_from_this(), *v))
                .error(std::bind(&filter_map_op::error,
                                 this->shared_from_this(),
                                 std::placeholders::_1));
          }

          promise_.value();
        }
      }

      boost::asio::coroutine coro_;

      parent_type *parent_;
      Promise<executor_type, void> promise_;
      Future<future_result_type> fut_;
    };

    template <typename V> Future<void> on_next(V &&v) {

      Promise<executor_type, void> promise(ex_);

      (*std::make_shared<filter_map_op>(this, promise,
                                        selector_(std::forward<V>(v))))({});

      return promise.future();
    }

    Future<void> on_error(error_type e) { return dest_.on_error(e); }

    Future<void> on_completed() { return dest_.on_completed(); }

    static subscriber<executor_type, input_type, observer_type>
    make(dest_type dest, select_type s) {
      auto &ex = dest.get_executor();
      return subscriber<executor_type, input_type, observer_type>(
          ex, dest.get_subscription(),
          observer<output_type>(self_type(ex, std::move(dest), std::move(s))));
    }
  };

  template <typename Subscriber,
            typename Executor = typename Subscriber::executor_type>
  subscriber<Executor, input_type, observer_type> operator()(Subscriber dest) {
    return filter_map_observer<Subscriber>::make(std::move(dest), selector_);
  }
};

} // namespace rx
