/**
 * @file brigand.hpp
 * @brief Small aliases over Brigand typelist primitives.
 */

#pragma once

#include <type_traits>

namespace glrpp::meta {

template <typename... Ts>
struct type_list final {};

template <typename List>
struct size;

template <typename... Ts>
struct size<type_list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename List>
inline constexpr std::size_t size_v = size<List>::value;

template <typename List, typename T>
struct push_back;

template <typename... Ts, typename T>
struct push_back<type_list<Ts...>, T> {
  using type = type_list<Ts..., T>;
};

template <typename List, typename T>
using push_back_t = typename push_back<List, T>::type;

template <typename... Lists>
struct concat;

template <>
struct concat<> {
  using type = type_list<>;
};

template <typename... Ts>
struct concat<type_list<Ts...>> {
  using type = type_list<Ts...>;
};

template <typename... Left, typename... Right, typename... Rest>
struct concat<type_list<Left...>, type_list<Right...>, Rest...> {
  using type = typename concat<type_list<Left..., Right...>, Rest...>::type;
};

template <typename... Lists>
using concat_t = typename concat<Lists...>::type;

template <typename List, template <typename> class Pred>
struct all_of;

template <template <typename> class Pred, typename... Ts>
struct all_of<type_list<Ts...>, Pred> : std::bool_constant<(Pred<Ts>::value && ...)> {};

}  // namespace glrpp::meta
