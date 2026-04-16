/**
 * @file forest.hpp
 * @brief Parse forest view returned by the parser.
 */

#pragma once

#include <memory>
#include <vector>

#include <glrpp/glr/node.hpp>

namespace glrpp::glr {

/** @brief Collection of parse roots, supporting ambiguous parses. */
class forest {
 public:
  forest() = default;
  explicit forest(std::vector<node> roots) : roots_(std::move(roots)) {}
  forest(void* native, const void* grammar, std::shared_ptr<void> owner)
      : native_(native), grammar_(grammar), owner_(std::move(owner)) {
    hydrate_roots();
  }

  [[nodiscard]] bool empty() const noexcept { return roots_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return roots_.size(); }
  [[nodiscard]] const std::vector<node>& roots() const noexcept { return roots_; }
  [[nodiscard]] const node& front() const { return roots_.front(); }

 private:
  std::vector<node> roots_;
  void* native_ = nullptr;
  const void* grammar_ = nullptr;
  std::shared_ptr<void> owner_;

  void hydrate_roots() {
    auto* raw = static_cast<glr_forest_t*>(native_);
    if (raw == nullptr) {
      return;
    }
    roots_.clear();
    for (std::size_t position = 0; position < raw->node_count; ++position) {
      for (auto* current = raw->nodes[position]; current != nullptr; current = current->next) {
        if (current->type == GLR_NODE_NONTERMINAL) {
          roots_.emplace_back(current, grammar_);
        }
      }
    }
  }
};

}  // namespace glrpp::glr
