#include "../dslutil.hpp"
#include <iostream>

int main() {
  auto square = dsl::memoize<int,int>([](int x){ return x * x; });
  std::cout << square(9) << "\n";
}
