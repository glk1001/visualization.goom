#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_STRUTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_STRUTILS_H_

#include <magic_enum.hpp>
#include <string>
#include <vector>

namespace goom::utils
{

std::vector<std::string> splitString(const std::string& str, const std::string& delim);

template<class E>
std::string enumToString(const E e)
{
  return std::string(magic_enum::enum_name(e));
}

} // namespace goom::utils
#endif
