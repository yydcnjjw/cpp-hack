#pragma once

#include <assert.h>
#include <boost/callable_traits/is_invocable.hpp>
#include <boost/type_traits.hpp>
#include <rx/subscriber.hpp>
#include <rx/type.hpp>

namespace rx {

template <typename InputType, typename OutputType, typename Selector>
struct map_operator {
  using input_type = InputType;
  using select_type = Selector;

  using output_type = OutputType;

  using is_async_invoke =
      boost::callable_traits::is_invocable<Selector, async_context<output_type>,
                                           input_type>;

  select_type selector_;

  map_operator(select_type s) : selector_(std::move(s)) {}

  template <typename Subscriber> struct map_observer {
    using self_type = map_observer<Subscriber>;
    using dest_type = Subscriber;

    using observer_type =
        observer<input_type, detail::forward_observer_tag, self_type>;

    dest_type dest_;
    select_type selector_;
    map_observer(dest_type dest, select_type selector)
        : dest_(std::move(dest)), selector_(std::move(selector)) {}

    template <typename V> void on_next(V &&v) {
      on_next(std::forward<V>(v), is_async_invoke());
    }

    template <typename V> void on_next(V &&v, std::false_type) {
      dest_.on_next(selector_(std::forward<V>(v)));
    }

    template <typename V> void on_next(V &&v, std::true_type) {
      selector_(make_async_context<output_type>(
          std::bind(&self_type::on_selector, this, std::placeholders::_1,
                    std::placeholders::_2)));
    }

    template <typename V>
    void on_next(async_context<void> yield, V &&v, std::false_type) {
      dest_.on_next(std::move(yield), selector_(std::forward<V>(v)));
    }

    template <typename V> void on_next(async_context<void> yield, V &&v) {
      on_next(yield, std::forward<V>(v), is_async_invoke());
    }

    template <typename V>
    void on_next(async_context<void> yield, V &&v, std::true_type) {
      selector_(make_async_context<output_type>(
          std::bind(&self_type::on_selector, this, std::move(yield),
                    std::placeholders::_1, std::placeholders::_2)));
    }

    void on_selector(error_type e, output_type o) { dest_.on_next(o); }

    void on_selector(async_context<void> yield, error_type e, output_type o) {
      dest_.on_next(yield, o);
    }

    template <typename... Args> void on_error(Args &&...args) {
      return dest_.on_error(std::forward<Args>(args)...);
    }

    template <typename... Args> void on_completed(Args &&...args) {
      return dest_.on_completed(std::forward<Args>(args)...);
    }

    template <typename _Subscriber>
    static subscriber<input_type, observer_type> make(_Subscriber &&dest,
                                                      select_type s) {
      return make_subscriber<input_type>(
          dest.get_subscription(),
          make_observer<input_type>(
              self_type(std::forward<_Subscriber>(dest), std::move(s))));
    }
  };

  template <typename Subscriber>
  subscriber<input_type, observer<input_type, detail::forward_observer_tag,
                                  map_observer<Subscriber>>>
  operator()(Subscriber &&dest) {
    return map_observer<Subscriber>::make(std::forward<Subscriber>(dest),
                                          selector_);
  }
};

} // namespace rx
