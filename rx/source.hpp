#pragma once

namespace rx {
namespace source {

struct tag_source {};

template <typename T> struct source_base {
  using value_type = T;
  using source_tag = tag_source;
};

} // namespace source
} // namespace rx
