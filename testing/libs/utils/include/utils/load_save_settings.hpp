#ifndef LIBS_UTILS_INCLUDE_UTILS_LOAD_SAVE_SETTINGS_H_
#define LIBS_UTILS_INCLUDE_UTILS_LOAD_SAVE_SETTINGS_H_

#include <string>
#include <vector>
#include <map>
#include <istream>

class SimpleSettings 
{
  public:
    SimpleSettings();
    void Parse(std::istream& configLines);
    const std::vector<std::string>& getNames() const;
    bool isSet(const std::string& name) const;
    std::string getValue(const std::string& name) const;
  private:
    std::vector<std::string> names;
    std::map<std::string, std::string> settings;
    bool isValidName(const std::string& name) const;
};

#endif
