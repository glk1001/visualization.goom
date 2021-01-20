#include "goomutils/strutils.h"

#include <ranges>
#include <string>
#include <vector>

namespace GOOM::UTILS
{

auto SplitString(const std::string& str, const std::string& delim) -> std::vector<std::string>
{
  auto parts = str | std::views::split(delim);
  std::vector<std::string> vec;
  for (auto part : parts)
  {
    std::string s = "";
    for (auto c : part)
    {
      s += c;
    }
    vec.emplace_back(s);
  }
  return vec;
}

} // namespace GOOM::UTILS
