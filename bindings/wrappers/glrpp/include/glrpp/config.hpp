/**
 * @file config.hpp
 * @brief Global configuration and feature macros for glrpp.
 */

#pragma once

#include <cstddef>
#include <string_view>

#define GLRPP_VERSION_MAJOR 0
#define GLRPP_VERSION_MINOR 1
#define GLRPP_VERSION_PATCH 0

#define GLRPP_VERSION_STRING "0.1.0"

#if defined(__cpp_consteval)
#define GLRPP_CONSTEVAL consteval
#else
#define GLRPP_CONSTEVAL constexpr
#endif

#if defined(__cpp_lib_source_location)
#define GLRPP_HAS_SOURCE_LOCATION 1
#else
#define GLRPP_HAS_SOURCE_LOCATION 0
#endif

#if __has_include(<glrpp_glr_bindings.hpp>)
#define GLRPP_HAS_SWIG_BINDINGS 1
#else
#define GLRPP_HAS_SWIG_BINDINGS 0
#endif

#if __has_include(<glr/glr.h>)
#define GLRPP_HAS_LIBGLR 1
#else
#define GLRPP_HAS_LIBGLR 0
#endif

namespace glrpp {

/** @brief Semantic version for the wrapper library. */
struct version final {
  std::size_t major = GLRPP_VERSION_MAJOR;
  std::size_t minor = GLRPP_VERSION_MINOR;
  std::size_t patch = GLRPP_VERSION_PATCH;

  [[nodiscard]] constexpr std::string_view string() const noexcept {
    return GLRPP_VERSION_STRING;
  }
};

/** @brief Returns the compile-time version of glrpp. */
[[nodiscard]] constexpr version library_version() noexcept { return {}; }

}  // namespace glrpp
