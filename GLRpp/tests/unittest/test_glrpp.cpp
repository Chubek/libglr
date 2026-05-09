#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include "../../../GLRpp/GLRpp.hpp"

#include <string>
#include <vector>

static void test_noop_deleter(void *) {}

TEST_CASE("resource handle default") {
  glrpp::detail::ResourceHandle<void *, test_noop_deleter> h;
  REQUIRE(h.get() == nullptr);
}

TEST_CASE("symbol terminal metadata") {
  glrpp::Symbol s(7, "tok", true);
  REQUIRE(s.id() == 7);
  REQUIRE(s.name() == "tok");
  REQUIRE(s.is_terminal());
}

TEST_CASE("symbol nonterminal metadata") {
  glrpp::Symbol s(9, "expr", false);
  REQUIRE(s.id() == 9);
  REQUIRE(s.name() == "expr");
  REQUIRE_FALSE(s.is_terminal());
}

TEST_CASE("production id is stable") {
  glrpp::Production p(11);
  REQUIRE(p.id() == 11);
}

TEST_CASE("grammar add symbols") {
  glrpp::Grammar g;
  auto t = g.terminal("num");
  auto nt = g.nonterminal("expr");
  REQUIRE(t.id() >= 0);
  REQUIRE(nt.id() >= 0);
  REQUIRE(t.is_terminal());
  REQUIRE_FALSE(nt.is_terminal());
}

TEST_CASE("grammar symbol interning") {
  glrpp::Grammar g;
  auto a = g.terminal("plus");
  auto b = g.terminal("plus");
  REQUIRE(a.id() == b.id());
}

TEST_CASE("grammar can add production") {
  glrpp::Grammar g;
  auto e = g.nonterminal("E");
  auto n = g.terminal("n");
  auto p = g.add_production(e, {n});
  REQUIRE(p.id() >= 0);
}

TEST_CASE("production builder accumulates rhs") {
  glrpp::Grammar g;
  auto e = g.nonterminal("E");
  auto n = g.terminal("n");
  auto plus = g.terminal("+");
  auto p = glrpp::ProductionBuilder(g, e).operator>>(e).operator>>(plus).operator>>(n).build();
  REQUIRE(p.id() >= 0);
}

struct MiniDSL : glrpp::GrammarDSL<MiniDSL> { glrpp::Grammar grammar_; };

TEST_CASE("grammar dsl forwards terminal") {
  MiniDSL d;
  auto s = d.terminal("id");
  REQUIRE(s.id() >= 0);
  REQUIRE(s.name() == "id");
}

TEST_CASE("grammar dsl forwards rule") {
  MiniDSL d;
  auto lhs = d.nonterminal("S");
  auto tok = d.terminal("tok");
  auto p = d.rule(lhs).operator>>(tok).build();
  REQUIRE(p.id() >= 0);
}

TEST_CASE("disambiguation builder prefer builds hook") {
  auto hook = glrpp::DisambiguationBuilder{}
                  .when([](const glrpp::DisambiguationContext &) { return true; })
                  .prefer(0)
                  .build("t", 1);
  REQUIRE(true);
  (void)hook;
}

TEST_CASE("grammar start symbol set") {
  glrpp::Grammar g;
  auto s = g.nonterminal("S");
  REQUIRE_NOTHROW(g.set_start(s));
}
