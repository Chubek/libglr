/**
 * @file string_utils.hpp
 * @brief Small string helpers used across the DSL and diagnostics.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace glrpp::util {

[[nodiscard]] constexpr bool is_space(const char ch) noexcept {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v';
}

/** @brief Trims whitespace on the left side of a string view. */
[[nodiscard]] constexpr std::string_view ltrim(std::string_view text) noexcept {
  while (!text.empty() && is_space(text.front())) {
    text.remove_prefix(1);
  }
  return text;
}

/** @brief Trims whitespace on the right side of a string view. */
[[nodiscard]] constexpr std::string_view rtrim(std::string_view text) noexcept {
  while (!text.empty() && is_space(text.back())) {
    text.remove_suffix(1);
  }
  return text;
}

/** @brief Trims whitespace on both sides of a string view. */
[[nodiscard]] constexpr std::string_view trim(std::string_view text) noexcept {
  return rtrim(ltrim(text));
}

/** @brief Returns true when the text starts with the given prefix. */
[[nodiscard]] constexpr bool starts_with(std::string_view text, std::string_view prefix) noexcept {
  return text.substr(0, prefix.size()) == prefix;
}

/** @brief Returns true when the text ends with the given suffix. */
[[nodiscard]] constexpr bool ends_with(std::string_view text, std::string_view suffix) noexcept {
  return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

/** @brief Escapes common control characters into a printable string. */
[[nodiscard]] inline std::string escape(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (const char ch : text) {
    switch (ch) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(ch); break;
    }
  }
  return out;
}

/** @brief Splits a string view by a single-character delimiter. */
[[nodiscard]] inline std::vector<std::string_view> split(std::string_view text, const char delim) {
  std::vector<std::string_view> parts;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto pos = text.find(delim, start);
    if (pos == std::string_view::npos) {
      parts.emplace_back(text.substr(start));
      break;
    }
    parts.emplace_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return parts;
}

template <typename Range>
[[nodiscard]] inline std::string join(const Range& parts, std::string_view separator) {
  std::ostringstream stream;
  bool first = true;
  for (const auto& part : parts) {
    if (!first) {
      stream << separator;
    }
    first = false;
    stream << part;
  }
  return stream.str();
}

}  // namespace glrpp::util
