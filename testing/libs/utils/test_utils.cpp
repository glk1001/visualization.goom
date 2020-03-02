#include "utils/test_utils.hpp"
#include "fmt/format.h"
#include <string>
#include <ctime>
#include <stdexcept>
#include <filesystem>

std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));    
}

std::string date_append(const std::string& str, const std::string& delim)
{
    const std::time_t t = std::time(nullptr);
    char mbstr[100];
    std::strftime(mbstr, sizeof(mbstr), "%F_%H-%M-%S", std::localtime(&t));
    return str + delim + std::string(mbstr);
}

void checkFilePathExists(const std::string& filePath)
{
  namespace fs = std::filesystem;

  const fs::path filePathToCheck(filePath);
  if (!fs::exists(filePathToCheck)) {
    throw std::runtime_error(fmt::format("Could not find file path \"{}\".", filePath));
  }
}

void createFilePath(const std::string& filePath)
{
  namespace fs = std::filesystem;

  const fs::path newFilePath(filePath);
  if (!newFilePath.is_absolute()) {
    throw std::runtime_error(fmt::format("Must specify an absolute directory for file path \"{}\".", filePath));
  }
  fs::create_directories(newFilePath);
  checkFilePathExists(filePath);
}
