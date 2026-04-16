/**
 * @file ast.hpp
 * @brief AST primitives used by glrpp grammars and examples.
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace glrpp::dsl {

struct ast_node;

using ast_array = std::vector<ast_node>;
using ast_object = std::map<std::string, ast_node>;

/** @brief Generic tree node suited for diagnostics, tests, and lightweight wrappers. */
struct ast_node final {
  using value_type = std::variant<std::monostate, std::string, std::int64_t, double, bool, ast_array, ast_object>;

  std::string kind;
  value_type value;

  ast_node() = default;
  ast_node(std::string kind_value, value_type current)
      : kind(std::move(kind_value)), value(std::move(current)) {}

  [[nodiscard]] bool is_null() const noexcept { return std::holds_alternative<std::monostate>(value); }
  [[nodiscard]] bool is_array() const noexcept { return std::holds_alternative<ast_array>(value); }
  [[nodiscard]] bool is_object() const noexcept { return std::holds_alternative<ast_object>(value); }
};

template <typename... Ts>
using ast_variant = std::variant<Ts...>;

template <typename T>
concept ast_like = requires(const T& value) {
  typename T::value_type;
  value.kind;
};

}  // namespace glrpp::dsl
