/*---------------------------------------------------------------------------*/
/*
 ** Goom Project
 **
 ** Created by Jean-Christophe Hoelt on Sat Jul 19 2003
 ** Copyright (c) 2003 iOS. All rights reserved.
 */
/*---------------------------------------------------------------------------*/

#include "goom_config_param.h"

#include "goomutils/logging_control.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"

namespace goom
{

static void empty_fct(PluginParam*)
{
}

PluginParam secure_param()
{
  PluginParam p;
  p.changed = empty_fct;
  p.change_listener = empty_fct;
  p.user_data = nullptr;
  p.name = "";
  p.desc = "";
  return p;
}

PluginParam secure_f_param(const char* name)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::floatVal;
  p.fval = 0.5f;
  return p;
}

PluginParam secure_f_feedback(const char* name)
{
  return secure_f_param(name);
}

PluginParam secure_b_param(const char* name, int value)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::boolVal;
  p.bval = value;
  return p;
}

PluginParam secure_i_param(const char* name)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::intVal;
  p.ival = 50;
  return p;
}

PluginParam secure_i_feedback(const char* name)
{
  return secure_i_param(name);
}

} // namespace goom
