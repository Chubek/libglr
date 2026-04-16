/**
 * @file pipeline.hpp
 * @brief Alternate-expression operator support for the grammar DSL.
 *
 * This header overloads `operator|` so grammar alternatives can be written in
 * familiar EBNF style:
 *
 * @code{.cpp}
 * using namespace glrpp::dsl;
 *
 * const auto expr = sym(nonterminal("expr")) | sym(nonterminal("term")) | literal("+");
 * @endcode
 *
 * The overloads flatten pre-existing choice nodes so chained pipelines produce
 * one coherent `expr_kind::choice` tree instead of deeply nested binary nodes.
 */

#pragma once

#include <type_traits>
#include <utility>
#include <vector>

#include <glrpp/dsl/ast.hpp>
#include <glrpp/dsl/rule.hpp>

namespace glrpp::dsl {
namespace detail {

template <typename value_type>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<value_type>>;

template <typename value_type>
inline constexpr bool is_expression_v = std::is_same_v<remove_cvref_t<value_type>, dsl::expression>;

template <typename value_type>
inline constexpr bool is_symbol_v = std::is_same_v<remove_cvref_t<value_type>, dsl::symbol>;

template <typename value_type>
inline constexpr bool is_pipeline_operand_v = is_expression_v<value_type> || is_symbol_v<value_type>;

template <typename value_type>
[[nodiscard]] inline dsl::expression to_expression(value_type&& value) {
  if constexpr (is_expression_v<value_type>) {
    return std::forward<value_type>(value);
  } else {
    return dsl::sym(std::forward<value_type>(value));
  }
}

inline void append_choice_branch(std::vector<dsl::expression>& branches, dsl::expression&& expr) {
  if (expr.kind == dsl::expr_kind::choice) {
    for (auto& child : expr.children) {
      branches.push_back(std::move(child));
    }
    return;
  }
  branches.push_back(std::move(expr));
}

}  // namespace detail

/**
 * @brief Builds an alternation expression from two DSL operands.
 *
 * Accepts both `glrpp::dsl::symbol` and `glrpp::dsl::expression` operands and
 * promotes symbols to atomic expressions automatically. Existing choice nodes
 * are flattened so `a | b | c` behaves like a single EBNF alternation.
 */
template <typename left_type, typename right_type,
          typename = std::enable_if_t<detail::is_pipeline_operand_v<left_type> &&
                                      detail::is_pipeline_operand_v<right_type>>>
[[nodiscard]] inline dsl::expression operator|(left_type&& left, right_type&& right) {
  std::vector<dsl::expression> branches;
  branches.reserve(4);

  detail::append_choice_branch(branches, detail::to_expression(std::forward<left_type>(left)));
  detail::append_choice_branch(branches, detail::to_expression(std::forward<right_type>(right)));

  return dsl::expression{dsl::expr_kind::choice, dsl::epsilon(), std::move(branches)};
}

}  // namespace glrpp::dsl
