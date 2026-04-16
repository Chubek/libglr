/**
 * @file token.hpp
 * @brief Token model used by the parser adapter and examples.
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <glrpp/dsl/symbol.hpp>

namespace glrpp::dsl {

/** @brief Concrete token produced by user lexers. */
struct token final {
  std::string kind;
  std::string lexeme;
  std::size_t offset = 0;
  std::size_t line = 1;
  std::size_t column = 1;
  std::size_t bytes_consumed = 0;
  std::uint32_t codepoint = 0;
  bool from_hook = false;

  [[nodiscard]] symbol as_symbol() const { return terminal(kind); }
};

using token_stream = std::vector<token>;

/** @brief Convenience factory for tokens in tests or examples. */
[[nodiscard]] inline token make_token(std::string_view kind, std::string_view lexeme,
                                      const std::size_t offset = 0,
                                      const std::size_t line = 1,
                                      const std::size_t column = 1,
                                      const std::size_t bytes_consumed = 0,
                                      const std::uint32_t codepoint = 0,
                                      const bool from_hook = false) {
  return token{std::string(kind), std::string(lexeme), offset, line, column,
               bytes_consumed, codepoint, from_hook};
}

}  // namespace glrpp::dsl
