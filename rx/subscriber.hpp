#pragma once
#include <rx/observer.hpp>
#include <rx/subscription.hpp>
namespace rx {

template <typename T, typename Observer = observer<T, void>> class subscriber {
  using observer_type = Observer;
  static_assert(is_observer<Observer>::value, "");

public:
  // subscriber(observer_type ob) : ob_(std::move(ob)) {}

  subscriber(subscription subscription, observer_type ob)
      : subscription_(std::move(subscription)), ob_(std::move(ob)) {}

  template <typename... Args> void on_next(Args &&...args) {
    if (is_subscribed()) {
      return ob_.on_next(std::forward<Args>(args)...);
    }
  }

  template <typename... Args> void on_error(Args &&...args) {
    if (is_subscribed()) {
      return ob_.on_error(std::forward<Args>(args)...);
    }
  }

  template <typename... Args> void on_completed(Args &&...args) {
    if (is_subscribed()) {
      return ob_.on_completed(std::forward<Args>(args)...);
    }
  }

  bool is_subscribed() const { return subscription_.is_subscribed(); }

  subscription &get_subscription() { return subscription_; }

private:
  subscription subscription_;
  observer_type ob_;
};

// template <typename T, typename Observer>
// static inline subscriber<T, Observer> make_subscriber(Observer &&ob) {
//   return subscriber<T, Observer>(std::forward<Observer>(ob));
// }

template <typename T, typename... Args,
          typename Observer = decltype(make_observer<T>(
              std::forward<Args>(std::declval<Args>())...)),
          typename Subscriber = subscriber<T, Observer>>
static inline Subscriber make_subscriber(subscription s, Args &&...args) {
  return Subscriber(std::move(s),
                    make_observer<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args,
          typename Observer = decltype(make_observer<T>(
              std::forward<Args>(std::declval<Args>())...)),
          typename Subscriber = subscriber<T, Observer>>
static inline Subscriber make_subscriber(Args &&...args) {
  return Subscriber(subscription(),
                    make_observer<T>(std::forward<Args>(args)...));
}

} // namespace rx
