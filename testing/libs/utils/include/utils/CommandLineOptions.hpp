#ifndef LIBS_UTILS_INCLUDE_UTILS_COMMAND_LINE_OPTIONS_H_
  #define LIBS_UTILS_INCLUDE_UTILS_COMMAND_LINE_OPTIONS_H_

  #include <string>
  #include <vector>
  #include <map>
  #include <stdexcept>

  class CommandLineOptions 
  {
    public:
      struct OptionDef { 
        char shortName; 
        std::string longName; 
        bool hasArgs; 
        std::string defaultVal; 
      }; 
      CommandLineOptions(const int argc, const char *argv[],
			const std::vector<CommandLineOptions::OptionDef> &optDefs);
      CommandLineOptions(const std::string& commandLine,
    		const std::vector<CommandLineOptions::OptionDef>& optDefs);
      bool getAreInvalidOptions() const;
      const std::vector<std::string>& getNames() const;
      bool isSet(const std::string& name) const;
      std::string getValue(const std::string& name) const;
      const std::vector<std::string>& getNonOptionArgs() const;
    private:  
      const std::vector<OptionDef> optionDefs;
      std::vector<std::string> optionNames;
      std::vector<std::string> nonOptionArgs;
      std::map<std::string, std::string> options;
      bool areInvalidOptions;
      bool isValidName(const std::string& name) const;
      const OptionDef& getOptionDef(const std::string& name) const;
      void Init(int argc, const char* argv[]);
    private:
      static const int MaxArgs = 100;
      static void getArgv(char* commandLine, int& argc, const char* argv[MaxArgs]);
  };


  inline const std::vector<std::string>& CommandLineOptions::getNames() const
  {
    return optionNames; 
  }

  inline bool CommandLineOptions::isSet(const std::string& name) const
  {
    if (!isValidName(name)) {
      throw std::logic_error("Invalid option name '" + name + "'.");
    }      
    return options.find(name) != options.end();
  }

  inline bool CommandLineOptions::getAreInvalidOptions() const 
  {
    return areInvalidOptions;
  }

inline const std::vector<std::string>& CommandLineOptions::getNonOptionArgs() const
  {
    return nonOptionArgs; 
  }

#endif
