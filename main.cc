#include <boost/callable_traits/is_invocable.hpp>
#include <functional>
#include <iostream>

template <typename Output> struct async_context {};

namespace ct = boost::callable_traits;

template <typename Invoke> struct is_async_invoke {
  // using type = is_async_invoke_impl<Invoke>
};
template <typename InvokeType> struct async_invoke_output {
  using type = void;
};

template <typename OutputType, typename... Args>
struct async_invoke_output<void (*)(async_context<OutputType>, Args...)> {
  using type = OutputType;
};

template <typename OutputType, typename... Args>
struct async_invoke_output<
    std::function<void(async_context<OutputType>, Args...)>> {
  using type = OutputType;
};

// template <typename Selector> struct async_invoke_output {
//   template <typename OutputType, typename... Args>
//   using type = typename std::conditional<
//       ct::is_invocable<Selector, async_context<OutputType>, Args...>::value,
//       OutputType, void>::type;
// };

// template <typename Selector> void a(Selector &&) {
//   static_assert(
//       std::is_same<typename async_invoke_output<Selector>::template type<>,
//                    long>::value,
//       "");
// }


int main(int argc, char *argv[]) {
  return 0;
}
