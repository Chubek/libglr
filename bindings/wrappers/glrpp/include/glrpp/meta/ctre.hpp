/**
 * @file ctre.hpp
 * @brief Tiny constexpr validators used by the grammar DSL.
 */

#pragma once

#include <cctype>
#include <string_view>

namespace glrpp::meta {

/** @brief Checks whether a DSL identifier uses letters, digits, or underscores. */
[[nodiscard]] constexpr bool is_identifier(std::string_view text) noexcept {
  if (text.empty()) {
    return false;
  }
  const auto first = static_cast<unsigned char>(text.front());
  if (!(std::isalpha(first) || first == '_')) {
    return false;
  }
  for (const char ch : text.substr(1)) {
    const auto value = static_cast<unsigned char>(ch);
    if (!(std::isalnum(value) || value == '_')) {
      return false;
    }
  }
  return true;
}

/** @brief Checks whether a literal token body is non-empty. */
[[nodiscard]] constexpr bool is_literal(std::string_view text) noexcept { return !text.empty(); }

}  // namespace glrpp::meta
