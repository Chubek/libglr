/**
 * @file context.hpp
 * @brief Dynamic runtime context for libglr loaded via libltdl.
 */

#pragma once

#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

#include <glrpp/config.hpp>

extern "C" {
#include <glr/forest.h>
#include <glr/grammar.h>
#include <glr/lexer-hooks.h>
#include <glr/parser.h>
#include <glr/reader.h>
#include <ltdl.h>
}

namespace glrpp::glr {

struct runtime_api final {
  using grammar_create_fn = glr_grammar_t* (*)();
  using grammar_destroy_fn = void (*)(glr_grammar_t*);
  using grammar_add_symbol_fn = int (*)(glr_grammar_t*, glr_symbol_type_t, const char*);
  using grammar_get_symbol_fn = glr_symbol_t* (*)(const glr_grammar_t*, int);
  using grammar_add_production_fn = int (*)(glr_grammar_t*, int, glr_symbol_t**, size_t);
  using grammar_set_start_symbol_fn = int (*)(glr_grammar_t*, int);
  using parser_create_fn = glr_parser_t* (*)(glr_grammar_t*);
  using parser_destroy_fn = void (*)(glr_parser_t*);
  using parse_fn = glr_parse_result_t (*)(glr_parser_t*, const char*, size_t);
  using parser_set_lexer_hooks_fn = int (*)(glr_parser_t*, glr_lexer_hooks_t*);
  using lexer_hooks_create_fn = glr_lexer_hooks_t* (*)();
  using lexer_hooks_destroy_fn = void (*)(glr_lexer_hooks_t*);
  using lexer_hooks_add_fn = int (*)(glr_lexer_hooks_t*, const char*, int, glr_lexer_hook_fn, void*, glr_lexer_hook_destroy_fn);
  using reader_create_fn = glr_reader_t* (*)();
  using reader_destroy_fn = void (*)(glr_reader_t*);
  using reader_set_encoding_fn = void (*)(glr_reader_t*, glr_reader_encoding_t);
  using reader_set_input_fn = int (*)(glr_reader_t*, const void*, size_t);
  using reader_reset_fn = void (*)(glr_reader_t*);
  using reader_set_lexer_hooks_fn = int (*)(glr_reader_t*, glr_lexer_hooks_t*);
  using reader_next_fn = glr_reader_status_t (*)(glr_reader_t*, glr_reader_token_t*);
  using reader_token_clear_fn = void (*)(glr_reader_token_t*);
  using reader_status_string_fn = const char* (*)(glr_reader_status_t);
  using forest_destroy_fn = void (*)(glr_forest_t*);

  grammar_create_fn grammar_create = nullptr;
  grammar_destroy_fn grammar_destroy = nullptr;
  grammar_add_symbol_fn grammar_add_symbol = nullptr;
  grammar_get_symbol_fn grammar_get_symbol = nullptr;
  grammar_add_production_fn grammar_add_production = nullptr;
  grammar_set_start_symbol_fn grammar_set_start_symbol = nullptr;
  parser_create_fn parser_create = nullptr;
  parser_destroy_fn parser_destroy = nullptr;
  parse_fn parse = nullptr;
  parser_set_lexer_hooks_fn parser_set_lexer_hooks = nullptr;
  lexer_hooks_create_fn lexer_hooks_create = nullptr;
  lexer_hooks_destroy_fn lexer_hooks_destroy = nullptr;
  lexer_hooks_add_fn lexer_hooks_add = nullptr;
  reader_create_fn reader_create = nullptr;
  reader_destroy_fn reader_destroy = nullptr;
  reader_set_encoding_fn reader_set_encoding = nullptr;
  reader_set_input_fn reader_set_input = nullptr;
  reader_reset_fn reader_reset = nullptr;
  reader_set_lexer_hooks_fn reader_set_lexer_hooks = nullptr;
  reader_next_fn reader_next = nullptr;
  reader_token_clear_fn reader_token_clear = nullptr;
  reader_status_string_fn reader_status_string = nullptr;
  forest_destroy_fn forest_destroy = nullptr;
};

class context {
 public:
  context() = default;
  explicit context(std::string input) : input_(std::move(input)) {}

  [[nodiscard]] std::string_view input() const noexcept { return input_; }
  void reset(std::string input) { input_ = std::move(input); }

  static const runtime_api& api() {
    static context singleton;
    singleton.ensure_runtime();
    return singleton.api_;
  }

 private:
  void ensure_runtime() {
    std::call_once(init_flag_, [this] {
      if (lt_dlinit() != 0) {
        throw std::runtime_error("glrpp: lt_dlinit failed");
      }

      handle_ = lt_dlopenext("libglr");
      if (handle_ == nullptr) {
        handle_ = lt_dlopenext("glr");
      }
      if (handle_ == nullptr) {
        const char* error = lt_dlerror();
        throw std::runtime_error(std::string("glrpp: failed to load libglr from shared library paths") +
                                 (error ? std::string(": ") + error : std::string{}));
      }

      api_.grammar_create = load_symbol<runtime_api::grammar_create_fn>("glr_grammar_create");
      api_.grammar_destroy = load_symbol<runtime_api::grammar_destroy_fn>("glr_grammar_destroy");
      api_.grammar_add_symbol = load_symbol<runtime_api::grammar_add_symbol_fn>("glr_grammar_add_symbol");
      api_.grammar_get_symbol = load_symbol<runtime_api::grammar_get_symbol_fn>("glr_grammar_get_symbol");
      api_.grammar_add_production = load_symbol<runtime_api::grammar_add_production_fn>("glr_grammar_add_production");
      api_.grammar_set_start_symbol = load_symbol<runtime_api::grammar_set_start_symbol_fn>("glr_grammar_set_start_symbol");
      api_.parser_create = load_symbol<runtime_api::parser_create_fn>("glr_parser_create");
      api_.parser_destroy = load_symbol<runtime_api::parser_destroy_fn>("glr_parser_destroy");
      api_.parse = load_symbol<runtime_api::parse_fn>("glr_parse");
      api_.parser_set_lexer_hooks = load_symbol<runtime_api::parser_set_lexer_hooks_fn>("glr_parser_set_lexer_hooks");
      api_.lexer_hooks_create = load_symbol<runtime_api::lexer_hooks_create_fn>("glr_lexer_hooks_create");
      api_.lexer_hooks_destroy = load_symbol<runtime_api::lexer_hooks_destroy_fn>("glr_lexer_hooks_destroy");
      api_.lexer_hooks_add = load_symbol<runtime_api::lexer_hooks_add_fn>("glr_lexer_hooks_add");
      api_.reader_create = load_symbol<runtime_api::reader_create_fn>("glr_reader_create");
      api_.reader_destroy = load_symbol<runtime_api::reader_destroy_fn>("glr_reader_destroy");
      api_.reader_set_encoding = load_symbol<runtime_api::reader_set_encoding_fn>("glr_reader_set_encoding");
      api_.reader_set_input = load_symbol<runtime_api::reader_set_input_fn>("glr_reader_set_input");
      api_.reader_reset = load_symbol<runtime_api::reader_reset_fn>("glr_reader_reset");
      api_.reader_set_lexer_hooks = load_symbol<runtime_api::reader_set_lexer_hooks_fn>("glr_reader_set_lexer_hooks");
      api_.reader_next = load_symbol<runtime_api::reader_next_fn>("glr_reader_next");
      api_.reader_token_clear = load_symbol<runtime_api::reader_token_clear_fn>("glr_reader_token_clear");
      api_.reader_status_string = load_symbol<runtime_api::reader_status_string_fn>("glr_reader_status_string");
      api_.forest_destroy = load_symbol<runtime_api::forest_destroy_fn>("glr_forest_destroy");
    });
  }

  template <typename Fn>
  [[nodiscard]] Fn load_symbol(const char* name) {
    auto* symbol = lt_dlsym(handle_, name);
    if (symbol == nullptr) {
      const char* error = lt_dlerror();
      throw std::runtime_error(std::string("glrpp: missing symbol ") + name +
                               (error ? std::string(" (") + error + ")" : std::string{}));
    }
    return reinterpret_cast<Fn>(symbol);
  }

  std::string input_;
  runtime_api api_{};
  lt_dlhandle handle_ = nullptr;
  std::once_flag init_flag_{};
};

}  // namespace glrpp::glr
