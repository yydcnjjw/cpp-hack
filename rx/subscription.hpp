#pragma once

#include <memory>

namespace rx {

class subscription {
  struct state {
    state() : is_subscribed(true) {}
    bool is_subscribed;
  };

public:
  subscription() : s_(std::make_shared<state>()) {}

  bool is_subscribed() const { return s_->is_subscribed; }

  void unsubscribe() { s_->is_subscribed = false; }

private:
  std::shared_ptr<state> s_;
};

} // namespace rx
