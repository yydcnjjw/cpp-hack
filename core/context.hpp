#pragma once
#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <typeindex>

struct Context {
  template <typename... Args> void set(Args &&...args) {
    om_.emplace(std::forward<Args>(args)...);
  }

  template <typename T> boost::optional<T> get() {
    auto it = om_.find(typeid(T));
    if (it != om_.end()) {
      return boost::any_cast<T>(*it);
    } else {
      return boost::none;
    }
  }

  std::map<std::type_index, boost::any> om_;
};
