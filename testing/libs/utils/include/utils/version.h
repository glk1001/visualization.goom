#ifndef LIBS_UTILS_INCLUDE_UTILS_VERSION_H_
#define LIBS_UTILS_INCLUDE_UTILS_VERSION_H_

#include "fmt/format.h"
#include <string>

class VersionInfo {
  public:
    explicit VersionInfo(const std::string& verNumber, const std::string& nm, const std::string& desc);
    std::string getVersionNumber() const;
    std::string getName() const;
    std::string getDescription() const;
    std::string getFullInfo() const;
  private:
    std::string versionNumber;
    std::string name;
    std::string description;
};

inline VersionInfo::VersionInfo(const std::string& verNumber, const std::string& nm, const std::string& desc)
: versionNumber(verNumber),
  name(nm),
  description(desc)
{
}

inline std::string VersionInfo::getVersionNumber() const
{
  return versionNumber;
}

inline std::string VersionInfo::getName() const
{
  return name;
}

inline std::string VersionInfo::getDescription() const
{
  return description;
}

inline std::string VersionInfo::getFullInfo() const
{
  return fmt::format("{} {}, {}", name, versionNumber, description);
}

#endif /* LIBS_UTILS_INCLUDE_UTILS_VERSION_H_ */
