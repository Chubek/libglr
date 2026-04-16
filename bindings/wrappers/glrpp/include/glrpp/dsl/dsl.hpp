/**
 * @file dsl.hpp
 * @brief Umbrella include and top-level conveniences for the grammar DSL.
 */

#pragma once

#include <glrpp/dsl/actions.hpp>
#include <glrpp/dsl/ast.hpp>
#include <glrpp/dsl/grammar.hpp>
#include <glrpp/dsl/rule.hpp>
#include <glrpp/dsl/scanner.hpp>
#include <glrpp/dsl/symbol.hpp>
#include <glrpp/dsl/token.hpp>
#include <glrpp/meta/pipeline.hpp>

namespace glrpp {
using dsl::alt;
using dsl::ast_array;
using dsl::ast_node;
using dsl::expression;
using dsl::grammar;
using dsl::literal;
using dsl::make_grammar;
using dsl::make_token;
using dsl::nonterminal;
using dsl::opt;
using dsl::plus;
using dsl::production;
using dsl::scanner;
using dsl::seq;
using dsl::skip_rule;
using dsl::star;
using dsl::sym;
using dsl::terminal;
using dsl::token;
using dsl::token_rule;
using dsl::token_stream;
}  // namespace glrpp
