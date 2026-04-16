/**
 * @file reflect.hpp
 * @brief Reflection helpers for types, enums, and aggregate-like AST values.
 */

#pragma once

#include <array>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>

namespace glrpp::meta {

template <typename T>
[[nodiscard]] inline std::string_view type_name() noexcept {
#if defined(__clang__) || defined(__GNUC__)
  return __PRETTY_FUNCTION__;
#else
  return typeid(T).name();
#endif
}

template <typename Enum>
[[nodiscard]] constexpr auto enum_name(const Enum value) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(value);
}

template <typename T>
struct fields {
  static constexpr auto names = std::array<std::string_view, 0>{};
};

template <typename T>
inline constexpr auto field_names_v = fields<T>::names;

template <typename T>
concept reflectable = requires { field_names_v<T>; };

}  // namespace glrpp::meta
