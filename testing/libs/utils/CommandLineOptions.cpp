#include "utils/CommandLineOptions.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>

std::string CommandLineOptions::getValue(const std::string& name) const
{
  if (!isValidName(name)) {
    throw std::logic_error("Invalid option name '" + name + "'.");
  }      
  if (!isSet(name)) {
    const OptionDef& optDef = getOptionDef(name);
    return optDef.hasArgs ? optDef.defaultVal : "false";
  }
  return options.at(name);
}

bool CommandLineOptions::isValidName(const std::string& name) const
{
  for (const std::string nm : optionNames) {
    if (nm == name) {
      return true;
    }
  }
  return false;
}

const CommandLineOptions::OptionDef& CommandLineOptions::getOptionDef(const std::string& name) const
{
  for (const OptionDef& optDef : optionDefs) {
    if (name == optDef.longName) {
      return optDef;
    }
  }
  throw std::logic_error("Invalid option name '" + name + "'.");
}

CommandLineOptions::CommandLineOptions(const int argc, const char *argv[],
		const std::vector<CommandLineOptions::OptionDef> &optDefs)
: optionDefs(optDefs),
  optionNames(),
  nonOptionArgs(),
  options(),
  areInvalidOptions { false }
{
  Init(argc, argv);
}

CommandLineOptions::CommandLineOptions(const std::string& commandLine, const std::vector<CommandLineOptions::OptionDef>& optDefs)
: optionDefs(optDefs),
  optionNames(),
  nonOptionArgs(),
  options(),
  areInvalidOptions { false }
{
  char cmdLine[commandLine.size()+1];
  commandLine.copy(cmdLine, commandLine.size());
  cmdLine[commandLine.size()] = '\0';

  int argc;
  const char* argv[MaxArgs];
  getArgv(cmdLine, argc, argv);

  Init(argc, argv);
}

void CommandLineOptions::getArgv(char* commandLine, int& argc, const char* argv[MaxArgs])
{
  argc = 0;
  char* p2 = strtok(commandLine, " ");
  while (p2 && argc < MaxArgs-1) {
    argv[argc++] = p2;
    p2 = strtok(0, " ");
  }
  argv[argc] = 0;
}

void CommandLineOptions::Init(int argc, const char* argv[])
{
  areInvalidOptions = false;

  // Get the 'getopt_long' short options.
  std::string short_options = ":";
  std::map<char, std::string> shortOptionMap;
  for (const auto opt : optionDefs) {
    if (shortOptionMap.find(opt.shortName) != shortOptionMap.end()) {
      std::string shortOpt = ""; 
      shortOpt += opt.shortName;
      throw std::logic_error("Duplicate short option '-" + shortOpt + "'.");
    }
    shortOptionMap.insert({opt.shortName, opt.longName});
    short_options += opt.shortName;
    if (opt.hasArgs) {
      short_options += ':';
    }
    optionNames.push_back(opt.longName);
  }

  // Get the 'getopt_long' long options.
  option long_options[optionDefs.size()+1];
  for (size_t i=0; i < optionDefs.size(); i++) {
    const OptionDef& optDef = optionDefs[i];
    const int has_arg = optDef.hasArgs ? required_argument : no_argument;
    long_options[i] = {optDef.longName.c_str(), has_arg, 0, optDef.shortName};
  }
  long_options[optionDefs.size()] = {0, 0, 0, 0};

  optind = 0;
  while (1) {
    int option_index = 0;

    char** pargv = const_cast<char**>(argv);
    const int c = getopt_long(argc, pargv, short_options.c_str(), long_options, &option_index);
    if (c == -1) {
      break; // no more options
    }

    if (c == 0) {
      areInvalidOptions = true;
      std::cerr << "Unexpected case '0' - option " << long_options[option_index].name;
      if (optarg)
        std::cerr << " with arg " << optarg;
      std::cerr << std::endl;
    } else if (c == ':') {
      areInvalidOptions = true;
      std::cerr << argv[0] << ": Option '" << argv[optind-1] << "' requires an argument." << std::endl;
    } else if (c == '?') {
      areInvalidOptions = true;
      std::cerr << argv[0] << ": Option '" << argv[optind-1] << "' is invalid: ignored." << std::endl;
    } else {
      const std::string longName = shortOptionMap[static_cast<char>(c)];
      if (options.find(longName) != options.end()) {
        std::cerr << argv[0] << ": Option '--" << longName << "' is duplicated: ignored." << std::endl;
      } else if (!optarg) {
        options.insert({longName, "true"});
      } else {
        options.insert({longName, optarg});
      }
    }
  }

  if (optind < argc) {
    for (int i=optind; i < argc; i++) {
      nonOptionArgs.push_back(argv[i]);
    }
  }
}
