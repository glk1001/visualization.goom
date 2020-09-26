#ifndef _CONFIG_PARAM_H
#define _CONFIG_PARAM_H

/**
 * File created on 2003-05-24 by Jeko.
 * (c)2003, JC Hoelt for iOS-software.
 *
 * LGPL Licence.
 */

#include <cstddef>

enum class ParamType
{
  intVal,
  floatVal,
  boolVal,
  strVal,
};

struct IntVal
{
  int value;
  int min;
  int max;
  int step;
};
struct FloatVal
{
  float value;
  float min;
  float max;
  float step;
};
struct StrVal
{
  char* value;
};
struct BoolVal
{
  int value;
};

struct PluginParam
{
  const char* name;
  const char* desc;
  char rw;
  ParamType type;
  union
  {
    struct IntVal ival;
    struct FloatVal fval;
    struct StrVal sval;
    struct BoolVal bval;
  } param;

  // used by the core to inform the GUI of a change
  void (*change_listener)(PluginParam* _this);

  // used by the GUI to inform the core of a change
  void (*changed)(PluginParam* _this);

  // can be used by the GUI
  void* user_data;
};

#define IVAL(p) ((p).param.ival.value)
#define SVAL(p) ((p).param.sval.value)
#define FVAL(p) ((p).param.fval.value)
#define BVAL(p) ((p).param.bval.value)

#define FMIN(p) ((p).param.fval.min)
#define FMAX(p) ((p).param.fval.max)
#define FSTEP(p) ((p).param.fval.step)

#define IMIN(p) ((p).param.ival.min)
#define IMAX(p) ((p).param.ival.max)
#define ISTEP(p) ((p).param.ival.step)

PluginParam goom_secure_param(void);

PluginParam goom_secure_f_param(const char* name);
PluginParam goom_secure_i_param(const char* name);
PluginParam goom_secure_b_param(const char* name, int value);
PluginParam goom_secure_s_param(const char* name);

PluginParam goom_secure_f_feedback(const char* name);
PluginParam goom_secure_i_feedback(const char* name);

void goom_set_str_param_value(PluginParam* p, const char* str);

struct PluginParameters
{
  const char* name;
  const char* desc;
  size_t nbParams;
  PluginParam** params;
};

PluginParameters goom_plugin_parameters(const char* name, unsigned int nb);

#define secure_param goom_secure_param
#define secure_f_param goom_secure_f_param
#define secure_i_param goom_secure_i_param
#define secure_b_param goom_secure_b_param
#define secure_s_param goom_secure_s_param
#define secure_f_feedback goom_secure_f_feedback
#define secure_i_feedback goom_secure_i_feedback
#define set_str_param_value goom_set_str_param_value
#define plugin_parameters goom_plugin_parameters

#endif