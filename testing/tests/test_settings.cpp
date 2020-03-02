// g++ -std=c++2a -Wall -Weffc++ -Wextra -Wsign-conversion test_settings.cpp load_save_settings.cpp test_utils.cpp fmt_src/format.cc -o test_settings

#include "utils/load_save_settings.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <ostream>
#include <fstream>
#include <stdexcept>


int main (int argc, char **argv)
{
    const std::string configFilename = "test_settings.conf";
    std::ifstream configFile(configFilename, std::ios::in);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open settings file \"" + configFilename + "\".");
    }

    std::cout << "Load settings from file \"" << configFilename << "\"." << std::endl;
    std::cout << std::endl;
	SimpleSettings settings	{ };
    settings.Parse(configFile);

    const std::vector<std::string> settingNames = settings.getNames();
    std::cout << "Print the " << settingNames.size() << " settings with values:" << std::endl;
    for (const auto name : settingNames) {
        std::cout << "key = " << name << ": value = " << settings.getValue(name) << std::endl;
    }        
    std::cout << std::endl;

    std::cout << "Print the settings by name:" << std::endl;
    std::string name = "url";
    std::cout << "key = " << name << ": value = \"" << settings.getValue(name) << "\"" << std::endl;
    name = "main.file";
    std::cout << "key = " << name << ": value = \"" << settings.getValue(name) << "\"" << std::endl;
    name = "main.true";
    std::cout << "key = " << name << ": value = \"" << settings.getValue(name) << "\"" << std::endl;
    name = "throw.exception";
    std::cout << "key = " << name << ": value = \"" << settings.getValue(name) << "\"" << std::endl;
}
