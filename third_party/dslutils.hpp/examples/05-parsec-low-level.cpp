#include "../dslutil.hpp"
#include <iostream>

int main(){
  dsl::ParsecInput in{"abc",0};
  auto p = dsl::parser([](dsl::ParsecInput& i)->dsl::ExpectedResult<char>{
    if(i.peek()=='a') return i.consume();
    return std::nullopt;
  });
  auto r = p(in);
  std::cout << (r ? *r : '?') << "\n";
}
