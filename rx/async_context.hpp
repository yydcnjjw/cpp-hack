#pragma once

#include <functional>
#include <rx/type.hpp>

namespace rx {

namespace detail {

template <typename ValueType> class async_context {
public:
  using value_type = ValueType;
  using ptr = std::shared_ptr<async_context>;

  template <typename Next>
  async_context(Next &&next) : next_(std::forward<Next>(next)) {}

  ~async_context() {
    if (next_) {
      next_(ec_, v_);
    }
  }

  void value(value_type const &v) { v_ = v; }

  void error(error_type const &e) { ec_ = e; }

private:
  value_type v_;
  error_type ec_;
  std::function<void(error_type const &, value_type const &)> next_;
};

template <> class async_context<void> {

public:
  using ptr = std::shared_ptr<async_context>;

  template <typename Next>
  async_context(Next &&next) : next_(std::forward<Next>(next)) {}

  ~async_context() {
    if (next_) {
      next_(ec_);
    }
  }

  void error(error_type const &e) { ec_ = e; }

private:
  error_type ec_;
  std::function<void(error_type const &)> next_;
};

} // namespace detail

template <typename ValueType>
using async_context = typename detail::async_context<ValueType>::ptr;

// TODO: Infer ValueType in Next
template <typename ValueType, typename Next,
          typename AsyncContext = detail::async_context<ValueType>>
async_context<ValueType> make_async_context(Next &&next) {
  return std::make_shared<AsyncContext>(std::forward<Next>(next));
}

} // namespace rx
