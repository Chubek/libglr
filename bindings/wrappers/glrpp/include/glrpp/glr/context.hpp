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
#if GLRPP_HAS_LMDB_CACHE
#include <glr/cache.h>
#endif
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
  using parser_parse_incremental_fn = int (*)(glr_parser_t*, const glr_forest_t*, const char*, size_t, const char*, size_t,
                                              size_t, size_t, glr_forest_t**);
  using parser_set_lexer_hooks_fn = int (*)(glr_parser_t*, glr_lexer_hooks_t*);
#if GLRPP_HAS_LMDB_CACHE
  using parser_set_cache_fn = void (*)(glr_parser_t*, glr_cache_t*);
  using parser_get_cache_fn = glr_cache_t* (*)(const glr_parser_t*);
  using parser_enable_incremental_fn = int (*)(glr_parser_t*, const char*);
  using parser_disable_incremental_fn = void (*)(glr_parser_t*);
  using parser_get_cache_stats_fn = int (*)(glr_parser_t*, glr_cache_stats_t*);
  using cache_open_fn = glr_cache_t* (*)(const glr_cache_config_t*);
  using cache_close_fn = void (*)(glr_cache_t*);
  using cache_sync_fn = int (*)(glr_cache_t*);
  using cache_get_stats_fn = int (*)(glr_cache_t*, glr_cache_stats_t*);
  using cache_clear_fn = int (*)(glr_cache_t*);
  using cache_vacuum_fn = int (*)(glr_cache_t*);
#endif
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
  parser_parse_incremental_fn parser_parse_incremental = nullptr;
  parser_set_lexer_hooks_fn parser_set_lexer_hooks = nullptr;
#if GLRPP_HAS_LMDB_CACHE
  parser_set_cache_fn parser_set_cache = nullptr;
  parser_get_cache_fn parser_get_cache = nullptr;
  parser_enable_incremental_fn parser_enable_incremental = nullptr;
  parser_disable_incremental_fn parser_disable_incremental = nullptr;
  parser_get_cache_stats_fn parser_get_cache_stats = nullptr;
  cache_open_fn cache_open = nullptr;
  cache_close_fn cache_close = nullptr;
  cache_sync_fn cache_sync = nullptr;
  cache_get_stats_fn cache_get_stats = nullptr;
  cache_clear_fn cache_clear = nullptr;
  cache_vacuum_fn cache_vacuum = nullptr;
#endif
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

      for (const auto& candidate : glrpp::shared_library_candidates()) {
        handle_ = open_candidate(candidate);
        if (handle_ != nullptr) {
          break;
        }
      }
      if (handle_ == nullptr) {
        const char* error = lt_dlerror();
        throw std::runtime_error(
            std::string("glrpp: failed to load libglr.so; set LIBGLR_SHAREDLIB or place the library in "
                        "LD_LIBRARY_PATH, /lib, /usr/lib, /usr/local/lib, or ~/.local/lib") +
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
      api_.parser_parse_incremental = load_symbol<runtime_api::parser_parse_incremental_fn>("glr_parser_parse_incremental");
      api_.parser_set_lexer_hooks = load_symbol<runtime_api::parser_set_lexer_hooks_fn>("glr_parser_set_lexer_hooks");
#if GLRPP_HAS_LMDB_CACHE
      api_.parser_set_cache = try_load_symbol<runtime_api::parser_set_cache_fn>("glr_parser_set_cache");
      api_.parser_get_cache = try_load_symbol<runtime_api::parser_get_cache_fn>("glr_parser_get_cache");
      api_.parser_enable_incremental =
          try_load_symbol<runtime_api::parser_enable_incremental_fn>("glr_parser_enable_incremental");
      api_.parser_disable_incremental =
          try_load_symbol<runtime_api::parser_disable_incremental_fn>("glr_parser_disable_incremental");
      api_.parser_get_cache_stats =
          try_load_symbol<runtime_api::parser_get_cache_stats_fn>("glr_parser_get_cache_stats");
      api_.cache_open = try_load_symbol<runtime_api::cache_open_fn>("glr_cache_open");
      api_.cache_close = try_load_symbol<runtime_api::cache_close_fn>("glr_cache_close");
      api_.cache_sync = try_load_symbol<runtime_api::cache_sync_fn>("glr_cache_sync");
      api_.cache_get_stats = try_load_symbol<runtime_api::cache_get_stats_fn>("glr_cache_get_stats");
      api_.cache_clear = try_load_symbol<runtime_api::cache_clear_fn>("glr_cache_clear");
      api_.cache_vacuum = try_load_symbol<runtime_api::cache_vacuum_fn>("glr_cache_vacuum");
#endif
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

  template <typename Fn>
  [[nodiscard]] Fn try_load_symbol(const char* name) {
    auto* symbol = lt_dlsym(handle_, name);
    if (symbol == nullptr) {
      (void)lt_dlerror();
      return nullptr;
    }
    return reinterpret_cast<Fn>(symbol);
  }

  [[nodiscard]] static lt_dlhandle open_candidate(const std::string& candidate) {
    lt_dlhandle handle = lt_dlopen(candidate.c_str());
    if (handle != nullptr) {
      return handle;
    }
    return lt_dlopenext(candidate.c_str());
  }

  std::string input_;
  runtime_api api_{};
  lt_dlhandle handle_ = nullptr;
  std::once_flag init_flag_{};
};

}  // namespace glrpp::glr
