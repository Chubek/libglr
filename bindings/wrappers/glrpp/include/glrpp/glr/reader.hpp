/**
 * @file reader.hpp
 * @brief UTF-16 reader facade with CTRE-backed lexer hook integration.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glrpp/dsl/scanner.hpp>
#include <glrpp/glr/context.hpp>
#include <glrpp/util/error.hpp>
#include <glrpp/util/expected.hpp>

namespace glrpp::glr {

namespace detail {

struct utf16_window final {
  std::string utf8;
  std::vector<std::size_t> utf8_to_byte_offsets;
};

[[nodiscard]] inline bool detect_utf16_big_endian(const unsigned char* input, std::size_t length) {
  if (input == nullptr || length < 2) {
    return false;
  }
  if (input[0] == 0xFE && input[1] == 0xFF) {
    return true;
  }
  if (input[0] == 0xFF && input[1] == 0xFE) {
    return false;
  }
  return false;
}

[[nodiscard]] inline std::uint16_t read_u16(const unsigned char* data, bool big_endian) {
  if (big_endian) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[0]) << 8) | data[1]);
  }
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[1]) << 8) | data[0]);
}

[[nodiscard]] inline utf16_window utf16_event_to_utf8_window(const glr_lexer_event_t& event) {
  utf16_window window;
  if (event.input == nullptr || event.byte_offset >= event.input_length) {
    return window;
  }

  const bool big_endian = detect_utf16_big_endian(event.input, event.input_length);
  const auto* bytes = event.input + event.byte_offset;
  const auto remaining = event.input_length - event.byte_offset;
  window.utf8.reserve(remaining / 2);

  for (std::size_t offset = 0; offset + 1 < remaining;) {
    const auto first = read_u16(bytes + offset, big_endian);
    std::uint32_t codepoint = first;
    std::size_t consumed = 2;

    if (first >= 0xD800 && first <= 0xDBFF && offset + 3 < remaining) {
      const auto second = read_u16(bytes + offset + 2, big_endian);
      if (second >= 0xDC00 && second <= 0xDFFF) {
        codepoint = 0x10000 + ((((std::uint32_t)first - 0xD800) << 10) | ((std::uint32_t)second - 0xDC00));
        consumed = 4;
      }
    }

    const auto emit_byte = [&](char byte) {
      window.utf8.push_back(byte);
      window.utf8_to_byte_offsets.push_back(offset + consumed);
    };

    if (codepoint <= 0x7F) {
      emit_byte(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
      emit_byte(static_cast<char>(0xC0 | (codepoint >> 6)));
      emit_byte(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
      emit_byte(static_cast<char>(0xE0 | (codepoint >> 12)));
      emit_byte(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      emit_byte(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
      emit_byte(static_cast<char>(0xF0 | (codepoint >> 18)));
      emit_byte(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
      emit_byte(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      emit_byte(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }

    offset += consumed;
  }

  return window;
}

[[nodiscard]] inline std::string utf8_to_utf16le_bytes(std::string_view input, bool with_bom = false) {
  std::string out;
  out.reserve(input.size() * 2 + (with_bom ? 2 : 0));
  if (with_bom) {
    out.push_back(static_cast<char>(0xFF));
    out.push_back(static_cast<char>(0xFE));
  }
  for (unsigned char ch : input) {
    out.push_back(static_cast<char>(ch));
    out.push_back('\0');
  }
  return out;
}

}  // namespace detail

class reader {
 public:
  struct token_view final {
    std::string terminal_name;
    std::uint32_t codepoint = 0;
    std::size_t byte_offset = 0;
    std::size_t bytes_consumed = 0;
    bool from_hook = false;
  };

  explicit reader(std::shared_ptr<dsl::scanner> scanner = nullptr) : scanner_(std::move(scanner)) {
    native_reader_ = context::api().reader_create();
    if (native_reader_ == nullptr) {
      throw util::grammar_error("glrpp: failed to create native reader");
    }
    context::api().reader_set_encoding(native_reader_, GLR_READER_ENCODING_UTF16_AUTO);
    if (scanner_ != nullptr) {
      install_scanner_hooks();
      (void)context::api().reader_set_lexer_hooks(native_reader_, lexer_hooks_);
    }
  }

  ~reader() {
    if (native_reader_ != nullptr) {
      context::api().reader_destroy(native_reader_);
    }
    if (lexer_hooks_ != nullptr) {
      context::api().lexer_hooks_destroy(lexer_hooks_);
    }
  }

  reader(const reader&) = delete;
  reader& operator=(const reader&) = delete;

  [[nodiscard]] util::expected<void, util::parse_diagnostic> set_input(std::string_view utf16_bytes) {
    input_.assign(utf16_bytes.begin(), utf16_bytes.end());
    if (context::api().reader_set_input(native_reader_, input_.data(), input_.size()) != 0) {
      return util::unexpected<util::parse_diagnostic>{{"reader rejected input", "valid UTF-16 bytes", "<invalid>", {0, 1, 1}, 0}};
    }
    return {};
  }

  [[nodiscard]] util::expected<void, util::parse_diagnostic> set_input(const std::u16string& utf16) {
    input_.assign(reinterpret_cast<const char*>(utf16.data()), utf16.size() * sizeof(char16_t));
    if (context::api().reader_set_input(native_reader_, input_.data(), input_.size()) != 0) {
      return util::unexpected<util::parse_diagnostic>{{"reader rejected input", "valid UTF-16 units", "<invalid>", {0, 1, 1}, 0}};
    }
    return {};
  }

  void reset() { context::api().reader_reset(native_reader_); }

  [[nodiscard]] util::expected<token_view, util::parse_diagnostic> next() const {
    glr_reader_token_t native_token{};
    const auto status = context::api().reader_next(native_reader_, &native_token);
    if (status != GLR_READER_STATUS_OK) {
      if (status == GLR_READER_STATUS_EOF) {
        return util::unexpected<util::parse_diagnostic>{{"end of input", "token", "<eof>", {input_.size(), 1, input_.size() + 1}, input_.size()}};
      }
      return util::unexpected<util::parse_diagnostic>{{context::api().reader_status_string(status), "valid UTF-16 token", "<invalid>", {0, 1, 1}, 0}};
    }

    token_view view{native_token.terminal_name != nullptr ? native_token.terminal_name : std::string{},
                    native_token.codepoint,
                    native_token.byte_offset,
                    native_token.bytes_consumed,
                    native_token.from_hook};
    context::api().reader_token_clear(&native_token);
    return view;
  }

 private:
  struct hook_payload final {
    std::shared_ptr<dsl::scanner> scanner;
    std::string storage;
  };

  void install_scanner_hooks() {
    lexer_hooks_ = context::api().lexer_hooks_create();
    if (lexer_hooks_ == nullptr) {
      throw util::grammar_error("glrpp: failed to create lexer hook registry");
    }

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

  std::shared_ptr<dsl::scanner> scanner_;
  glr_reader_t* native_reader_ = nullptr;
  glr_lexer_hooks_t* lexer_hooks_ = nullptr;
  mutable std::string input_;
};

}  // namespace glrpp::glr
