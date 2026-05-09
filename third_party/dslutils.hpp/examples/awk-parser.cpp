#include "../dslutil.hpp"
#include <iostream>

int main(){
  dsl::ParsecInput in{"BEGIN { print 1 }",0};
  auto begin_kw = dsl::parser([](dsl::ParsecInput& i)->dsl::ExpectedResult<std::string>{
    std::size_t s = i.pos;
    for(char c: std::string("BEGIN")) if(i.consume()!=c){ i.pos = s; return std::nullopt; }
    return std::string(i.get_span(s));
  });
  auto r = begin_kw(in);
  std::cout << (r ? *r : std::string("fail")) << "\n";
}
