/**
 * @file symbol.hpp
 * @brief Symbol descriptors used by the grammar DSL.
 */

#pragma once

#include <string>
#include <string_view>
#include <utility>

#include <glrpp/meta/ctre.hpp>

namespace glrpp::dsl {

/** @brief Category of grammar symbols. */
enum class symbol_kind {
  terminal,
  nonterminal,
  literal,
  epsilon
};

/** @brief Runtime descriptor for a grammar symbol. */
struct symbol final {
  std::string name;
  symbol_kind kind = symbol_kind::nonterminal;

  [[nodiscard]] constexpr bool terminal() const noexcept {
    return kind == symbol_kind::terminal || kind == symbol_kind::literal;
  }

  [[nodiscard]] constexpr bool nonterminal() const noexcept { return kind == symbol_kind::nonterminal; }
};

/** @brief Constructs a named terminal symbol. */
[[nodiscard]] inline symbol terminal(std::string_view name) {
  return symbol{std::string(name), symbol_kind::terminal};
}

/** @brief Constructs a named nonterminal symbol. */
[[nodiscard]] inline symbol nonterminal(std::string_view name) {
  return symbol{std::string(name), symbol_kind::nonterminal};
}

/** @brief Constructs a literal symbol. */
[[nodiscard]] inline symbol literal(std::string_view text) {
  return symbol{std::string(text), symbol_kind::literal};
}

/** @brief Returns the distinguished epsilon symbol. */
[[nodiscard]] inline symbol epsilon() { return symbol{"ε", symbol_kind::epsilon}; }

}  // namespace glrpp::dsl
