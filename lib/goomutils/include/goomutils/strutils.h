#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_STRUTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_STRUTILS_H_

#include <magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace GOOM::UTILS
{

std::vector<std::string> splitString(const std::string& str, const std::string& delim);

template<class E>
std::string enumToString(const E e)
{
  return std::string(magic_enum::enum_name(e));
}

template<class E>
E stringToEnum(const std::string& eStr)
{
  const auto val = magic_enum::enum_cast<E>(eStr);
  if (val)
  {
    return *val;
  }

  throw std::runtime_error("Unknown enum value \"" + eStr + "\".");
}

} // namespace GOOM::UTILS
#endif
