#include <glrpp/glrpp.hpp>

#include <cassert>

int main() {
  glrpp::dsl::ast_node number{"number", std::int64_t{42}};
  assert(!number.is_null());

  glrpp::dsl::ast_array items{number};
  glrpp::dsl::ast_node list{"list", items};
  assert(list.is_array());
  return 0;
}
