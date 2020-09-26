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

#include <cstddef>
#include <string.h>

/* TODO: Ajouter goom_ devant ces fonctions */

static void empty_fct(PluginParam*)
{
}

PluginParam goom_secure_param()
{
  PluginParam p;
  p.changed = empty_fct;
  p.change_listener = empty_fct;
  p.user_data = 0;
  p.name = p.desc = 0;
  p.rw = 1;
  return p;
}

PluginParam goom_secure_f_param(const char* name)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::floatVal;
  FVAL(p) = 0.5f;
  FMIN(p) = 0.0f;
  FMAX(p) = 1.0f;
  FSTEP(p) = 0.01f;
  return p;
}

PluginParam goom_secure_f_feedback(const char* name)
{
  PluginParam p = secure_f_param(name);
  p.rw = 0;
  return p;
}

PluginParam goom_secure_s_param(const char* name)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::strVal;
  SVAL(p) = 0;
  return p;
}

PluginParam goom_secure_b_param(const char* name, int value)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::boolVal;
  BVAL(p) = value;
  return p;
}

PluginParam goom_secure_i_param(const char* name)
{
  PluginParam p = secure_param();
  p.name = name;
  p.type = ParamType::intVal;
  IVAL(p) = 50;
  IMIN(p) = 0;
  IMAX(p) = 100;
  ISTEP(p) = 1;
  return p;
}

PluginParam goom_secure_i_feedback(const char* name)
{
  PluginParam p = secure_i_param(name);
  p.rw = 0;
  return p;
}

PluginParameters goom_plugin_parameters(const char* name, unsigned int nb)
{
  PluginParameters p;
  p.name = name;
  p.desc = "";
  p.nbParams = nb;
  p.params = (PluginParam**)malloc(nb * sizeof(PluginParam*));
  return p;
}

void goom_set_str_param_value(PluginParam* p, const char* str)
{
  const size_t len = strlen(str);
  if (SVAL(*p))
  {
    SVAL(*p) = (char*)realloc(SVAL(*p), len + 1);
  }
  else
  {
    SVAL(*p) = (char*)malloc(len + 1);
  }
  memcpy(SVAL(*p), str, len + 1);
}