/**
 * @file parser.hpp
 * @brief High-level parser facade backed by dynamically loaded libglr.
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glrpp/dsl/grammar.hpp>
#include <glrpp/dsl/scanner.hpp>
#include <glrpp/dsl/token.hpp>
#include <glrpp/glr/context.hpp>
#include <glrpp/glr/forest.hpp>
#include <glrpp/glr/reader.hpp>
#include <glrpp/util/error.hpp>
#include <glrpp/util/expected.hpp>
#include <glrpp/util/string_utils.hpp>

namespace glrpp::glr {

class parser {
 public:
  explicit parser(dsl::grammar grammar, std::shared_ptr<dsl::scanner> scanner = nullptr)
      : grammar_(std::move(grammar)), scanner_(std::move(scanner)) {
    initialize_backend();
  }

  ~parser() {
    if (native_parser_ != nullptr) {
      context::api().parser_destroy(native_parser_);
    }
    if (lexer_hooks_ != nullptr) {
      context::api().lexer_hooks_destroy(lexer_hooks_);
    }
    if (native_grammar_ != nullptr) {
      context::api().grammar_destroy(native_grammar_);
    }
  }

  parser(const parser&) = delete;
  parser& operator=(const parser&) = delete;

  [[nodiscard]] const dsl::grammar& grammar() const noexcept { return grammar_; }
  [[nodiscard]] const std::shared_ptr<dsl::scanner>& scanner() const noexcept { return scanner_; }

  [[nodiscard]] util::expected<forest, util::parse_diagnostic> parse(const dsl::token_stream& tokens) const {
    if (native_parser_ == nullptr) {
      return util::unexpected<util::parse_diagnostic>{{"libglr runtime is unavailable", "loaded libglr", "<none>", {0, 1, 1}, 0}};
    }

    const std::string serialized = serialize_tokens(tokens);
    auto result = context::api().parse(native_parser_, serialized.c_str(), serialized.size());
    if (result.error != GLR_PARSE_SUCCESS) {
      return util::unexpected<util::parse_diagnostic>{make_diagnostic(result.error, result.position, tokens)};
    }

    return forest(result.forest, native_grammar_, std::shared_ptr<void>(native_parser_, [](void*) {}));
  }

  [[nodiscard]] util::expected<forest, util::parse_diagnostic> parse(std::string_view input) const {
    if (native_parser_ == nullptr) {
      return util::unexpected<util::parse_diagnostic>{{"libglr runtime is unavailable", "loaded libglr", "<none>", {0, 1, 1}, 0}};
    }

    if (scanner_ != nullptr) {
      const auto utf16 = detail::utf8_to_utf16le_bytes(input, true);
      auto result = context::api().parse(native_parser_, utf16.data(), utf16.size());
      if (result.error != GLR_PARSE_SUCCESS) {
        return util::unexpected<util::parse_diagnostic>{make_diagnostic(result.error, result.position, {})};
      }
      return forest(result.forest, native_grammar_, std::shared_ptr<void>(native_parser_, [](void*) {}));
    }

    auto result = context::api().parse(native_parser_, input.data(), input.size());
    if (result.error != GLR_PARSE_SUCCESS) {
      return util::unexpected<util::parse_diagnostic>{make_diagnostic(result.error, result.position, {})};
    }

    return forest(result.forest, native_grammar_, std::shared_ptr<void>(native_parser_, [](void*) {}));
  }

 private:
  dsl::grammar grammar_;
  std::shared_ptr<dsl::scanner> scanner_;
  glr_grammar_t* native_grammar_ = nullptr;
  glr_parser_t* native_parser_ = nullptr;
  glr_lexer_hooks_t* lexer_hooks_ = nullptr;

  void initialize_backend() {
    const auto& api = context::api();
    native_grammar_ = api.grammar_create();
    if (native_grammar_ == nullptr) {
      throw util::grammar_error("glrpp: failed to create native grammar");
    }

    struct symbol_entry final {
      int id = -1;
      dsl::symbol_kind kind = dsl::symbol_kind::nonterminal;
    };

    std::unordered_map<std::string, symbol_entry> ids;
    ids.reserve(grammar_.rules().size() * 2 + 8);

    const auto ensure_symbol = [&](std::string_view name, dsl::symbol_kind kind) -> int {
      const auto key = std::string(name);
      if (const auto it = ids.find(key); it != ids.end()) {
        return it->second.id;
      }
      const auto native_kind = (kind == dsl::symbol_kind::nonterminal) ? GLR_SYMBOL_NONTERMINAL : GLR_SYMBOL_TERMINAL;
      const int id = api.grammar_add_symbol(native_grammar_, native_kind, key.c_str());
      if (id >= 0) {
        ids.emplace(key, symbol_entry{id, kind});
      }
      return id;
    };

    for (const auto& rule : grammar_.rules()) {
      if (ensure_symbol(rule.lhs, dsl::symbol_kind::nonterminal) < 0) {
        throw util::grammar_error("glrpp: failed to register nonterminal " + rule.lhs);
      }
      flatten_symbols(rule.rhs, [&](const dsl::symbol& current) {
        if (current.kind != dsl::symbol_kind::epsilon) {
          (void)ensure_symbol(current.name, current.kind == dsl::symbol_kind::nonterminal ? current.kind : dsl::symbol_kind::terminal);
        }
      });
    }

    for (const auto& rule : grammar_.rules()) {
      std::vector<glr_symbol_t*> body;
      flatten_symbols(rule.rhs, [&](const dsl::symbol& current) {
        if (current.kind == dsl::symbol_kind::epsilon) {
          return;
        }
        if (const auto it = ids.find(current.name); it != ids.end()) {
          body.push_back(context::api().grammar_get_symbol(native_grammar_, it->second.id));
        }
      });
      const auto head_it = ids.find(rule.lhs);
      if (head_it == ids.end() || context::api().grammar_add_production(native_grammar_, head_it->second.id, body.data(), body.size()) < 0) {
        throw util::grammar_error("glrpp: failed to add production for " + rule.lhs);
      }
    }

    const auto start_it = ids.find(grammar_.start());
    if (start_it == ids.end() || context::api().grammar_set_start_symbol(native_grammar_, start_it->second.id) != 0) {
      throw util::grammar_error("glrpp: failed to set start symbol");
    }

    native_parser_ = api.parser_create(native_grammar_);
    if (native_parser_ == nullptr) {
      throw util::grammar_error("glrpp: failed to create native parser");
    }

    if (scanner_ != nullptr) {
      lexer_hooks_ = api.lexer_hooks_create();
      if (lexer_hooks_ == nullptr) {
        throw util::grammar_error("glrpp: failed to create lexer hook registry");
      }
      install_scanner_hooks();
      (void)api.parser_set_lexer_hooks(native_parser_, lexer_hooks_);
    }
  }

  void install_scanner_hooks() {
    struct hook_payload final {
      std::shared_ptr<dsl::scanner> scanner;
      std::string storage;
    };

    auto* payload = new hook_payload{scanner_, {}};
    const auto bridge = [](const glr_lexer_event_t* event, glr_lexer_response_t* response, void* user_data) -> bool {
      auto* payload = static_cast<hook_payload*>(user_data);
      if (event == nullptr || response == nullptr || payload == nullptr || payload->scanner == nullptr) {
        return false;
      }

      const auto window = detail::utf16_event_to_utf8_window(*event);
      auto matched = payload->scanner->match_at(window.utf8, 0);
      if (!matched.has_value()) {
        return false;
      }

      const auto& value = matched.value();
      if (value.rule == nullptr || value.rule->skip) {
        return false;
      }

      payload->storage = value.rule->name;
      std::size_t consumed = event->default_bytes_consumed;
      if (value.match.length != 0 && value.match.length <= window.utf8_to_byte_offsets.size()) {
        consumed = window.utf8_to_byte_offsets[value.match.length - 1];
      }
      glr_lexer_response_accept(response, payload->storage.c_str(), consumed);
      return true;
    };

    const auto destroy = [](void* user_data) {
      delete static_cast<hook_payload*>(user_data);
    };

    if (context::api().lexer_hooks_add(lexer_hooks_, "glrpp.ctre", 100, bridge, payload, destroy) != 0) {
      destroy(payload);
      throw util::grammar_error("glrpp: failed to register CTRE lexer hook");
    }
  }

  template <typename Visitor>
  static void flatten_symbols(const dsl::expression& expr, Visitor&& visitor) {
    if (expr.kind == dsl::expr_kind::atom) {
      visitor(expr.atom);
      return;
    }
    for (const auto& child : expr.children) {
      flatten_symbols(child, visitor);
    }
  }

  [[nodiscard]] static std::string serialize_tokens(const dsl::token_stream& tokens) {
    std::string buffer;
    for (const auto& tok : tokens) {
      if (!buffer.empty()) {
        buffer.push_back(' ');
      }
      buffer += tok.kind;
    }
    return buffer;
  }


  [[nodiscard]] util::parse_diagnostic make_diagnostic(glr_parse_error_t error, std::size_t position,
                                                       const dsl::token_stream& tokens) const {
    util::parse_diagnostic diagnostic;
    diagnostic.message = parse_error_name(error);
    diagnostic.expected = grammar_.start();
    diagnostic.found = "<eof>";
    diagnostic.position = {position, 1, position + 1};
    diagnostic.consumed = position;

    for (const auto& tok : tokens) {
      if (tok.offset >= position) {
        diagnostic.found = tok.kind;
        diagnostic.position = {tok.offset, tok.line, tok.column};
        break;
      }
    }
    return diagnostic;
  }

  [[nodiscard]] static const char* parse_error_name(glr_parse_error_t error) noexcept {
    switch (error) {
      case GLR_PARSE_SUCCESS: return "success";
      case GLR_PARSE_ERROR_SYNTAX: return "syntax error";
      case GLR_PARSE_ERROR_MEMORY: return "memory error";
      case GLR_PARSE_ERROR_GRAMMAR: return "grammar error";
      case GLR_PARSE_ERROR_UNRECOVERABLE: return "unrecoverable parse error";
      default: return "unknown parse error";
    }
  }
};

}  // namespace glrpp::glr
