/**
 * @file scanner.hpp
 * @brief Optional CTRE-powered scanner and UTF-16 lexer-hook bridge.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ctre.hpp>

#include <glrpp/dsl/token.hpp>
#include <glrpp/util/error.hpp>
#include <glrpp/util/expected.hpp>

namespace glrpp::dsl {

template <std::size_t N>
struct fixed_pattern final {
  char value[N];

  constexpr fixed_pattern(const char (&text)[N]) {
    for (std::size_t index = 0; index < N; ++index) {
      value[index] = text[index];
    }
  }
};

struct scan_match final {
  std::size_t length = 0;
  std::size_t bytes_consumed = 0;
  std::uint32_t codepoint = 0;
  std::string_view lexeme{};
};

struct scan_rule final {
  std::string name;
  std::size_t priority = 0;
  bool skip = false;
  bool (*matcher)(std::string_view input, std::size_t offset, scan_match& match) = nullptr;
};

template <fixed_pattern Pattern>
struct ctre_matcher final {
  static bool match(std::string_view input, std::size_t offset, scan_match& match) {
    if (offset > input.size()) {
      return false;
    }
    auto view = input.substr(offset);
    if (auto result = ctre::starts_with<Pattern.value>(view); result) {
      match.length = std::string_view(result).size();
      match.lexeme = view.substr(0, match.length);
      if (match.length != 0) {
        match.bytes_consumed = match.length;
        if (!match.lexeme.empty()) {
          match.codepoint = static_cast<unsigned char>(match.lexeme.front());
        }
        return true;
      }
    }
    return false;
  }
};

template <fixed_pattern Pattern>
[[nodiscard]] inline scan_rule token_rule(std::string_view name, std::size_t priority = 0) {
  return scan_rule{std::string(name), priority, false, &ctre_matcher<Pattern>::match};
}

template <fixed_pattern Pattern>
[[nodiscard]] inline scan_rule skip_rule(std::string_view name = "skip", std::size_t priority = 0) {
  return scan_rule{std::string(name), priority, true, &ctre_matcher<Pattern>::match};
}

struct scanner_match final {
  const scan_rule* rule = nullptr;
  scan_match match{};
};

class scanner {
 public:
  scanner() = default;
  explicit scanner(std::vector<scan_rule> rules) : rules_(std::move(rules)) {}

  [[nodiscard]] util::expected<token_stream, util::parse_diagnostic> scan(std::string_view input) const {
    token_stream out;
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    while (offset < input.size()) {
      auto matched = match_at(input, offset);
      if (!matched.has_value()) {
        return util::unexpected<util::parse_diagnostic>{make_scan_error(input, offset, line, column)};
      }

      const auto& current = matched.value();
      if (!current.rule->skip) {
        out.push_back(make_token(current.rule->name, current.match.lexeme, offset, line, column,
                                 current.match.bytes_consumed, current.match.codepoint, true));
      }

      advance_position(current.match.lexeme, line, column);
      offset += current.match.length;
    }

    return out;
  }

  [[nodiscard]] util::expected<scanner_match, util::parse_diagnostic> match_at(std::string_view input,
                                                                                std::size_t offset,
                                                                                std::size_t line = 1,
                                                                                std::size_t column = 1) const {
    const scan_rule* best_rule = nullptr;
    scan_match best_match{};

    for (const auto& rule : rules_) {
      scan_match current_match{};
      if (rule.matcher != nullptr && rule.matcher(input, offset, current_match)) {
        if (current_match.length > best_match.length ||
            (current_match.length == best_match.length && best_rule != nullptr && rule.priority > best_rule->priority)) {
          best_rule = &rule;
          best_match = current_match;
        }
      }
    }

    if (best_rule == nullptr || best_match.length == 0) {
      return util::unexpected<util::parse_diagnostic>{make_scan_error(input, offset, line, column)};
    }

    return scanner_match{best_rule, best_match};
  }

  [[nodiscard]] const std::vector<scan_rule>& rules() const noexcept { return rules_; }

 private:
  [[nodiscard]] static util::parse_diagnostic make_scan_error(std::string_view input, std::size_t offset,
                                                              std::size_t line, std::size_t column) {
    return {"scanner could not classify input",
            "a token rule",
            std::string(input.substr(offset, std::min<std::size_t>(16, input.size() - offset))),
            {offset, line, column},
            offset};
  }

  static void advance_position(std::string_view lexeme, std::size_t& line, std::size_t& column) {
    for (char ch : lexeme) {
      if (ch == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
    }
  }

  std::vector<scan_rule> rules_;
};

}  // namespace glrpp::dsl
