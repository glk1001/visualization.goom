// Compile with
//
// g++ -std=c++2a -Wall -Weffc++ -Wextra -Wsign-conversion -pthread test_goom.cpp test_utils.cpp CommandLineOptions.cpp load_save_settings.cpp test_common.cpp fmt_src/format.cc -o test_goom -I ../lib/goom/src -lgoom -L ../build/visualization.goom-prefix/src/visualization.goom-build/lib/goom

#include "goom_tester.h"
#include "utils/goom_loggers.hpp"
#include "utils/CommandLineOptions.hpp"
#include "utils/version.h"
extern "C" {
  #include "goom/goom_testing.h"
}
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <ostream>


int main(int argc, const char* argv[])
{
  const VersionInfo versionInfo { "1.0", "Goom Tester", "Initial version"};

  const std::function<void(std::string)> f_console_log =
      [](const std:: string& s) { std::cout << s << std::endl; };
  addLogHandler(f_console_log);

  const std::vector<CommandLineOptions::OptionDef> optionDefs = {
    {'v', CommonTestOptions::VERSION, false, ""},
    {'i', CommonTestOptions::INPUT_DIR, true, ""},
    {'o', CommonTestOptions::OUTPUT_DIR, true, ""},
    {'n', CommonTestOptions::STATE_NUM, true, ""},
    {'r', CommonTestOptions::GOOM_SEED, true, CommonTestOptions::DEFAULT_GOOM_SEED},
    {'a', CommonTestOptions::AUDIO_FILE_PREFIX, true, CommonTestOptions::DEFAULT_AUDIO_FILE_PREFIX},
    {'g', CommonTestOptions::GOOM_FILE_PREFIX, true, CommonTestOptions::DEFAULT_GOOM_FILE_PREFIX},
    {'s', CommonTestOptions::START_BUFFER_NUM, true, CommonTestOptions::DEFAULT_START_BUFFER_NUM},
    {'e', CommonTestOptions::END_BUFFER_NUM, true, CommonTestOptions::DEFAULT_END_BUFFER_NUM}
  };
  const CommandLineOptions cmdLineOptions(argc, argv, optionDefs);
  if (cmdLineOptions.getAreInvalidOptions()) {
    throw std::runtime_error("Invalid command line options.");
  }

  if (cmdLineOptions.isSet(CommonTestOptions::VERSION)) {
    std::cout << versionInfo.getFullInfo() << std::endl;
    std::cout << getFullGoomVersionInfo() << std::endl;
    return 0;
  }

  std::cout << "Goom tester" << std::endl << std::endl;

  std::cout << "Setup options..." << std::endl;
  const GoomTestOptions options(cmdLineOptions);

  std::cout << "Check input directory OK..." << std::endl;
  checkFilePathExists(options.getInputDir());

  std::cout << "Make audio output directories." << std::endl;
  createFilePath(options.getAudioOutputDir());

  std::cout << "Make goom buffer output directories." << std::endl;
  createFilePath(options.getGoomBufferOutputDir());

  std::cout << "Make goom state output directories." << std::endl;
  createFilePath(options.getGoomStateOutputDir());

  std::cout << "Setup goom tester..." << std::endl;
  GoomTester tester { versionInfo, options };

  std::cout << "Start threads..." << std::endl;
  const std::function<void()> f_producer = [&tester]() { tester.AudioDataProducerThread(); };
  std::thread producer(f_producer);
  const std::function<void()> f_consumer = [&tester]() { tester.AudioDataConsumerThread(); };
  std::thread consumer(f_consumer);

  std::cout << "Joining threads..." << std::endl;
  consumer.join();
  producer.join();
}
