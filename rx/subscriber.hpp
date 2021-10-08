#pragma once
#include <rx/observer.hpp>
#include <rx/subscription.hpp>
namespace rx {

template <typename Executor, typename T, typename Observer = observer<T>>
class subscriber {
  using observer_type = Observer;

public:
  using executor_type = Executor;

  subscriber(executor_type &ex, observer_type ob)
      : ex_(ex), ob_(std::move(ob)) {}
  subscriber(executor_type &ex, subscription subscription, observer_type ob)
      : ex_(ex), subscription_(std::move(subscription)), ob_(std::move(ob)) {}

  template <typename V> Future<void> on_next(V &&v) {
    if (is_subscribed()) {
      return ob_.on_next(std::forward<V>(v));
    } else {
      return promise_resolve(ex_);
    }
  }

  Future<void> on_error(error_type e) {
    if (is_subscribed()) {
      return ob_.on_error(e);
    } else {
      return promise_resolve(ex_);
    }
  }

  Future<void> on_completed() {
    if (is_subscribed()) {
      return ob_.on_completed();
    } else {
      return promise_resolve(ex_);
    }
  }

  bool is_subscribed() const { return subscription_.is_subscribed(); }

  subscription &get_subscription() { return subscription_; }

  executor_type &get_executor() { return ex_; }

private:
  executor_type &ex_;
  subscription subscription_;
  observer_type ob_;
};

template <typename T, typename Executor, typename Observer>
static inline subscriber<Executor, T, Observer> make_subscriber(Executor &&ex,
                                                                Observer &&ob) {
  return subscriber<Executor, T, Observer>(std::forward<Executor>(ex),
                                           std::forward<Observer>(ob));
}

template <typename T, typename Executor, typename Observer>
static inline subscriber<Executor, T, Observer>
make_subscriber(Executor &&ex, subscription s, Observer &&ob) {
  return subscriber<Executor, T, Observer>(
      std::forward<Executor>(ex), std::move(s), std::forward<Observer>(ob));
}

} // namespace rx
