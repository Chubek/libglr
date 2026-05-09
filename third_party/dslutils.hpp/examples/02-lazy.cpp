#include "../dslutil.hpp"
#include <iostream>

int main() {
  dsl::Lazy<int> value([]{ return 42; });
  std::cout << value.is_computed() << "\n";
  std::cout << value.get() << "\n";
}
