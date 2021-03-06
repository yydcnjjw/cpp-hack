#pragma once

#include <rx/observer.hpp>
#include <rx/sources/interval.hpp>
#include <rx/subscriber.hpp>
#include <rx/subscription.hpp>

// #include <rx/operators/filter.hpp>
// #include <rx/operators/filter_map.hpp>
#include <rx/operators/lift.hpp>
#include <rx/operators/map.hpp>

#include <utility>

namespace rx {

template <typename T> struct observable_base { using value_type = T; };

template <typename SourceOperator,
          typename T = typename SourceOperator::value_type>
class observable : public observable_base<T> {
  using self_type = observable<SourceOperator>;
  using source_operator_type = SourceOperator;

public:
  explicit observable(source_operator_type const &so) : so_(so) {}
  explicit observable(source_operator_type &&so) : so_(std::move(so)) {}

  template <typename Subscriber> subscription &detail_subscribe(Subscriber o) {
    so_.on_subscribe(o);
    return o.get_subscription();
  }

  template <typename OutputType, typename Operator,
            typename LiftOperator =
                rx::lift_operator<OutputType, source_operator_type, Operator>>
  observable<LiftOperator> lift(Operator &&op) const {
    return observable<LiftOperator>(
        LiftOperator(so_, std::forward<Operator>(op)));
  }

  template <typename OutputType, typename Selector,
            typename SourceValue = typename self_type::value_type,
            typename Map = rx::map_operator<SourceValue, OutputType, Selector>,
            typename LiftOperator =
                rx::lift_operator<OutputType, source_operator_type, Map>>
  observable<LiftOperator> map(Selector &&s) {
    return lift<OutputType>(Map(std::forward<Selector>(s)));
  }

  // template <typename Predicate,
  //           typename SourceValue = typename self_type::value_type,
  //           typename Map = rx::filter_operator<SourceValue, Predicate>,
  //           typename LiftOperator =
  //               rx::lift_operator<SourceValue, source_operator_type, Map>>
  // observable<LiftOperator> filter(Predicate &&pred) {
  //   return lift<SourceValue>(Map(std::forward<Predicate>(pred)));
  // }

  // template <typename Selector,
  //           typename SourceValue = typename self_type::value_type,
  //           typename FilterMap = rx::filter_map_operator<SourceValue,
  //           Selector>, typename OutputType = typename FilterMap::output_type,
  //           typename LiftOperator =
  //               rx::lift_operator<OutputType, source_operator_type,
  //               FilterMap>>
  // observable<LiftOperator> filter_map(Selector &&s) {
  //   return lift<OutputType>(FilterMap(std::forward<Selector>(s)));
  // }

  template <typename... Args> subscription subscribe(Args &&...args) {
    return detail_subscribe(make_subscriber<T>(std::forward<Args>(args)...));
  }

private:
  source_operator_type so_;
};

template <> struct observable<void, void> {
  template <typename SourceType = source::interval, typename... Args>
  static observable<SourceType> interval(Args &&...args) {
    return observable<SourceType>(SourceType(std::forward<Args>(args)...));
  }
};

} // namespace rx
