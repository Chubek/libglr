/**
 * @file cache.hpp
 * @brief RAII helpers for libglr's incremental parsing cache.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <glrpp/config.hpp>
#include <glrpp/glr/context.hpp>

namespace glrpp::glr {

#if GLRPP_HAS_LMDB_CACHE

/** @brief Value-type configuration used when opening an incremental parse cache. */
struct cache_config final {
  std::string mdbx_path = ".";
  std::size_t map_size = 1024ull * 1024ull * 1024ull;
  std::uint32_t max_readers = 126;
  bool use_async = false;
  std::uint64_t ttl_seconds = 86400;

  [[nodiscard]] glr_cache_config_t native_config() const noexcept {
    glr_cache_config_t native{};
    native.mdbx_path = mdbx_path.c_str();
    native.map_size = map_size;
    native.max_readers = max_readers;
    native.use_async = use_async;
    native.ttl_seconds = ttl_seconds;
    return native;
  }
};

/** @brief Snapshot of cache activity counters reported by libglr. */
struct cache_stats final {
  std::uint64_t forest_hits = 0;
  std::uint64_t forest_misses = 0;
  std::uint64_t gss_hits = 0;
  std::uint64_t gss_misses = 0;
  std::uint64_t subtree_hits = 0;
  std::uint64_t subtree_misses = 0;
  std::uint64_t cache_size_bytes = 0;
  std::uint32_t forest_count = 0;
  std::uint32_t gss_count = 0;
  std::uint32_t subtree_count = 0;

  cache_stats() = default;
  explicit cache_stats(const glr_cache_stats_t& native)
      : forest_hits(native.forest_hits),
        forest_misses(native.forest_misses),
        gss_hits(native.gss_hits),
        gss_misses(native.gss_misses),
        subtree_hits(native.subtree_hits),
        subtree_misses(native.subtree_misses),
        cache_size_bytes(native.cache_size_bytes),
        forest_count(native.forest_count),
        gss_count(native.gss_count),
        subtree_count(native.subtree_count) {}

  [[nodiscard]] double hit_rate() const noexcept {
    const auto total_hits = forest_hits + gss_hits + subtree_hits;
    const auto total_misses = forest_misses + gss_misses + subtree_misses;
    const auto total = total_hits + total_misses;
    return total == 0 ? 0.0 : static_cast<double>(total_hits) / static_cast<double>(total);
  }
};

/** @brief Owning handle around `glr_cache_t` with explicit lifecycle operations. */
class cache {
 public:
  explicit cache(const cache_config& config) {
    const auto& api = context::api();
    if (api.cache_open == nullptr) {
      throw std::runtime_error("glrpp: incremental cache support is unavailable in the loaded libglr runtime");
    }
    auto native = config.native_config();
    native_cache_ = api.cache_open(&native);
    if (native_cache_ == nullptr) {
      throw std::runtime_error("glrpp: failed to open incremental cache");
    }
  }

  ~cache() {
    if (native_cache_ != nullptr) {
      if (const auto close = context::api().cache_close; close != nullptr) {
        close(native_cache_);
      }
    }
  }

  cache(const cache&) = delete;
  cache& operator=(const cache&) = delete;

  cache(cache&& other) noexcept : native_cache_(other.native_cache_) { other.native_cache_ = nullptr; }

  cache& operator=(cache&& other) noexcept {
    if (this != &other) {
      if (native_cache_ != nullptr) {
        if (const auto close = context::api().cache_close; close != nullptr) {
          close(native_cache_);
        }
      }
      native_cache_ = other.native_cache_;
      other.native_cache_ = nullptr;
    }
    return *this;
  }

  void sync() {
    const auto fn = context::api().cache_sync;
    if (fn == nullptr || fn(native_cache_) != 0) {
      throw std::runtime_error("glrpp: failed to sync incremental cache");
    }
  }

  [[nodiscard]] cache_stats get_stats() const {
    glr_cache_stats_t native{};
    const auto fn = context::api().cache_get_stats;
    if (fn == nullptr || fn(native_cache_, &native) != 0) {
      throw std::runtime_error("glrpp: failed to fetch incremental cache statistics");
    }
    return cache_stats(native);
  }

  void clear() {
    const auto fn = context::api().cache_clear;
    if (fn == nullptr || fn(native_cache_) != 0) {
      throw std::runtime_error("glrpp: failed to clear incremental cache");
    }
  }

  void vacuum() {
    const auto fn = context::api().cache_vacuum;
    if (fn == nullptr || fn(native_cache_) != 0) {
      throw std::runtime_error("glrpp: failed to vacuum incremental cache");
    }
  }

  [[nodiscard]] glr_cache_t* native_handle() const noexcept { return native_cache_; }

 private:
  glr_cache_t* native_cache_ = nullptr;
};

#endif

}  // namespace glrpp::glr
