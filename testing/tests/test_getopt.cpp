// g++ -std=c++2a -Wall -Weffc++ -Wextra -Wsign-conversion test_getopt.cpp CommandLineOptions.cpp fmt_src/format.cc -o test_getopt

#include "utils/CommandLineOptions.hpp"
#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include <ostream>

void PrintOptions(const CommandLineOptions& options)
{
  std::cout << std::endl;
  for (auto name : options.getNames()) {
    std::cout << name << ": " << options.getValue(name);
    if (options.isSet(name)) {
     std::cout << " (set value)";
    } else {
     std::cout << " (default value)";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  // Print any remaining command line arguments (not options).
  if (!options.getNonOptionArgs().empty()) {
    std::cout << "Non-option ARGV-elements: ";
    for (auto arg : options.getNonOptionArgs()) {
      std::cout << arg << " ";
    }
    std::cout << std::endl;
  }
}

const std::vector<CommandLineOptions::OptionDef> optionDefs = {
//{shortName, longName,  hasArgs, defaultVal}
  {'a',       "add",     false,   ""},
  {'A',       "append",  false,   ""},
  {'c',       "create",  true,    "something"},
  {'d',       "delete",  true,    "beans"},
  {'f',       "file",    true,    "away"},
  {'v',       "verbose", false,   ""}
};

void TestFromCommandLine(int argc, const char* argv[])
{
  const CommandLineOptions options(argc, argv, optionDefs);
  if (options.getAreInvalidOptions()) {
    std::cout << "There were invalid options." << std::endl;
  }

  PrintOptions(options);
}

void TestFromString(const std::string& commandLine)
{
  const CommandLineOptions options(commandLine, optionDefs);
  if (options.getAreInvalidOptions()) {
    std::cout << "There were invalid options." << std::endl;
  }

  PrintOptions(options);
}  


int main(int argc, const char* argv[])
{
  std::cout << "Test from command line 1." << std::endl;
  TestFromCommandLine(argc, argv);
  std::cout << std::endl;

  std::cout << "Test from string 1." << std::endl;
  TestFromString("program -aA -c foo --verbose arg1 arg2 arg3 --file junk --hello");
  std::cout << std::endl;

  std::cout << "Test from command line 2." << std::endl;
  TestFromCommandLine(argc, argv);
  std::cout << std::endl;

  return 0;
}
