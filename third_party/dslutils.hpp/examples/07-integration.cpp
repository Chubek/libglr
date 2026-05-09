#include "../dslutil.hpp"
#include <iostream>

int main(){
  dsl::Lazy<dsl::Maybe<int>> lazy([]{ return dsl::Maybe<int>(8); });
  std::cout << lazy.get().map([](int x){ return x + 2; }).or_else(0) << "\n";
}
