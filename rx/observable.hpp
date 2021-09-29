#pragma once

#include <rx/observer.hpp>
#include <rx/sources/interval.hpp>
#include <rx/subscriber.hpp>
#include <rx/subscription.hpp>

#include <rx/operators/filter.hpp>
#include <rx/operators/lift.hpp>
#include <rx/operators/map.hpp>

#include <utility>

namespace rx {

struct tag_observable {};

template <typename T> struct observable_base {
  using observable_tag = tag_observable;
  using value_type = T;
};

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

  template <typename Selector,
            typename SourceValue = typename self_type::value_type,
            typename Map = rx::map_operator<SourceValue, Selector>,
            typename OutputType = typename Map::output_type,
            typename LiftOperator =
                rx::lift_operator<OutputType, source_operator_type, Map>>
  observable<LiftOperator> map(Selector &&s) {
    return lift<OutputType>(Map(std::forward<Selector>(s)));
  }

  template <typename Predicate,
            typename SourceValue = typename self_type::value_type,
            typename Map = rx::filter_operator<SourceValue, Predicate>,
            typename LiftOperator =
                rx::lift_operator<SourceValue, source_operator_type, Map>>
  observable<LiftOperator> filter(Predicate &&pred) {
    return lift<SourceValue>(Map(std::forward<Predicate>(pred)));
  }

  template <typename Executor, typename Observer>
  subscription &subscribe(Executor &ex, Observer ob) {
    return detail_subscribe(
        subscriber<Executor, T>(ex, observer<T>(std::move(ob))));
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
