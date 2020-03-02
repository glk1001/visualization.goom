#include "utils/goom_loggers.hpp"
extern "C" {
  #include "goom/goom_logging.h"
}
#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <ostream>
#include <fstream>


std::unique_ptr<GoomLogger> GoomLogger::logger(new GoomLogger());

void do_goom_log(const int lvl, const int line_num, const char* func_name, const char* msg)
{
  GoomLogger::getLogger().log(static_cast<GoomLogger::LogLevel>(lvl), line_num, func_name, msg);
}

void GoomLogger::log(const LogLevel lvl,
  const int line_num, const std::string &func_name, const std::string &msg)
{
  const std::lock_guard<std::mutex> lock { mutex };
  if (!doLogging) {
    return;
  }

  const std::string logMsg = func_name + ":" + std::to_string(line_num) + ":" + msg;

  if (lvl >= cutoffFileLogLevel) {
    logEntries.push_back(logMsg);
  }
  if (lvl >= cutoffHandlersLogLevel) {
    for (const auto h : handlers) {
      h(logMsg);
    }
  }
}

void GoomLogger::start()
{
  const std::lock_guard<std::mutex> lock { mutex };
  goom_logger = do_goom_log;
  doLogging = true;
  logEntries.clear();
};

void GoomLogger::stop()
{
  const std::lock_guard<std::mutex> lock { mutex };
  doLogging = false;
  doFlush();
  goom_logger = nullptr;
};

void GoomLogger::doFlush()
{
  if (logFile == "") {
    return;
  }
  if (logEntries.empty()) {
    return;
  }

  std::ofstream f(logFile, std::ios::out | std::ios::app);
  if (!f.good()) {
    throw std::runtime_error("Could not open log file \"" + logFile + "\".");
  }
  for (const auto entry : logEntries) {
    f << entry << std::endl;
  }

  logEntries.clear();
}
