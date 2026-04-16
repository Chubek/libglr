/**
 * @file actions.hpp
 * @brief Semantic action helpers used by rules and parsers.
 */

#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace glrpp::dsl {

/** @brief Opaque semantic value placeholder for generic actions. */
using semantic_values = std::vector<std::string>;

template <typename Fn>
struct action final {
  Fn fn;

  template <typename... Args>
  constexpr decltype(auto) operator()(Args&&... args) const {
    return fn(std::forward<Args>(args)...);
  }
};

template <typename Fn>
[[nodiscard]] constexpr auto make_action(Fn&& fn) {
  return action<std::decay_t<Fn>>{std::forward<Fn>(fn)};
}

/** @brief Identity action used when a rule does not transform its children. */
inline constexpr auto identity = make_action([](auto&& value) -> decltype(auto) {
  return std::forward<decltype(value)>(value);
});

}  // namespace glrpp::dsl
