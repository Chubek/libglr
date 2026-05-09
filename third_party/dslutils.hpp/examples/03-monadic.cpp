#include "../dslutil.hpp"
#include <iostream>

int main() {
  dsl::Maybe<int> m(5);
  auto out = m.map([](int x){ return x * 3; }).filter([](int x){ return x > 10; }).or_else(0);
  std::cout << out << "\n";
}
