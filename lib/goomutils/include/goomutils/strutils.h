#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_STRUTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_STRUTILS_H_

#include <string>
#include <vector>

namespace goom::utils
{

std::vector<std::string> splitString(const std::string& str, const std::string& delim);

} // namespace goom::utils
#endif
