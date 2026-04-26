/**
 * @file config.hpp
 * @brief Global configuration and feature macros for glrpp.
 */

#pragma once

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

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

#define GLRPP_HAS_SWIG_BINDINGS 0

#if __has_include(<glr/glr.h>)
#define GLRPP_HAS_LIBGLR 1
#else
#define GLRPP_HAS_LIBGLR 0
#endif

#if defined(HAVE_LMDB)
#define GLRPP_HAS_LMDB_CACHE 1
#elif defined(__has_include)
#if __has_include(<glr/cache.h>)
#define GLRPP_HAS_LMDB_CACHE 1
#else
#define GLRPP_HAS_LMDB_CACHE 0
#endif
#else
#define GLRPP_HAS_LMDB_CACHE 0
#endif

namespace glrpp {

namespace detail {

[[nodiscard]] inline std::vector<std::filesystem::path> split_search_path(
    const char* value) {
  std::vector<std::filesystem::path> result;
  if (value == nullptr || *value == '\0') {
    return result;
  }

  std::string current;
  for (const char* ptr = value; *ptr != '\0'; ++ptr) {
    if (*ptr == ':') {
      if (!current.empty()) {
        result.emplace_back(current);
        current.clear();
      }
      continue;
    }
    current.push_back(*ptr);
  }
  if (!current.empty()) {
    result.emplace_back(current);
  }
  return result;
}

}  // namespace detail

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

/** @brief Returns the preferred environment variable for overriding `libglr.so`. */
[[nodiscard]] inline constexpr std::string_view sharedlib_envvar() noexcept {
  return "LIBGLR_SHAREDLIB";
}

/** @brief Returns the runtime environment variable searched by the loader first on Unix-like systems. */
[[nodiscard]] inline constexpr std::string_view loader_path_envvar() noexcept {
  return "LD_LIBRARY_PATH";
}

/** @brief Returns the default shared-library file name expected by glrpp. */
[[nodiscard]] inline constexpr std::string_view default_sharedlib_name() noexcept {
  return "libglr.so";
}

/**
 * @brief Builds the ordered list of shared-library candidates glrpp will try.
 *
 * The order is:
 * 1. `$LIBGLR_SHAREDLIB`, if set.
 * 2. Each directory in `$LD_LIBRARY_PATH`, combined with `libglr.so`.
 * 3. `/lib`, `/usr/lib`, `/usr/local/lib`, and `~/.local/lib`.
 * 4. Bare library names passed to the platform loader.
 */
[[nodiscard]] inline std::vector<std::string> shared_library_candidates() {
  std::vector<std::string> candidates;

  if (const char* explicit_path = std::getenv(sharedlib_envvar().data());
      explicit_path != nullptr && *explicit_path != '\0') {
    candidates.emplace_back(explicit_path);
    return candidates;
  }

  for (const auto& dir :
       detail::split_search_path(std::getenv(loader_path_envvar().data()))) {
    candidates.push_back((dir / default_sharedlib_name()).string());
  }

  candidates.emplace_back("/lib/libglr.so");
  candidates.emplace_back("/usr/lib/libglr.so");
  candidates.emplace_back("/usr/local/lib/libglr.so");

  if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
    candidates.push_back((std::filesystem::path(home) / ".local/lib/libglr.so").string());
  }

  candidates.emplace_back("libglr.so");
  candidates.emplace_back("libglr");
  candidates.emplace_back("glr");
  return candidates;
}

}  // namespace glrpp
