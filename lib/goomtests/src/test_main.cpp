#define CATCH_CONFIG_RUNNER
//#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file

#undef NO_LOGGING
#include "catch2/catch.hpp"
#include "logging.h"

#include <iostream>
#include <ostream>
#include <string>

using GOOM::UTILS::Logging;

int main(int argc, char* argv[])
{
  // global setup...
  const auto f_console_log = [](Logging::LogLevel, const std::string& s) {
    std::clog << s << std::endl;
  };
  addLogHandler("console-log", f_console_log);
  setLogLevel(Logging::LogLevel::info);
  setLogLevelForFiles(Logging::LogLevel::info);
  logStart();
  logInfo("Start unit tests...");

  int result = Catch::Session().run(argc, argv);

  // global clean-up...

  return result;
}
