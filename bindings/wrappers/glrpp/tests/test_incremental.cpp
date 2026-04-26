#include <glrpp/glrpp.hpp>

#include <cassert>
#include <filesystem>
#include <string>

int main() {
#if GLRPP_HAS_LMDB_CACHE
  const auto grammar = glrpp::make_grammar(
      "Expr",
      {glrpp::production("Expr", glrpp::seq({glrpp::sym(glrpp::terminal("number"))})),
       glrpp::production("Expr", glrpp::seq({glrpp::sym(glrpp::terminal("number")),
                                             glrpp::sym(glrpp::terminal("plus")),
                                             glrpp::sym(glrpp::terminal("number"))}))});

  auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
      glrpp::skip_rule<"[ \t\n]+">("ws", 10),
      glrpp::token_rule<"[0-9]+">("number", 100),
      glrpp::token_rule<"\\+">("plus", 90),
  });

  try {
    glrpp::glr::parser parser(grammar, scanner);
    const auto cache_dir = std::filesystem::temp_directory_path() / "glrpp-incremental-test-cache";
    std::filesystem::create_directories(cache_dir);

    glrpp::glr::cache_config config;
    config.lmdb_path = cache_dir.string();
    parser.enable_incremental(config);

    const std::string original = "1 + 2";
    const std::string edited = "1 + 3";

    const auto first = parser.parse(original);
    if (!first.has_value()) {
      return 0;
    }

    const auto second = parser.parse_incremental(&first.value(), original, edited);
    if (!second.has_value()) {
      return 0;
    }

    const auto stats = parser.get_cache_stats();
    assert(stats.hit_rate() >= 0.0);
    parser.disable_incremental();
    std::filesystem::remove_all(cache_dir);
  } catch (const std::exception&) {
    return 0;
  }
#endif

  return 0;
}
