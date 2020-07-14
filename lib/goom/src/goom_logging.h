#ifndef _GOOM_LOGGING_H
#define _GOOM_LOGGING_H
#include <stdarg.h>

extern void (*goom_logger)(int lvl, int line_num, const char* func_name, const char* msg);
extern int goom_log_get_level();
extern void goom_log_set_level(int lvl);
extern void goom_log(int lvl, int line_num, const char* func_name, const char* fmt, ...);

#define GOOM_LOG_LVL_DEBUG 1
#define GOOM_LOG_LVL_INFO 2
#define GOOM_LOG_LVL_WARN 3
#define GOOM_LOG_LVL_ERROR 4

#ifndef DO_GOOM_LOGGING
  #pragma message ("Compiling " __FILE__ " with 'DO_GOOM_LOGGING' OFF.")
  #define GOOM_LOG_DEBUG(...)
  #define GOOM_LOG_INFO(...)
  #define GOOM_LOG_WARN(...)
  #define GOOM_LOG_ERROR(...)
#else
  #pragma message ("Compiling " __FILE__ " with 'DO_GOOM_LOGGING' ON.")
  #define GOOM_LOG_DEBUG(...) goom_log(GOOM_LOG_LVL_DEBUG, __LINE__, __func__, __VA_ARGS__)
  #define GOOM_LOG_INFO(...) goom_log(GOOM_LOG_LVL_INFO, __LINE__, __func__, __VA_ARGS__)
  #define GOOM_LOG_WARN(...) goom_log(GOOM_LOG_LVL_WARN, __LINE__, __func__, __VA_ARGS__)
  #define GOOM_LOG_ERROR(...) goom_log(GOOM_LOG_LVL_ERROR, __LINE__, __func__, __VA_ARGS__)
#endif

#endif
