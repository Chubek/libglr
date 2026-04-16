/**
 * @file node.hpp
 * @brief Lightweight parse forest node handle.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glrpp/dsl/symbol.hpp>

extern "C" {
#include <glr/forest.h>
#include <glr/grammar.h>
}

namespace glrpp::glr {

struct node_data final {
  std::string name;
  std::vector<std::shared_ptr<node_data>> children;
  std::size_t begin = 0;
  std::size_t end = 0;
};

/** @brief Cheap handle to parse-forest storage. */
class node {
 public:
  node() = default;
  explicit node(std::shared_ptr<node_data> data) : data_(std::move(data)) {}
#if GLRPP_HAS_LIBGLR || GLRPP_HAS_SWIG_BINDINGS
  node(void* native, const void* grammar) : native_(native), grammar_(grammar) {}
#endif

  [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(data_); }
  [[nodiscard]] explicit operator bool() const noexcept { return valid(); }
  [[nodiscard]] std::string_view name() const noexcept { return data_ ? std::string_view(data_->name) : std::string_view{}; }
  [[nodiscard]] std::size_t begin() const noexcept { return data_ ? data_->begin : 0; }
  [[nodiscard]] std::size_t end() const noexcept { return data_ ? data_->end : 0; }

  [[nodiscard]] std::vector<node> children() const {
    if (native_ != nullptr) {
      auto* raw = static_cast<glr_forest_node_t*>(native_);
      std::vector<node> result;
      result.reserve(raw->child_count);
      for (std::size_t index = 0; index < raw->child_count; ++index) {
        result.emplace_back(raw->children[index], grammar_);
      }
      return result;
    }
    std::vector<node> result;
    if (!data_) {
      return result;
    }
    result.reserve(data_->children.size());
    for (const auto& child : data_->children) {
      result.emplace_back(child);
    }
    return result;
  }

  [[nodiscard]] dsl::symbol symbol() const { return dsl::symbol{std::string(name()), dsl::symbol_kind::nonterminal}; }

  [[nodiscard]] void* native_handle() const noexcept { return native_; }

 private:
  std::shared_ptr<node_data> data_;
  void* native_ = nullptr;
  const void* grammar_ = nullptr;

  [[nodiscard]] std::string_view resolve_name() const noexcept {
    if (native_ != nullptr && grammar_ != nullptr) {
      auto* raw = static_cast<glr_forest_node_t*>(native_);
      auto* grammar = static_cast<const glr_grammar_t*>(grammar_);
      if (raw->symbol_id >= 0) {
        auto* symbol = glr_grammar_get_symbol(grammar, raw->symbol_id);
        if (symbol != nullptr && symbol->name != nullptr) {
          return symbol->name;
        }
      }
    }
    return data_ ? std::string_view(data_->name) : std::string_view{};
  }
};

}  // namespace glrpp::glr
