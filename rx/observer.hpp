#pragma once

#include <core/future.hpp>
#include <rx/type.hpp>

namespace rx {

struct tag_observer {};

template <class T> struct observer_base {
  using value_type = T;
  using observer_tag = tag_observer;
};

// template <typename T, typename OnNext, typename OnError, typename
// OnCompleted> class observer : public observer_base<T> {

//   Future<void> on_next(T &t) {}
//   Future<void> on_error(error_type) {}
//   Future<void> on_completed() {}
// };

template <typename T, typename ForwardObserver = void>
class observer : observer_base<T> {
  using forward_type = ForwardObserver;

public:
  observer(forward_type forward) : forward_(std::move(forward)) {}

  Future<void> on_next(T const &t) { return forward_.on_next(t); }
  Future<void> on_error(error_type e) { return forward_.on_error(e); }
  Future<void> on_completed() { return forward_.on_completed(); }

private:
  forward_type forward_;
};

namespace detail {

template <typename T> struct virtual_observer {
  virtual ~virtual_observer() {}
  virtual Future<void> on_next(T const &t) = 0;
  virtual Future<void> on_error(error_type) = 0;
  virtual Future<void> on_completed() = 0;
};

template <typename T, typename Observer>
struct specific_observer : public virtual_observer<T> {

  explicit specific_observer(Observer ob) : ob_(std::move(ob)) {}

  Future<void> on_next(T const &t) override { return ob_.on_next(t); }
  Future<void> on_error(error_type e) override { return ob_.on_error(e); }
  Future<void> on_completed() override { return ob_.on_completed(); }

  Observer ob_;
};

} // namespace detail

template <typename T> class observer<T, void> : public observer_base<T> {

  using virtual_observer = detail::virtual_observer<T>;

public:
  template <typename Observer>
  explicit observer(Observer ob)
      : ob_(std::make_shared<detail::specific_observer<T, Observer>>(
            std::move(ob))) {}

  template <typename V> Future<void> on_next(V &&v) {
    return ob_->on_next(std::forward<V>(v));
  }

  Future<void> on_error(error_type e) {
    return ob_->on_error(e);
  }

  Future<void> on_completed() {
    return ob_->on_completed();
  }

private:
  std::shared_ptr<virtual_observer> ob_;
};

} // namespace rx
