/**
 * @file rule.hpp
 * @brief Production rules and expression combinators for grammars.
 */

#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glrpp/dsl/actions.hpp>
#include <glrpp/dsl/ast.hpp>
#include <glrpp/dsl/symbol.hpp>

namespace glrpp::dsl {

/** @brief Expression kinds supported by the grammar DSL. */
enum class expr_kind {
  atom,
  sequence,
  choice,
  optional,
  zero_or_more,
  one_or_more
};

/** @brief Runtime grammar expression tree. */
struct expression final {
  expr_kind kind = expr_kind::atom;
  symbol atom = epsilon();
  std::vector<expression> children;

  [[nodiscard]] static expression make_atom(symbol current) {
    return expression{expr_kind::atom, std::move(current), {}};
  }
};

[[nodiscard]] inline expression sym(symbol current) { return expression::make_atom(std::move(current)); }

[[nodiscard]] inline expression seq(std::initializer_list<expression> items) {
  return expression{expr_kind::sequence, epsilon(), std::vector<expression>(items)};
}

[[nodiscard]] inline expression alt(std::initializer_list<expression> items) {
  return expression{expr_kind::choice, epsilon(), std::vector<expression>(items)};
}

[[nodiscard]] inline expression opt(expression item) {
  return expression{expr_kind::optional, epsilon(), {std::move(item)}};
}

[[nodiscard]] inline expression star(expression item) {
  return expression{expr_kind::zero_or_more, epsilon(), {std::move(item)}};
}

[[nodiscard]] inline expression plus(expression item) {
  return expression{expr_kind::one_or_more, epsilon(), {std::move(item)}};
}

/** @brief One production rule of a grammar. */
struct rule final {
  std::string lhs;
  expression rhs;
  std::optional<action<std::function<ast_node(const ast_array&)>>> reducer;
};

/** @brief User-friendly factory for grammar rules. */
[[nodiscard]] inline rule production(std::string_view lhs, expression rhs) {
  return rule{std::string(lhs), std::move(rhs), std::nullopt};
}

}  // namespace glrpp::dsl
