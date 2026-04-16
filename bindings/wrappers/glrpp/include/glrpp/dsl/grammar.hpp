/**
 * @file grammar.hpp
 * @brief Grammar container and validation helpers.
 */

#pragma once

#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <glrpp/dsl/rule.hpp>
#include <glrpp/util/error.hpp>

namespace glrpp::dsl {

/** @brief Runtime grammar assembled through the C++ DSL. */
class grammar {
 public:
  grammar() = default;

  grammar(std::string start_symbol, std::vector<rule> rules)
      : start_(std::move(start_symbol)), rules_(std::move(rules)) {
    validate();
  }

  [[nodiscard]] const std::string& start() const noexcept { return start_; }
  [[nodiscard]] const std::vector<rule>& rules() const noexcept { return rules_; }

  [[nodiscard]] const rule* find_rule(std::string_view lhs) const noexcept {
    const auto it = std::find_if(rules_.begin(), rules_.end(), [&](const rule& current) { return current.lhs == lhs; });
    return it == rules_.end() ? nullptr : &*it;
  }

  [[nodiscard]] std::vector<symbol> terminals() const {
    std::vector<symbol> result;
    collect_symbols([&](const symbol& current) {
      if (current.terminal()) {
        result.push_back(current);
      }
    });
    return result;
  }

  [[nodiscard]] std::vector<symbol> nonterminals() const {
    std::set<std::string> names;
    for (const auto& current : rules_) {
      names.insert(current.lhs);
    }
    std::vector<symbol> result;
    result.reserve(names.size());
    for (const auto& name : names) {
      result.push_back(nonterminal(name));
    }
    return result;
  }

 private:
  void collect_symbols(const auto& visitor) const {
    const auto walk = [&](const auto& self, const expression& expr) -> void {
      if (expr.kind == expr_kind::atom) {
        visitor(expr.atom);
      }
      for (const auto& child : expr.children) {
        self(self, child);
      }
    };
    for (const auto& current : rules_) {
      walk(walk, current.rhs);
    }
  }

  void validate() const {
    if (start_.empty()) {
      throw util::grammar_error("grammar start symbol must not be empty");
    }
    if (rules_.empty()) {
      throw util::grammar_error("grammar must contain at least one rule");
    }
    if (find_rule(start_) == nullptr) {
      throw util::grammar_error("grammar start symbol has no matching rule");
    }
  }

  std::string start_;
  std::vector<rule> rules_;
};

/** @brief Helper builder for concise grammar creation. */
[[nodiscard]] inline grammar make_grammar(std::string_view start, std::initializer_list<rule> rules) {
  return grammar(std::string(start), std::vector<rule>(rules));
}

}  // namespace glrpp::dsl
