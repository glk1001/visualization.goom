#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_STRUTILS_H_
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_STRUTILS_H_

#include <magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace GOOM::UTILS
{

auto SplitString(const std::string& str, const std::string& delim) -> std::vector<std::string>;

template<class E>
auto EnumToString(const E e) -> std::string
{
  return std::string(magic_enum::enum_name(e));
}

template<class E>
auto StringToEnum(const std::string& eStr) -> E
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
