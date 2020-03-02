// Compile with
//
// g++ -I.. -std=c++2a -Wall -Weffc++ -Wextra -Wsign-conversion test_fmt.cpp ../fmt_src/format.cc -o test_fmt 

#include <string>
#include <iostream>
#include <fmt/format.h>
 
int main()
{
  std::cout << "__cplusplus = '" << __cplusplus << "'" <<  std::endl;
  std::cout <<  std::endl;

  fmt::print("Hello, {}!\n", "world");  // Python-like format string syntax

  std::string str = fmt::format("Hello, {} from string!", std::string("world"));
  std::cout << str << std::endl;

  long longVal = 10003434343434;
  int intVal = 345;
  float floatVal = 234.567;
  str = fmt::format("Long {}, int {}, float {}", longVal, intVal, floatVal);
  std::cout << str << std::endl;
  str = fmt::format("Long '{:020}', int '{:4}', float '{:10}'", longVal, intVal, floatVal);
  std::cout << str << std::endl;
}
