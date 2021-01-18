#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_LOGGING_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_LOGGING_H_

#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace GOOM::UTILS
{

class Logging
{
public:
  enum class LogLevel
  {
    debug,
    info,
    notice,
    warn,
    error
  };
  using HandlerFunc = std::function<void(const LogLevel, const std::string&)>;

  static Logging& getLogger();
  Logging(const Logging& rhs) = delete;
  ~Logging() noexcept;
  void setLogFile(const std::string& logF);
  void addHandler(const std::string& name, const HandlerFunc&);
  void start();
  void stop();
  void flush();
  void suspend();
  void resume();
  [[nodiscard]] bool isLogging() const;
  [[nodiscard]] LogLevel getFileLogLevel() const;
  void setFileLogLevel(LogLevel lvl);
  [[nodiscard]] LogLevel getHandlersLogLevel() const;
  void setHandlersLogLevel(LogLevel lvl);
  void log(LogLevel lvl, int line_num, const std::string& func_name, const std::string& msg);
  template<typename... Args>
  void log(LogLevel lvl,
           int line_num,
           const std::string& func_name,
           std::string_view format_str,
           const Args&... args);

private:
  Logging() noexcept;
  Logging& operator=(const Logging& rhs) = delete;
  LogLevel cutoffFileLogLevel = LogLevel::info;
  LogLevel cutoffHandlersLogLevel = LogLevel::info;
  bool doLogging = false;
  std::string logFile = "";
  std::vector<std::pair<std::string, HandlerFunc>> handlers{};
  std::vector<std::string> logEntries{};
  std::mutex mutex{};
  static std::unique_ptr<Logging> logger;
  void doFlush();
  void vlog(LogLevel lvl,
            int line_num,
            const std::string& func_name,
            std::string_view format_str,
            std20::format_args args);
};

inline Logging::LogLevel Logging::getFileLogLevel() const
{
  return cutoffFileLogLevel;
}

inline Logging::LogLevel Logging::getHandlersLogLevel() const
{
  return cutoffHandlersLogLevel;
}

inline void Logging::setLogFile(const std::string& logF)
{
  logFile = logF;
}

inline void Logging::suspend()
{
  doLogging = false;
}
inline void Logging::resume()
{
  doLogging = true;
}
inline bool Logging::isLogging() const
{
  return doLogging;
}
inline Logging& Logging::getLogger()
{
  return *logger;
}

template<typename... Args>
inline void Logging::log(const LogLevel lvl,
                         const int line_num,
                         const std::string& func_name,
                         std::string_view format_str,
                         const Args&... args)
{
  vlog(lvl, line_num, func_name, format_str, std20::make_format_args(args...));
}

inline void Logging::vlog(const LogLevel lvl,
                          const int line_num,
                          const std::string& func_name,
                          std::string_view format_str,
                          std20::format_args args)
{
  std20::memory_buffer buffer;
  // Pass custom argument formatter as a template arg to vwrite.
  std20::vformat_to(std20::detail::buffer_appender<char>(buffer), format_str, args);
  log(lvl, line_num, func_name, std::string(buffer.data(), buffer.size()));
}

#ifdef NO_LOGGING
#pragma message("Compiling " __FILE__ " with 'NO_LOGGING' ON.")
#define setLogFile(logF)
#define addLogHandler(name, h)
#define logStart()
#define logStop()
#define logFlush()
#define logSuspend()
#define logResume()
#define isLogging() false
#define getLogLevel()
#define setLogLevel(lvl)
#define logDebug(...)
#define logInfo(...)
#define logWarn(...)
#define logError(...)
#else
#pragma message("Compiling " __FILE__ " with 'NO_LOGGING' OFF.")
#define setLogFile(logF) Logging::getLogger().setLogFile(logF)
#define addLogHandler(name, h) Logging::getLogger().addHandler(name, h);
#define logStart() Logging::getLogger().start()
#define logStop() Logging::getLogger().stop()
#define logFlush() Logging::getLogger().flush()
#define logSuspend() Logging::getLogger().suspend()
#define logResume() Logging::getLogger().resume()
#define isLogging() Logging::getLogger().isLogging()
#define getLogLevel() Logging::getLogger().getHandlersLogLevel()
#define setLogLevel(lvl) Logging::getLogger().setHandlersLogLevel(lvl)
#define getLogLevelForFiles() Logging::getLogger().getFileLogLevel()
#define setLogLevelForFiles(lvl) Logging::getLogger().setFileLogLevel(lvl)
#define logDebug(...) \
  Logging::getLogger().log(Logging::LogLevel::debug, __LINE__, __func__, __VA_ARGS__)
#define logInfo(...) \
  Logging::getLogger().log(Logging::LogLevel::info, __LINE__, __func__, __VA_ARGS__)
#define logWarn(...) \
  Logging::getLogger().log(Logging::LogLevel::warn, __LINE__, __func__, __VA_ARGS__)
#define logError(...) \
  Logging::getLogger().log(Logging::LogLevel::error, __LINE__, __func__, __VA_ARGS__)
#endif

} // namespace GOOM::UTILS
#endif
