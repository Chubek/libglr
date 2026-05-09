#include "../dslutil.hpp"
#include <iostream>

static dsl::Result<int,std::string> parse_positive(int v){
  if(v > 0) return dsl::Result<int,std::string>(v);
  return dsl::Result<int,std::string>(std::string("non-positive"));
}

int main(){
  auto r = parse_positive(7).map([](int x){ return x + 1; });
  std::cout << r.unwrap_or(0) << "\n";
}
