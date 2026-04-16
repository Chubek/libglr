/**
 * @file debug.hpp
 * @brief Human-friendly dumping helpers for grammar and parser objects.
 */

#pragma once

#include <iostream>
#include <string>

#include <glrpp/dsl/ast.hpp>
#include <glrpp/dsl/grammar.hpp>
#include <glrpp/dsl/token.hpp>
#include <glrpp/glr/forest.hpp>

namespace glrpp::util {

inline void dump(const dsl::token& tok, std::ostream& out = std::cout) {
  out << "token(" << tok.kind << ", \"" << tok.lexeme << "\")";
}

inline void dump(const dsl::grammar& grammar, std::ostream& out = std::cout) {
  out << "grammar(start=" << grammar.start() << ", rules=" << grammar.rules().size() << ")";
}

inline void dump(const glr::node& node, std::ostream& out = std::cout, const std::size_t depth = 0) {
  out << std::string(depth * 2, ' ') << node.name() << "\n";
  for (const auto& child : node.children()) {
    dump(child, out, depth + 1);
  }
}

inline void dump(const glr::forest& forest, std::ostream& out = std::cout) {
  for (const auto& root : forest.roots()) {
    dump(root, out, 0);
  }
}

}  // namespace glrpp::util
