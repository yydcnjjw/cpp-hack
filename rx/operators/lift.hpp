#pragma once

#include <rx/operator.hpp>
#include <utility>

namespace rx {

template <typename OutputType, typename SourceOperator, typename Operator>
struct lift_operator : public operator_base<OutputType> {

  using source_operator_type = SourceOperator;
  using operator_type = Operator;

  lift_operator(source_operator_type s, operator_type op)
      : source_(s), chain_(op) {
  }

  template <typename Subscriber> void on_subscribe(Subscriber o) {
    source_.on_subscribe(chain_(std::move(o)));
  }

  source_operator_type source_;
  operator_type chain_;
};

} // namespace rx
