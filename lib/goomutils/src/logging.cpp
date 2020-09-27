#include "goomutils/logging.h"

#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace goom::utils
{

std::unique_ptr<Logging> Logging::logger(new Logging());

Logging::Logging() noexcept
{
  setFileLogLevel(cutoffFileLogLevel);
  setHandlersLogLevel(cutoffHandlersLogLevel);
}

Logging::~Logging()
{
  doFlush();
}

void Logging::log(const LogLevel lvl,
                  const int line_num,
                  const std::string& func_name,
                  const std::string& msg)
{
  const std::lock_guard<std::mutex> lock{mutex};
  if (!doLogging)
  {
    return;
  }

  const std::string logMsg = func_name + ":" + std::to_string(line_num) + ":" + msg;

  if (lvl >= cutoffFileLogLevel)
  {
    logEntries.push_back(logMsg);
  }
  if (lvl >= cutoffHandlersLogLevel)
  {
    for (const auto& [name, handlr] : handlers)
    {
      handlr(lvl, logMsg);
    }
  }
}

void Logging::start()
{
  const std::lock_guard<std::mutex> lock{mutex};
  doLogging = true;
  logEntries.clear();
};

void Logging::stop()
{
  const std::lock_guard<std::mutex> lock{mutex};
  doLogging = false;
  doFlush();
};

void Logging::setFileLogLevel(const Logging::LogLevel lvl)
{
  cutoffFileLogLevel = lvl;
}

void Logging::setHandlersLogLevel(const LogLevel lvl)
{
  cutoffHandlersLogLevel = lvl;
}

void Logging::addHandler(const std::string& name, const HandlerFunc& f)
{
  for (const auto& [hname, handlr] : handlers)
  {
    if (hname == name)
    {
      return;
    }
  }
  handlers.push_back(std::make_pair(name, f));
}

void Logging::flush()
{
  const std::lock_guard<std::mutex> lock{mutex};
  doFlush();
}

void Logging::doFlush()
{
  if (logFile == "")
  {
    return;
  }
  if (logEntries.empty())
  {
    return;
  }

  std::ofstream f(logFile, std::ios::out | std::ios::app);
  if (!f.good())
  {
    throw std::runtime_error("Could not open log file \"" + logFile + "\".");
  }
  for (const auto entry : logEntries)
  {
    f << entry << std::endl;
  }

  logEntries.clear();
}

} // namespace goom::utils
