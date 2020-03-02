#ifndef LIBS_UTILS_INCLUDE_UTILS_GOOM_LOGGERS_H_
#define LIBS_UTILS_INCLUDE_UTILS_GOOM_LOGGERS_H_

extern "C" {
  #include "goom/goom_logging.h"
}
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <ostream>
#include <fstream>

class GoomLogger {
  public:
    static GoomLogger& getLogger();
    ~GoomLogger();
    void setLogFile(const std::string& logF);
    void addHandler(const std::function<void(std::string)> f);
    void start();
    void stop();
    void flush();
    void suspend();
    void resume();
    bool isLogging() const;
    enum class LogLevel: int {
      debug = GOOM_LOG_LVL_DEBUG,
      info = GOOM_LOG_LVL_INFO,
      warn = GOOM_LOG_LVL_WARN,
      error = GOOM_LOG_LVL_ERROR
    };
    LogLevel getFileLogLevel() const;
    void setFileLogLevel(const LogLevel lvl);
    LogLevel getHandlersLogLevel() const;
    void setHandlersLogLevel(const LogLevel lvl);
    void log(const LogLevel lvl, const int line_num, const std::string &func_name, const std::string &msg);
  private:
    GoomLogger();
    LogLevel cutoffFileLogLevel = LogLevel::debug;
    LogLevel cutoffHandlersLogLevel = LogLevel::info;
    bool doLogging = false;
    std::string logFile = "";
    std::vector<std::function<void(std::string)>> handlers;
    std::vector<std::string> logEntries;
    std::mutex mutex;
    static std::unique_ptr<GoomLogger> logger;
    void doFlush();
};

extern void do_goom_log(const int lvl, const int line_num, const char* func_name, const char* msg);

inline GoomLogger::GoomLogger()
: handlers(),
  logEntries(),
  mutex()
{
  setFileLogLevel(cutoffFileLogLevel);
  setHandlersLogLevel(cutoffHandlersLogLevel);
}

inline GoomLogger::~GoomLogger()
{
  doFlush();
}

inline GoomLogger::LogLevel GoomLogger::getFileLogLevel() const
{
  return cutoffFileLogLevel;
}

inline void GoomLogger::setFileLogLevel(const GoomLogger::LogLevel lvl)
{
  cutoffFileLogLevel = lvl;
  goom_log_set_level(static_cast<int>(cutoffFileLogLevel));
}

inline GoomLogger::LogLevel GoomLogger::getHandlersLogLevel() const
{
  return cutoffHandlersLogLevel;
}

inline void GoomLogger::setHandlersLogLevel(const LogLevel lvl)
{
  cutoffHandlersLogLevel = lvl;
}

inline void GoomLogger::setLogFile(const std::string& logF)
{
  logFile = logF;
}

inline void GoomLogger::addHandler(const std::function<void(std::string)> f)
{
  handlers.push_back(f);
}

inline void GoomLogger::flush()
{
  const std::lock_guard<std::mutex> lock(mutex);
  doFlush();
}

inline void GoomLogger::suspend() { doLogging = false; }
inline void GoomLogger::resume() { doLogging = true; }
inline bool GoomLogger::isLogging() const { return doLogging; }
inline GoomLogger& GoomLogger::getLogger() { return *logger.get(); }

#ifdef NO_LOGGING
  #define setLogFile(logF)
  #define addLogHandler(h)
  #define logStart()
  #define logStop()
  #define logFlush()
  #define logSuspend()
  #define logResume()
  #define isLogging()
  #define getLogLevel()
  #define setLogLevel(lvl)
  #define logDebug(msg)
  #define logInfo(msg)
  #define logWarn(msg)
  #define logError(msg)
#else
  #define setLogFile(logF) GoomLogger::getLogger().setLogFile(logF)
  #define addLogHandler(h) GoomLogger::getLogger().addHandler(h);
  #define logStart() GoomLogger::getLogger().start()
  #define logStop() GoomLogger::getLogger().stop()
  #define logFlush() GoomLogger::getLogger().flush()
  #define logSuspend() GoomLogger::getLogger().suspend()
  #define logResume() GoomLogger::getLogger().resume()
  #define isLogging() GoomLogger::getLogger().isLogging()
  #define getLogLevel() GoomLogger::getLogger().getLevel()
  #define setLogLevel(lvl) GoomLogger::getLogger().setLevel(lvl)
  #define logDebug(msg) GoomLogger::getLogger().log(GoomLogger::LogLevel::debug, __LINE__, __func__, msg)
  #define logInfo(msg) GoomLogger::getLogger().log(GoomLogger::LogLevel::info, __LINE__, __func__, msg)
  #define logWarn(msg) GoomLogger::getLogger().log(GoomLogger::LogLevel::warn, __LINE__, __func__, msg)
  #define logError(msg) GoomLogger::getLogger().log(GoomLogger::LogLevel::error, __LINE__, __func__, msg)
#endif

#endif
