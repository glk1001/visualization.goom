#ifndef _CONFIG_PARAM_H
#define _CONFIG_PARAM_H

/**
 * File created on 2003-05-24 by Jeko.
 * (c)2003, JC Hoelt for iOS-software.
 *
 * LGPL Licence.
 */

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <vector>

namespace goom
{

enum class ParamType
{
  intVal,
  floatVal,
  boolVal,
};

struct PluginParam
{
  std::string name = "";
  std::string desc = "";

  // TODO Use a std::variant here.
  ParamType type;
  int ival = 0;
  float fval = 0;
  bool bval = 0;

  template<class Archive>
  void serialize(Archive&);

  // used by the core to inform the GUI of a change
  void (*change_listener)(PluginParam* _this);

  // used by the GUI to inform the core of a change
  void (*changed)(PluginParam* _this);

  // can be used by the GUI
  void* user_data;
};

template<class Archive>
void PluginParam::serialize(Archive& ar)
{
  ar(name, desc, type, ival, fval, bval);
}

PluginParam secure_param();

PluginParam secure_f_param(const char* name);
PluginParam secure_i_param(const char* name);
PluginParam secure_b_param(const char* name, int value);

PluginParam secure_f_feedback(const char* name);
PluginParam secure_i_feedback(const char* name);

struct PluginParameters
{
  std::string name = "";
  std::string desc = "";
  std::vector<PluginParam*> params{};

  template<class Archive>
  void serialize(Archive&);
};

template<class Archive>
void PluginParameters::serialize(Archive& ar)
{
  ar(name, desc, params);
}

} // namespace goom
#endif
