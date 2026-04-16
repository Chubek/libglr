/**
 * @file error.hpp
 * @brief Error objects and exception types for parser workflows.
 */

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace glrpp::util {

/** @brief Source location within an input stream. */
struct source_position final {
  std::size_t offset = 0;
  std::size_t line = 1;
  std::size_t column = 1;
};

/** @brief Structured parse diagnostic. */
struct parse_diagnostic final {
  std::string message;
  std::string expected;
  std::string found;
  source_position position{};
  std::size_t consumed = 0;

  [[nodiscard]] inline std::string format() const {
    std::string out = message;
    if (!expected.empty()) {
      out += " | expected: ";
      out += expected;
    }
    if (!found.empty()) {
      out += " | found: ";
      out += found;
    }
    if (consumed != 0) {
      out += " | consumed: ";
      out += std::to_string(consumed);
    }
    out += " @" + std::to_string(position.line) + ":" + std::to_string(position.column);
    return out;
  }
};

/** @brief Exception thrown for hard parse failures. */
class parse_error : public std::runtime_error {
 public:
  explicit parse_error(parse_diagnostic diagnostic)
      : std::runtime_error(diagnostic.format()), diagnostic_(std::move(diagnostic)) {}

  [[nodiscard]] const parse_diagnostic& diagnostic() const noexcept { return diagnostic_; }

 private:
  parse_diagnostic diagnostic_;
};

/** @brief Exception thrown for malformed grammars. */
class grammar_error : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

}  // namespace glrpp::util
