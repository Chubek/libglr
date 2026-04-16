/**
 * @file expected.hpp
 * @brief Minimal expected<T, E> implementation with light monadic helpers.
 */

#pragma once

#include <type_traits>
#include <utility>
#include <variant>

namespace glrpp::util {

template <typename Error>
struct unexpected final {
  Error error;
};

template <typename Value, typename Error>
class expected {
 public:
  using value_type = Value;
  using error_type = Error;

  expected(const Value& value) : state_(value) {}
  expected(Value&& value) : state_(std::move(value)) {}
  expected(const unexpected<Error>& error) : state_(error.error) {}
  expected(unexpected<Error>&& error) : state_(std::move(error.error)) {}

  [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<Value>(state_); }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

  [[nodiscard]] Value& value() & { return std::get<Value>(state_); }
  [[nodiscard]] const Value& value() const& { return std::get<Value>(state_); }
  [[nodiscard]] Value&& value() && { return std::get<Value>(std::move(state_)); }

  [[nodiscard]] Error& error() & { return std::get<Error>(state_); }
  [[nodiscard]] const Error& error() const& { return std::get<Error>(state_); }

  template <typename Fn>
  [[nodiscard]] auto map(Fn&& fn) const -> expected<std::invoke_result_t<Fn, const Value&>, Error> {
    using next_type = expected<std::invoke_result_t<Fn, const Value&>, Error>;
    if (has_value()) {
      return next_type(std::forward<Fn>(fn)(value()));
    }
    return unexpected<Error>{error()};
  }

  template <typename Fn>
  [[nodiscard]] auto and_then(Fn&& fn) const -> std::invoke_result_t<Fn, const Value&> {
    using next_type = std::invoke_result_t<Fn, const Value&>;
    if (has_value()) {
      return std::forward<Fn>(fn)(value());
    }
    return next_type(unexpected<Error>{error()});
  }

 private:
  std::variant<Value, Error> state_;
};

template <typename Error>
struct expected<void, Error> {
  bool ok = true;
  Error err{};

  expected() = default;
  expected(unexpected<Error> error) : ok(false), err(std::move(error.error)) {}

  [[nodiscard]] bool has_value() const noexcept { return ok; }
  [[nodiscard]] explicit operator bool() const noexcept { return ok; }
  [[nodiscard]] const Error& error() const noexcept { return err; }
};

}  // namespace glrpp::util
