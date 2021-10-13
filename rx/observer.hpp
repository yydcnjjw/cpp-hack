#pragma once

#include <rx/async_context.hpp>
#include <rx/type.hpp>

namespace rx {

template <typename T, typename Tag, typename ForwardObserver = void,
          typename OnNext = void, typename OnError = void,
          typename OnCompleted = void>
struct observer;

namespace detail {
struct normal_observer_tag;
struct forward_observer_tag;
struct dynamic_observer_tag;

} // namespace detail

template <typename T, typename OnNext, typename OnError, typename OnCompleted>
class observer<T, detail::normal_observer_tag, void, OnNext, OnError,
               OnCompleted>;

template <typename T, typename ForwardObserver>
class observer<T, detail::forward_observer_tag, ForwardObserver>;

template <typename T> class observer<T, detail::dynamic_observer_tag>;

template <typename T> struct is_observer : std::false_type {};
template <typename... T> struct is_observer<observer<T...>> : std::true_type {};

template <class T> struct observer_base { using value_type = T; };

namespace detail {

struct EmptyCallback {
  template <typename... Args> void operator()(Args &&...args) const {}
};

template <typename Callback>
struct callback_or_empty : std::conditional<std::is_same<Callback, void>::value,
                                            detail::EmptyCallback, Callback> {};

} // namespace detail

template <typename T, typename OnNext, typename OnError, typename OnCompleted>
class observer<T, detail::normal_observer_tag, void, OnNext, OnError,
               OnCompleted> : observer_base<T> {

  using on_next_t = typename detail::callback_or_empty<OnNext>::type;
  using on_error_t = typename detail::callback_or_empty<OnError>::type;
  using on_completed_t = typename detail::callback_or_empty<OnCompleted>::type;

public:
  observer(on_next_t next = on_next_t(), on_error_t error = on_error_t(),
           on_completed_t completed = on_completed_t())
      : next_(std::move(next)), error_(std::move(error)),
        completed_(std::move(completed)) {}

  template <typename... Args> void on_next(Args &&...args) {
    return next_(std::forward<Args>(args)...);
  }

  template <typename... Args> void on_error(Args &&...args) {
    return error_(std::forward<Args>(args)...);
  }

  template <typename... Args> void on_completed(Args &&...args) {
    return completed_(std::forward<Args>(args)...);
  }

private:
  on_next_t next_;
  on_error_t error_;
  on_completed_t completed_;
};

template <typename T, typename ForwardObserver>
struct observer<T, detail::forward_observer_tag, ForwardObserver>
    : observer_base<T> {
  static_assert(!std::is_same<ForwardObserver, void>::value, "");
  using forward_type = ForwardObserver;

public:
  observer(forward_type forward) : forward_(std::move(forward)) {}

  template <typename... Args> void on_next(Args &&...args) {
    return forward_.on_next(std::forward<Args>(args)...);
  }

  template <typename... Args> void on_error(Args &&...args) {
    return forward_.on_error(std::forward<Args>(args)...);
  }

  template <typename... Args> void on_completed(Args &&...args) {
    return forward_.on_completed(std::forward<Args>(args)...);
  }

private:
  forward_type forward_;
};

namespace detail {

using observer_async_context = async_context<void>;

template <typename T> struct virtual_observer {

  virtual ~virtual_observer() {}

  virtual void on_next(T const &t) = 0;
  virtual void on_next(T &&t) = 0;
  virtual void on_next(observer_async_context, T const &) = 0;
  virtual void on_next(observer_async_context, T &&) = 0;

  virtual void on_error(error_type) = 0;
  virtual void on_error(observer_async_context, error_type) = 0;

  virtual void on_completed() = 0;
  virtual void on_completed(observer_async_context) = 0;
};

template <typename T, typename Observer>
struct specific_observer : public virtual_observer<T> {

  explicit specific_observer(Observer ob) : ob_(std::move(ob)) {}

  void on_next(T const &t) { return ob_.on_next(t); }
  void on_next(T &&t) { return ob_.on_next(std::move(t)); }
  void on_next(observer_async_context ctx, T const &t) {
    return ob_.on_next(ctx, t);
  }
  void on_next(observer_async_context ctx, T &&t) {
    return ob_.on_next(ctx, std::move(t));
  }

  void on_error(error_type e) { return ob_.on_error(std::move(e)); }
  void on_error(observer_async_context ctx, error_type e) {
    return ob_.on_error(ctx, std::move(e));
  }

  void on_completed() { return ob_.on_completed(); }
  void on_completed(observer_async_context ctx) {
    return ob_.on_completed(ctx);
  }

  Observer ob_;
};

} // namespace detail

template <typename T>
class observer<T, detail::dynamic_observer_tag> : public observer_base<T> {
  using virtual_observer = detail::virtual_observer<T>;

public:
  template <typename Observer>
  explicit observer(Observer &&ob)
      : ob_(std::make_shared<detail::specific_observer<T, Observer>>(
            std::forward<Observer>(ob))) {}

  template <typename... Args> void on_next(Args &&...args) {
    return ob_->on_next(std::forward<Args>(args)...);
  }

  template <typename... Args> void on_error(Args &&...args) {
    return ob_->on_error(std::forward<Args>(args)...);
  }

  template <typename... Args> void on_completed(Args &&...args) {
    return ob_->on_completed(std::forward<Args>(args)...);
  }

private:
  std::shared_ptr<virtual_observer> ob_;
};

template <
    typename T, typename... Args,
    typename Observer = observer<T, detail::normal_observer_tag, void, Args...>>
Observer make_observer(Args &&...args) {
  return Observer(std::forward<Args>(args)...);
}

template <typename T, typename ForwardObserver,
          typename Observer =
              observer<T, detail::forward_observer_tag, ForwardObserver>,
          typename std::enable_if<is_observer<ForwardObserver>::value,
                                  bool>::type = true>
Observer make_observer(ForwardObserver &&ob) {
  return Observer(std::forward<ForwardObserver>(ob));
}

template <typename T, typename DynamicObserver,
          typename Observer = observer<T, detail::dynamic_observer_tag>>
Observer make_dynamic_observer(DynamicObserver &&ob) {
  return Observer(std::forward<DynamicObserver>(ob));
}

} // namespace rx
