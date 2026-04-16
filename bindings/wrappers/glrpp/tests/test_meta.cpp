#include <glrpp/glrpp.hpp>

#include <array>
#include <cassert>
#include <string_view>
#include <type_traits>

namespace {

enum class sample_flag { off = 0, on = 7 };

struct sample_record final {
  int id = 0;
  bool active = false;
};

template <typename value_type>
struct is_not_pointer : std::bool_constant<!std::is_pointer_v<value_type>> {};

}  // namespace

template <>
struct glrpp::meta::fields<sample_record> {
  static constexpr auto names = std::array<std::string_view, 2>{"id", "active"};
};

int main() {
  using basic_types = glrpp::meta::type_list<int, double>;
  using expanded_types = glrpp::meta::push_back_t<basic_types, char>;
  using merged_types = glrpp::meta::concat_t<basic_types, glrpp::meta::type_list<bool>, glrpp::meta::type_list<char>>;

  static_assert(glrpp::meta::size_v<basic_types> == 2);
  static_assert(glrpp::meta::size_v<expanded_types> == 3);
  static_assert(glrpp::meta::size_v<merged_types> == 4);

  static_assert(glrpp::meta::all_of<merged_types, is_not_pointer>::value);
  static_assert(glrpp::meta::reflectable<sample_record>);

  constexpr auto flag_value = glrpp::meta::enum_name(sample_flag::on);
  static_assert(std::is_same_v<decltype(flag_value), const int>);
  assert(flag_value == 7);

  constexpr auto field_names = glrpp::meta::field_names_v<sample_record>;
  static_assert(field_names.size() == 2);
  assert(field_names[0] == "id");
  assert(field_names[1] == "active");

  const auto int_name = glrpp::meta::type_name<int>();
  assert(!int_name.empty());
  assert(int_name.find("int") != std::string_view::npos);

  const glrpp::util::expected<int, std::string_view> success(21);
  assert(success.has_value());
  const auto doubled = success.map([](const int value) { return value * 2; });
  assert(doubled.has_value());
  assert(doubled.value() == 42);

  const glrpp::util::expected<int, std::string_view> failure(
      glrpp::util::unexpected<std::string_view>{"boom"});
  assert(!failure.has_value());
  const auto chained = failure.and_then([](const int value) {
    return glrpp::util::expected<int, std::string_view>(value + 1);
  });
  assert(!chained.has_value());
  assert(chained.error() == "boom");

  return 0;
}
