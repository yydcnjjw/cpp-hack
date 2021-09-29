#pragma once

namespace rx {

struct tag_operator {};
template <class T> struct operator_base {
  using value_type = T;
  using operator_tag = tag_operator;
};

} // namespace rx
