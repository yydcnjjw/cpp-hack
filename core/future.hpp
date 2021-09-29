#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <type_traits>

using future_error = boost::system::error_code;

template <typename Result> struct Future;

template <typename Result>
struct FutureImpl : public std::enable_shared_from_this<FutureImpl<Result>> {
  using result_type = Result;
  using self_type = FutureImpl<Result>;
  using then_fn = std::function<void(result_type)>;
  using error_fn = std::function<void(future_error)>;

  self_type &then(then_fn fn) {
    then_fn_ = std::move(fn);
    return *this;
  }

  self_type &error(error_fn fn) {
    error_fn_ = std::move(fn);
    return *this;
  }

  void notifiy_then(result_type const &result) {
    then_fn_(result);
    release_fn();
  }

  void notifiy_error(future_error const &ec) {
    error_fn_(ec);
    release_fn();
  }

  inline Future<Result> shared();

  void release_fn() {
    then_fn_ = {};
    error_fn_ = {};
  }

  then_fn then_fn_;
  error_fn error_fn_;
};

template <>
struct FutureImpl<void>
    : public std::enable_shared_from_this<FutureImpl<void>> {
  using self_type = FutureImpl<void>;
  using then_fn = std::function<void(void)>;
  using error_fn = std::function<void(future_error)>;

  self_type &then(then_fn fn) {
    then_fn_ = std::move(fn);
    return *this;
  }

  self_type &error(error_fn fn) {
    error_fn_ = std::move(fn);
    return *this;
  }

  void notifiy_then() {
    then_fn_();
    release_fn();
  }

  void notifiy_error(future_error const &ec) {
    error_fn_(ec);
    release_fn();
  }

  void release_fn() {
    then_fn_ = {};
    error_fn_ = {};
  }

  inline Future<void> shared();

  then_fn then_fn_;
  error_fn error_fn_;
};

template <typename Result> struct Future {
  using result_type = Result;
  using self_type = Future<result_type>;
  using impl_type = FutureImpl<result_type>;

  Future(std::shared_ptr<impl_type> future) : impl_(future) {}

  template <typename... Args> self_type &then(Args &&...args) {
    impl_->then(std::forward<Args>(args)...);
    return *this;
  }

  template <typename... Args> self_type &error(Args &&...args) {
    impl_->error(std::forward<Args>(args)...);
    return *this;
  }

  template <typename... Args> void notifiy_then(Args &&...args) {
    impl_->notifiy_then(std::forward<Args>(args)...);
  }

  template <typename... Args> void notifiy_error(Args &&...args) {
    impl_->notifiy_error(std::forward<Args>(args)...);
  }

  std::shared_ptr<impl_type> impl_;
};

template <typename Result> Future<Result> FutureImpl<Result>::shared() {
  return Future<Result>(this->shared_from_this());
}

Future<void> FutureImpl<void>::shared() {
  return Future<void>(this->shared_from_this());
}

template <typename Executor, typename Result> struct Promise {
  using future_type = FutureImpl<Result>;
  using executor_type = Executor;

  Promise(executor_type &io_ctx)
      : io_ctx_(io_ctx), future_(std::make_shared<future_type>()) {}

  void value(Result const &result) {
    boost::asio::post(io_ctx_,
                      std::bind(&future_type::notifiy_then, future_, result));
  }

  void value() {
    boost::asio::post(io_ctx_, std::bind(&future_type::notifiy_then, future_));
  }

  void error(future_error const &ec) {
    boost::asio::post(io_ctx_,
                      std::bind(&future_type::notifiy_error, future_, ec));
  }

  Future<Result> future() const { return future_->shared(); }

  executor_type &io_ctx_;
  std::shared_ptr<future_type> future_;
};

template <typename Executor> struct Promise<Executor, void> {
  using future_type = FutureImpl<void>;
  using executor_type = Executor;

  Promise(executor_type &io_ctx)
      : io_ctx_(io_ctx), future_(std::make_shared<future_type>()) {}

  void value() {
    boost::asio::post(io_ctx_, std::bind(&future_type::notifiy_then, future_));
  }

  void error(boost::system::error_code const &ec) {
    boost::asio::post(io_ctx_,
                      std::bind(&future_type::notifiy_error, future_, ec));
  }

  Future<void> future() const { return future_->shared(); }

  executor_type &io_ctx_;
  std::shared_ptr<future_type> future_;
};

template <typename Executor> struct JoinAll {
  struct inner {
    inner(Executor &ex) : fut_cnt_(-1), promise_(ex) {}
    ssize_t fut_cnt_;
    Promise<Executor, void> promise_;
  };

  JoinAll(Executor &ex) : impl_(std::make_shared<inner>(ex)) {}

  Future<void> join_all() { return impl_->promise_.future(); }

  template <typename _Future, typename... Rest,
            typename std::enable_if<
                std::is_same<void, typename _Future::result_type>::value,
                bool>::type = true>
  Future<void> join_all(_Future &future, Rest &&...futures) {
    future.then(std::bind(&JoinAll::void_then_cb, *this))
        .error(std::bind(&JoinAll::error_cb, *this, std::placeholders::_1));
    return join_all(std::forward<Rest>(futures)...);
  }

  template <typename _Future, typename... Rest,
            typename std::enable_if<
                !std::is_same<void, typename _Future::result_type>::value,
                bool>::type = true>
  Future<void> join_all(_Future &future, Rest &&...futures) {

    future
        .then(std::bind(&JoinAll::result_then_cb<typename _Future::result_type>,
                        *this, std::placeholders::_1))
        .error(std::bind(&JoinAll::error_cb, *this, std::placeholders::_1));
    return join_all(std::forward<Rest>(futures)...);
  }

  template <typename _Future, typename... Rest>
  Future<void> operator()(_Future &future, Rest &&...futures) {
    impl_->fut_cnt_ = std::tuple_size<std::tuple<_Future, Rest...>>::value;
    return join_all(future, std::forward<Rest>(futures)...);
  }

  void void_then_cb() { then_cb(); }

  template <typename Result> void result_then_cb(Result const &args) {
    then_cb();
  }

  void then_cb() {
    std::cout << impl_->fut_cnt_ << std::endl;
    --impl_->fut_cnt_;
    if (0 == impl_->fut_cnt_) {
      impl_->promise_.value();
    }
  }

  void error_cb(boost::system::error_code const &ec) {
    impl_->promise_.error(ec);
  }

  std::shared_ptr<inner> impl_;
};

template <typename Executor, typename... Futures>
static inline Future<void> join_all(Executor &io_ctx, Futures &&...futures) {
  return JoinAll<Executor>(io_ctx)(std::forward<Futures>(futures)...);
}

template <typename Executor, typename ReturnType>
static inline Future<ReturnType> promise_resolve(Executor &io_ctx,
                                                 ReturnType const &v) {
  Promise<Executor, ReturnType> promise(io_ctx);
  promise.value(v);
  return promise.future();
}

template <typename Executor>
static inline Future<void> promise_resolve(Executor &io_ctx) {
  Promise<Executor, void> promise(io_ctx);
  promise.value();
  return promise.future();
}

template <typename ReturnType, typename Executor>
static inline Future<ReturnType> promise_error(Executor &io_ctx,
                                               future_error const &ec) {
  Promise<Executor, ReturnType> promise(io_ctx);
  promise.error(ec);
  return promise.future();
}

namespace promise {} // namespace promise
