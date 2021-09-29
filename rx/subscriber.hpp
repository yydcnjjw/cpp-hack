#pragma once
#include <rx/observer.hpp>
#include <rx/subscription.hpp>
namespace rx {

template <typename Executor, typename T, typename Observer = observer<T>>
class subscriber {
  using observer_type = Observer;

public:
  using executor_type = Executor;

  subscriber(executor_type &ex, observer_type ob) : ex_(ex), ob_(ob) {}
  subscriber(executor_type &ex, subscription subscription, observer_type ob)
      : ex_(ex), subscription_(std::move(subscription)), ob_(ob) {}

  Future<void> on_next(T const &t) {
    if (is_subscribed()) {
      return ob_.on_next(t);
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

} // namespace rx
