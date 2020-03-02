#include "utils/load_save_settings.hpp"
#include "utils/test_utils.hpp"
#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include <stdexcept>

SimpleSettings::SimpleSettings() 
: names(),
  settings()
{
} 
    
const std::vector<std::string>& SimpleSettings::getNames() const
{
    return names;
}
    
bool SimpleSettings::isSet(const std::string& name) const
{
    return settings.find(name) != settings.end();
}

std::string SimpleSettings::getValue(const std::string& name) const
{
  if (!isValidName(name)) {
    throw std::logic_error("ERROR: Invalid option name '" + name + "'.");
  }      
  if (!isSet(name)) {
    throw std::logic_error("ERROR: Could not find setting '" + name + "'.");
  }
  return settings.at(name);
}

bool SimpleSettings::isValidName([[maybe_unused]] const std::string& name) const
{
  return true;
}

void SimpleSettings::Parse(std::istream& configLines) 
{
    names.clear();
    settings.clear();

    int lineNum = 0;
    std::string line;
    while(std::getline(configLines, line)) {
        lineNum++;
        line = trim(line);
        if (line == "" || line[0] == '#') {
            continue;
        }
        std::istringstream sline(line);
        std::string key;
        if (!std::getline(sline, key, '=')) {
            throw std::runtime_error("Settings line " + std::to_string(lineNum) + ": invalid.");
        }    
        key = trim(key);
        if (isSet(key)) {
            throw std::runtime_error("Settings line " + std::to_string(lineNum) + ": duplicate setting: '" + key + "'");
        }
        std::string value;
        if (std::getline(sline, value)) {
            value = trim(value);
            settings.insert({key, value});
            names.push_back(key);
        }   
    }
}
