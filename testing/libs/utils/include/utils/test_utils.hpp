#ifndef LIBS_UTILS_INCLUDE_UTILS_TEST_UTILS_H_
#define LIBS_UTILS_INCLUDE_UTILS_TEST_UTILS_H_

#include <string>
#include <cstring>

using cstyle_str = char*;
using const_cstyle_str = const char*;

std::string trim(const std::string& str);

std::string date_append(const std::string& str, const std::string& delim=".");

void checkFilePathExists(const std::string& filePath);
void createFilePath(const std::string& filePath);

#endif
