#include "utils/test_common.hpp"
#include "utils/test_utils.hpp"
#include "utils/load_save_settings.hpp"
#include "sstream_convert.hpp"
#include "fmt/format.h"
extern "C" {
  #include "goom/goom.h"
  #include "goom/goom_config.h"
  #include "goom/lines.h"
}
#include <string>
#include <vector>
#include <stdexcept>
#include <ostream>
#include <fstream>
#include <iostream>

static void SaveResolution(std::ostream& f, int width, int height);
static void LoadResolution(const SimpleSettings& settings, uint32_t& width, uint32_t& height);

static void SaveRandomNumberInfo(std::ostream& f, const GoomRandom* gRandom);
static GoomRandom* LoadRandomNumberInfo(const SimpleSettings& settings, GoomRandom* gRandom);

static void SaveSoundInfo(std::ostream& f, const SoundInfo& sound);
static void LoadSoundInfo(const SimpleSettings& settings, SoundInfo& sound);

static void SaveVisualFXStates(std::ostream& f, const PluginInfo* goomInfo);
static void LoadVisualFXStates(const SimpleSettings& settings, PluginInfo* goomInfo);

static void SaveVisualFXState(std::ostream& f, const PluginInfo* goomInfo, VisualFX* vfx);
static void LoadVisualFXState(const SimpleSettings& settings, PluginInfo* goomInfo, VisualFX* vfx);

static void SavePluginParameters(std::ostream& f, const std::string& paramParent, int nbParams, const PluginParameters params[]);
static void LoadPluginParameters(const SimpleSettings& settings, const std::string& paramParent, int nbParams, PluginParameters params[]);

static void SavePluginParameters(std::ostream& f, const std::string& paramParent, const PluginParameters& p);
static void LoadPluginParameters(const SimpleSettings& settings, const std::string& paramParent, PluginParameters& p);

static void SavePluginParam(std::ostream& f, const std::string& paramParent, const PluginParam& p);
static void LoadPluginParam(const SimpleSettings& settings, const std::string& paramParent, PluginParam& p);

static void SaveUpdate(std::ostream& f, const PluginInfo::GoomUpdate& update);
static void LoadUpdate(const SimpleSettings& settings, PluginInfo::GoomUpdate& update);

static void SaveLines(std::ostream& f, const GMLine* gmline1, const GMLine* gmline2);
static void LoadLines(const SimpleSettings& settings, GMLine *gmline1, GMLine *gmline2);

static void SaveState(std::ostream& f, guint32 cycle, const GoomState states[STATES_MAX_NB],
  int statesNumber, int statesRangeMax, int curGStateIndex);
static void LoadState(const SimpleSettings& settings, guint32& cycle, GoomState states[STATES_MAX_NB], 
  int& statesNumber, int& statesRangeMax, int& curGStateIndex);

static void SaveGoomState(std::ostream& f, const std::string& stateParent, const GoomState& goomState);
static void LoadGoomState(const SimpleSettings& settings, const std::string& stateParent, GoomState& goomState);

static void SaveZoomFilterData(std::ostream& f, const ZoomFilterData& data);
static void LoadZoomFilterData(const SimpleSettings& settings, ZoomFilterData& zoomFilterData);


template <typename T> void RestoreSetting(const SimpleSettings& settings, const std::string& name, T& settingValue) 
{
  const T newSettingValue = convert_to<T>(settings.getValue(name));
  if (newSettingValue != settingValue) {
    settingValue = newSettingValue;
  }
}

template <> void RestoreSetting(const SimpleSettings& settings, const std::string& name, char& settingValue) 
{
  int settingValueInt = int(settingValue);
  RestoreSetting(settings, name, settingValueInt);
  const int newSettingValue = convert_to<int>(settings.getValue(name));
  if (newSettingValue != settingValue) {
    settingValue = newSettingValue;
  }
}

template <typename T> void CheckSettingUnchanged(const SimpleSettings& settings, const std::string& name, const T& settingValue) 
{
  const T newSettingValue = convert_to<T>(settings.getValue(name));
  if (newSettingValue != settingValue) {
    const std::string settingValueStr = convert_from<T>(settingValue);
    throw std::runtime_error(
      fmt::format("Setting '{}' has changed: old '{}' != '{}'.", 
        name.c_str(), settingValueStr.c_str(), settings.getValue(name).c_str())
    );
  }
}

template <> void CheckSettingUnchanged(const SimpleSettings& settings, const std::string& name, const cstyle_str& settingValue) 
{
  const std::string settingValueStr = settingValue;
  CheckSettingUnchanged(settings, name, settingValueStr);
}

template <> void CheckSettingUnchanged(const SimpleSettings& settings, const std::string& name, const const_cstyle_str& settingValue) 
{
  const std::string settingValueStr = settingValue;
  CheckSettingUnchanged(settings, name, settingValueStr);
}


void SaveGoomPluginInfo(std::ostream& f, const PluginInfo* goomInfo)
{
  SaveResolution(f, goomInfo->screen.width, goomInfo->screen.height);
  SaveRandomNumberInfo(f, goomInfo->gRandom);
  SaveSoundInfo(f, goomInfo->sound);
  SaveVisualFXStates(f, goomInfo);
  SavePluginParameters(f, "main", goomInfo->nbParams, goomInfo->params);
  SaveState(f, goomInfo->cycle, goomInfo->states, goomInfo->statesNumber, goomInfo->statesRangeMax, goomInfo->curGStateIndex);
  SaveUpdate(f, goomInfo->update);
  SaveLines(f, goomInfo->gmline1, goomInfo->gmline2);
}

PluginInfo* LoadGoomPluginInfo(const std::string& filename, [[maybe_unused]] const PluginInfo* existingGoomInfo)
{
  std::ifstream settingsFile(filename, std::ios::in);
  if (!settingsFile.is_open()) {
    throw std::runtime_error("Could not open settings file \"" + filename + "\".");
  }
  SimpleSettings settings;
  settings.Parse(settingsFile);

  // Must set the pgc32 seed before initializing goomInfo.
  const uint64_t pcg32_seed = std::stoul(settings.getValue("pcg32_seed"));
  const uint64_t pcg32_initial_state = std::stoul(settings.getValue("pcg32_last_state"));
  
  uint32_t width;
  uint32_t height;
  LoadResolution(settings, width, height);

  PluginInfo* goomInfo = goom_init(width, height, pcg32_seed); // don't seed, use above pgc32 state

  pcg32_set_state(pcg32_initial_state);
  goomInfo->gRandom = LoadRandomNumberInfo(settings, goomInfo->gRandom);
  LoadSoundInfo(settings, goomInfo->sound);
  LoadVisualFXStates(settings, goomInfo);
  LoadPluginParameters(settings, "main", goomInfo->nbParams, goomInfo->params);
  LoadState(settings, goomInfo->cycle, goomInfo->states, goomInfo->statesNumber, goomInfo->statesRangeMax, goomInfo->curGStateIndex);
  goomInfo->curGState = &(goomInfo->states[goomInfo->curGStateIndex]);
  LoadUpdate(settings, goomInfo->update);
  LoadLines(settings, goomInfo->gmline1, goomInfo->gmline2);

  pcg32_set_state(pcg32_initial_state);

  return goomInfo;
}

static void SaveResolution(std::ostream& f, int width, int height)
{
  f << "# Resolution Begin" << std::endl;

  f << "width = " << width << std::endl;
  f << "height = " << height << std::endl;

  f << "# Resolution End" << std::endl;
  f << std::endl;
}

static void LoadResolution(const SimpleSettings& settings, uint32_t& width, uint32_t& height)
{
  RestoreSetting(settings, "width", width);
  RestoreSetting(settings, "height", height);
}

static void SaveRandomNumberInfo(std::ostream& f, const GoomRandom* gRandom)
{
  f << "# Random Numbers Begin" << std::endl;

  f << "pcg32_seed = " << pcg32_get_seed() << std::endl;
  f << "pcg32_last_state = " << pcg32_get_last_state() << std::endl;
  f << "gRandom.pos = " << gRandom->pos << std::endl;
  f << "gRandom.array[pos] = " << gRandom->array[gRandom->pos] << std::endl;

  f << "# Random Numbers End" << std::endl;
  f << std::endl;
}

static GoomRandom* LoadRandomNumberInfo(const SimpleSettings& settings, GoomRandom* gRandom)
{
//  goom_random_free(gRandom);
//  gRandom = goom_random_init();
  RestoreSetting(settings, "gRandom.pos", gRandom->pos);
  return gRandom;
}

static void SaveSoundInfo(std::ostream& f, const SoundInfo& sound)
{
  f << "# SoundInfo Begin" << std::endl;
  f << "sound.timeSinceLastGoom = " << sound.timeSinceLastGoom << std::endl;
  f << "sound.timeSinceLastBigGoom = " << sound.timeSinceLastBigGoom << std::endl;
  f << "sound.goomPower = " << sound.goomPower << std::endl; // PRECISION????
  f << "sound.volume = " << sound.volume << std::endl; // PRECISION????
//	gint16 samples[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN];

  f << "sound.goom_limit = " << sound.goom_limit << std::endl; // PRECISION????
  f << "sound.bigGoomLimit = " << sound.bigGoomLimit << std::endl; // PRECISION????
  f << "sound.accelvar = " << sound.accelvar << std::endl; // PRECISION????
  f << "sound.speedvar = " << sound.speedvar << std::endl; // PRECISION????
  f << "sound.allTimesMax = " << sound.allTimesMax << std::endl;
  f << "sound.totalgoom = " << sound.totalgoom << std::endl;

  f << "sound.prov_max = " << sound.prov_max << std::endl; // PRECISION????
  f << "sound.cycle = " << sound.cycle << std::endl;

  SavePluginParam(f, "sound.volume_p", sound.volume_p);
  SavePluginParam(f, "sound.speed_p", sound.speed_p);
  SavePluginParam(f, "sound.accel_p", sound.accel_p);
  SavePluginParam(f, "sound.goom_limit_p", sound.goom_limit_p);
  SavePluginParam(f, "sound.goom_power_p", sound.goom_power_p);
  SavePluginParam(f, "sound.last_goom_p", sound.last_goom_p);
  SavePluginParam(f, "sound.last_biggoom_p", sound.last_biggoom_p);
  SavePluginParam(f, "sound.biggoom_speed_limit_p", sound.biggoom_speed_limit_p);
  SavePluginParam(f, "sound.biggoom_factor_p", sound.biggoom_factor_p);

  SavePluginParameters(f, "sound", sound.params);

  f << "# SoundInfo End" << std::endl;
  f << std::endl;
}

static void LoadSoundInfo(const SimpleSettings& settings, SoundInfo& sound)
{
  std::cout << "LoadSoundInfo Begin" << std::endl;

  RestoreSetting(settings, "sound.timeSinceLastGoom", sound.timeSinceLastGoom);
  RestoreSetting(settings, "sound.timeSinceLastBigGoom", sound.timeSinceLastBigGoom);
  RestoreSetting(settings, "sound.goomPower", sound.goomPower);
  RestoreSetting(settings, "sound.volume", sound.volume);

  RestoreSetting(settings, "sound.goom_limit", sound.goom_limit);
  RestoreSetting(settings, "sound.bigGoomLimit", sound.bigGoomLimit);
  RestoreSetting(settings, "sound.accelvar", sound.accelvar);
  RestoreSetting(settings, "sound.speedvar", sound.speedvar);
  RestoreSetting(settings, "sound.allTimesMax", sound.allTimesMax);
  RestoreSetting(settings, "sound.totalgoom", sound.totalgoom);

  RestoreSetting(settings, "sound.prov_max", sound.prov_max);
  RestoreSetting(settings, "sound.cycle", sound.cycle);

  std::cout << "LoadSoundInfo End" << std::endl;
}

static void SaveVisualFXStates(std::ostream& f, const PluginInfo* goomInfo)
{
  f << "# VisualFXStates Begin" << std::endl;

  for (int i=0; i < goomInfo->nbVisuals; i++) {
    SaveVisualFXState(f, goomInfo, goomInfo->visuals[i]);
  }

  f << "# VisualFXStates End" << std::endl;
  f << std::endl;
}

static void LoadVisualFXStates(const SimpleSettings& settings, PluginInfo* goomInfo)
{
  std::cout << "LoadVisualFXStates Begin" << std::endl;

  for (int i=0; i < goomInfo->nbVisuals; i++) {
    LoadVisualFXState(settings, goomInfo, goomInfo->visuals[i]);
  }

  std::cout << "LoadVisualFXStates End" << std::endl;
}

static void SaveVisualFXState(std::ostream& f, const PluginInfo* goomInfo, VisualFX* vfx)
{
  const std::string vfxName = vfx->params->name;

  if (!vfx->save) {
    f << "# VisualFXStates: " << vfxName << " empty save" << std::endl;
    return;
  }

  f << "# VisualFXStates " << vfxName << " Begin" << std::endl;
  const std::string file("/tmp/vfx.tmp");
  vfx->save(vfx, goomInfo, file.c_str());
  std::ifstream tmp(file);
  if (!tmp.good()) {
    throw std::runtime_error("Could not open temp file \"" + file + "\".");
  }
  std::string line;
  while (std::getline(tmp, line)) {
    f << line << std::endl;
  }

  f << "# VisualFXStates " << vfxName << " End" << std::endl;
  f << std::endl;
}

static void LoadVisualFXState(const SimpleSettings& settings, PluginInfo* goomInfo, VisualFX* vfx)
{
  const std::string vfxName = vfx->params->name;

  if (!vfx->restore) {
    return;
  }

  const std::vector<std::string> settingNames = settings.getNames();
  const std::string file("/tmp/vfx_restore.tmp");
  std::ofstream tmp(file);
  int numVfxSettings = 0;
  for (const auto name : settingNames) {
    if (name.rfind(vfxName + ".", 0) == 0) {  
      tmp << name << " = " << settings.getValue(name) << std::endl;
      numVfxSettings++;
    }
  }  
  tmp.close();

  if (numVfxSettings == 0) {
    throw std::runtime_error(fmt::format("There were no settings for vfx '{}'.", vfxName));
  }

  vfx->restore(vfx, goomInfo, file.c_str());
}

static void SavePluginParameters(std::ostream& f, const std::string& paramParent, int nbParams, const PluginParameters params[])
{
  f << "# PluginParameters Begin" << std::endl;

  for (int i=0; i < nbParams; i++) {
    const std::string thisParamParent = paramParent + ".p_" + std::to_string(i);
    SavePluginParameters(f, thisParamParent, params[i]);
  }

  f << "# PluginParameters End" << std::endl;
  f << std::endl;
}

static void LoadPluginParameters(const SimpleSettings& settings, const std::string& paramParent, int nbParams, PluginParameters params[])
{
  std::cout << "LoadPluginParameters nbParams Begin" << std::endl;

  for (int i=0; i < nbParams; i++) {
    const std::string thisParamParent = paramParent + ".p_" + std::to_string(i);
    LoadPluginParameters(settings, thisParamParent, params[i]);
  }

  std::cout << "LoadPluginParameters nbParams End" << std::endl;
}

static void SaveState(std::ostream& f, guint32 cycle,
  const GoomState states[STATES_MAX_NB], int statesNumber, int statesRangeMax, int curGStateIndex)
{
  f << "# State Begin" << std::endl;

  f << "state.cycle = " << cycle << std::endl;
  f << "state.statesNumber = " << statesNumber << std::endl;
  f << "state.statesRangeMax = " << statesRangeMax << std::endl;
  for (int i= 0; i < STATES_MAX_NB; i++) {
    SaveGoomState(f, std::string("state_") + std::to_string(i), states[i]);
  }
  f << "curGStateIndex = " << curGStateIndex << std::endl;

  f << "# State End" << std::endl;
  f << std::endl;
} 

static void LoadState(const SimpleSettings& settings, guint32& cycle, GoomState states[STATES_MAX_NB], 
  int& statesNumber, int& statesRangeMax, int& curGStateIndex)
{
  std::cout << "LoadState Begin" << std::endl;

  RestoreSetting(settings, "state.cycle", cycle);
  RestoreSetting(settings, "state.statesNumber", statesNumber);
  RestoreSetting(settings, "state.statesRangeMax", statesRangeMax);
  for (int i= 0; i < STATES_MAX_NB; i++) {
    LoadGoomState(settings, std::string("state_") + std::to_string(i), states[i]);
  }
  RestoreSetting(settings, "curGStateIndex", curGStateIndex);

  std::cout << "LoadState End" << std::endl;
}  

static void SaveUpdate(std::ostream& f, const PluginInfo::GoomUpdate& update)
{
  f << "# Update Begin" << std::endl;

  f << "update.lockvar = " << update.lockvar << std::endl;
  f << "update.goomvar = " << update.goomvar << std::endl;
  f << "update.loopvar = " << update.loopvar << std::endl;
  f << "update.stop_lines = " << update.stop_lines << std::endl;
  f << "update.ifs_incr = " << update.ifs_incr << std::endl;
  f << "update.decay_ifs = " << update.decay_ifs << std::endl;
  f << "update.recay_ifs = " << update.recay_ifs << std::endl;
  f << "update.cyclesSinceLastChange = " << update.cyclesSinceLastChange << std::endl;
  f << "update.drawLinesDuration = " << update.drawLinesDuration << std::endl;
  f << "update.lineMode = " << update.lineMode << std::endl;
  f << "update.switchMultAmount = " << update.switchMultAmount << std::endl; // PRECISION????
  f << "update.switchIncrAmount = " << update.switchIncrAmount << std::endl;
  f << "update.switchMult = " << update.switchMult << std::endl; // PRECISION????
  f << "update.switchIncr = " << update.switchIncr << std::endl;
  f << "update.stateSelectionRnd = " << update.stateSelectionRnd << std::endl;
  f << "update.stateSelectionBlocker = " << update.stateSelectionBlocker << std::endl;
  f << "update.previousZoomSpeed = " << update.previousZoomSpeed << std::endl;
  f << "update.timeOfTitleDisplay = " << update.timeOfTitleDisplay << std::endl;
  f << "update.titleText = " << update.titleText << std::endl;
  SaveZoomFilterData(f, update.zoomFilterData);                

  f << "# Update End" << std::endl;
  f << std::endl;
}

static void LoadUpdate(const SimpleSettings& settings, PluginInfo::GoomUpdate& update)
{
  std::cout << "LoadUpdate Begin" << std::endl;

  RestoreSetting(settings, "update.lockvar", update.lockvar);
  RestoreSetting(settings, "update.goomvar", update.goomvar);
  RestoreSetting(settings, "update.loopvar", update.loopvar);
  RestoreSetting(settings, "update.stop_lines", update.stop_lines);
  RestoreSetting(settings, "update.ifs_incr", update.ifs_incr);
  RestoreSetting(settings, "update.decay_ifs", update.decay_ifs);
  RestoreSetting(settings, "update.recay_ifs", update.recay_ifs);
  RestoreSetting(settings, "update.cyclesSinceLastChange", update.cyclesSinceLastChange);
  RestoreSetting(settings, "update.drawLinesDuration", update.drawLinesDuration);
  RestoreSetting(settings, "update.lineMode", update.lineMode);
  RestoreSetting(settings, "update.switchMultAmount", update.switchMultAmount);
  RestoreSetting(settings, "update.switchIncrAmount", update.switchIncrAmount);
  RestoreSetting(settings, "update.switchMult", update.switchMult);
  RestoreSetting(settings, "update.switchIncr", update.switchIncr);
  RestoreSetting(settings, "update.stateSelectionRnd", update.stateSelectionRnd);
  RestoreSetting(settings, "update.stateSelectionBlocker", update.stateSelectionBlocker);
  RestoreSetting(settings, "update.previousZoomSpeed", update.previousZoomSpeed);
  RestoreSetting(settings, "update.timeOfTitleDisplay", update.timeOfTitleDisplay);
  std::string titleText;
  RestoreSetting(settings, "update.titleText", titleText);
  strcpy(update.titleText, titleText.c_str());
  LoadZoomFilterData(settings, update.zoomFilterData);                

  std::cout << "LoadUpdate End" << std::endl;
}

static void SaveLines(std::ostream& f, const GMLine* gmline1, const GMLine* gmline2)
{
  f << "# SaveLines Begin" << std::endl;

  f << "SaveLines.gmline1->IDdest = " << gmline1->IDdest << std::endl;
  f << "SaveLines.gmline2->IDdest = " << gmline2->IDdest << std::endl;
  f << "SaveLines.gmline1->param = " << gmline1->param << std::endl;
  f << "SaveLines.gmline2->param = " << gmline2->param << std::endl;
  f << "SaveLines.gmline1->amplitudeF = " << gmline1->amplitudeF << std::endl;
  f << "SaveLines.gmline2->amplitudeF = " << gmline2->amplitudeF << std::endl;
  f << "SaveLines.gmline1->amplitude = " << gmline1->amplitude << std::endl;
  f << "SaveLines.gmline2->amplitude = " << gmline2->amplitude << std::endl;
  f << "SaveLines.gmline1->nbPoints = " << gmline1->nbPoints << std::endl;
  f << "SaveLines.gmline2->nbPoints = " << gmline2->nbPoints << std::endl;
  f << "SaveLines.gmline1->color = " << gmline1->color << std::endl;
  f << "SaveLines.gmline2->color = " << gmline2->color << std::endl;
  f << "SaveLines.gmline1->color2 = " << gmline1->color2 << std::endl;
  f << "SaveLines.gmline2->color2 = " << gmline2->color2 << std::endl;
  f << "SaveLines.gmline1->screenX = " << gmline1->screenX << std::endl;
  f << "SaveLines.gmline2->screenX = " << gmline2->screenX << std::endl;
  f << "SaveLines.gmline1->screenY = " << gmline1->screenY << std::endl;
  f << "SaveLines.gmline2->screenY = " << gmline2->screenY << std::endl;
  f << "SaveLines.gmline1->power = " << gmline1->power << std::endl;
  f << "SaveLines.gmline2->power = " << gmline2->power << std::endl;
  f << "SaveLines.gmline1->powinc = " << gmline1->powinc << std::endl;
  f << "SaveLines.gmline2->powinc = " << gmline2->powinc << std::endl;

  f << "# SaveLines End" << std::endl;
  f << std::endl;
}  

static void LoadLines(const SimpleSettings& settings, GMLine *gmline1, GMLine *gmline2)
{
  std::cout << "LoadUpdate Begin" << std::endl;

  RestoreSetting(settings, "SaveLines.gmline1->IDdest", gmline1->IDdest);
  RestoreSetting(settings, "SaveLines.gmline2->IDdest", gmline2->IDdest);
  RestoreSetting(settings, "SaveLines.gmline1->param", gmline1->param);
  RestoreSetting(settings, "SaveLines.gmline2->param", gmline2->param);
  RestoreSetting(settings, "SaveLines.gmline1->amplitudeF", gmline1->amplitudeF);
  RestoreSetting(settings, "SaveLines.gmline2->amplitudeF", gmline2->amplitudeF);
  RestoreSetting(settings, "SaveLines.gmline1->amplitude", gmline1->amplitude);
  RestoreSetting(settings, "SaveLines.gmline2->amplitude", gmline2->amplitude);
  RestoreSetting(settings, "SaveLines.gmline1->nbPoints", gmline1->nbPoints);
  RestoreSetting(settings, "SaveLines.gmline2->nbPoints", gmline2->nbPoints);
  RestoreSetting(settings, "SaveLines.gmline1->color", gmline1->color);
  RestoreSetting(settings, "SaveLines.gmline2->color", gmline2->color);
  RestoreSetting(settings, "SaveLines.gmline1->color2", gmline1->color2);
  RestoreSetting(settings, "SaveLines.gmline2->color2", gmline2->color2);
  RestoreSetting(settings, "SaveLines.gmline1->screenX", gmline1->screenX);
  RestoreSetting(settings, "SaveLines.gmline2->screenX", gmline2->screenX);
  RestoreSetting(settings, "SaveLines.gmline1->screenY", gmline1->screenY);
  RestoreSetting(settings, "SaveLines.gmline2->screenY", gmline2->screenY);
  RestoreSetting(settings, "SaveLines.gmline1->power", gmline1->power);
  RestoreSetting(settings, "SaveLines.gmline2->power", gmline2->power);
  RestoreSetting(settings, "SaveLines.gmline1->powinc", gmline1->powinc);
  RestoreSetting(settings, "SaveLines.gmline2->powinc", gmline2->powinc);

  std::cout << "LoadUpdate End" << std::endl;
}

static void SavePluginParameters(std::ostream& f, const std::string& paramParent, const PluginParameters& p)
{
  std::string desc = "NULL";
  if (p.desc && trim(p.desc) != "") {
    desc = p.desc;
  }

  f << paramParent << ".name = " << p.name << std::endl;
  f << paramParent << ".desc = " << desc << std::endl;
  f << paramParent << ".nbParams = " << p.nbParams << std::endl;
  for (int i=0; i < p.nbParams; i++) {
    if (p.params[i]) {
      const std::string thisParamParent = paramParent + ".p_" + std::to_string(i);
      SavePluginParam(f, thisParamParent, *(p.params[i]));
    }
  }
}

static void LoadPluginParameters(const SimpleSettings& settings, const std::string& paramParent, PluginParameters& p)
{
  std::cout << "LoadPluginParameters Begin" << std::endl;
  std::cout << "paramParent = " << paramParent << std::endl;

  std::string desc = "NULL";
  if (p.desc && trim(p.desc) != "") {
    desc = p.desc;
  }

  CheckSettingUnchanged(settings, paramParent + ".name", p.name);
  CheckSettingUnchanged(settings, paramParent + ".desc", desc);
  CheckSettingUnchanged(settings, paramParent + ".nbParams", p.nbParams);
  for (int i=0; i < p.nbParams; i++) {
    if (p.params[i]) {
      const std::string thisParamParent = paramParent + ".p_" + std::to_string(i);
      LoadPluginParam(settings, thisParamParent, *(p.params[i]));
    }
  }

  std::cout << "LoadPluginParameters End" << std::endl;
}

static void SavePluginParam(std::ostream& f, const std::string& paramParent, const PluginParam& p)
{
  f << paramParent << ".name = " << p.name << std::endl;
  f << paramParent << ".desc = " << (p.desc ? p.desc : "NULL") << std::endl;
  f << paramParent << ".rw = " << int(p.rw) << std::endl;
  f << paramParent << ".type = " << p.type << std::endl;
  switch (p.type) {
    case PARAM_INTVAL:
      f << paramParent << ".ival.value = " << p.param.ival.value << std::endl;
      f << paramParent << ".ival.min = " << p.param.ival.min << std::endl;
      f << paramParent << ".ival.max = " << p.param.ival.max << std::endl;
      f << paramParent << ".ival.step = " << p.param.ival.step << std::endl;
      break;
    case PARAM_FLOATVAL:
      f << paramParent << ".fval.value = " << p.param.fval.value << std::endl;
      f << paramParent << ".fval.min = " << p.param.fval.min << std::endl;
      f << paramParent << ".fval.max = " << p.param.fval.max << std::endl;
      f << paramParent << ".fval.step = " << p.param.fval.step << std::endl;
      break;
    case PARAM_BOOLVAL:
      f << paramParent << ".bval.value = " << p.param.bval.value << std::endl;
      break;
    case PARAM_STRVAL:
      f << paramParent << ".sval.value = " << p.param.sval.value << std::endl;
      break;
    case PARAM_LISTVAL:
      f << paramParent << ".slist.value = " << p.param.slist.value << std::endl;
      f << paramParent << ".slist.nbChoices = " << p.param.slist.nbChoices << std::endl;
      for (int i=0; i < p.param.slist.nbChoices; i++) {
        f << "  " << paramParent << ".slist.choices_" << std::to_string(i) << " = " << p.param.slist.choices[i] << std::endl;
      }
      break;
    default:  
      break; // ERROR ???????????????????????
  }
}

static void LoadPluginParam(const SimpleSettings& settings, const std::string& paramParent, PluginParam& p)
{
  std::cout << "LoadPluginParam Begin" << std::endl;

  CheckSettingUnchanged(settings, paramParent + ".name", p.name);
  CheckSettingUnchanged(settings, paramParent + ".desc", p.desc ? p.desc : std::string("NULL"));
  CheckSettingUnchanged(settings, paramParent + ".rw", int(p.rw));
  CheckSettingUnchanged(settings, paramParent + ".type", int(p.type));

  switch (p.type) {
    case PARAM_INTVAL:
      RestoreSetting(settings, paramParent + ".ival.value", p.param.ival.value);
      RestoreSetting(settings, paramParent + ".ival.min", p.param.ival.min);
      RestoreSetting(settings, paramParent + ".ival.step", p.param.ival.step);
      RestoreSetting(settings, paramParent + ".ival.max", p.param.ival.max);
      break;
    case PARAM_FLOATVAL:
      RestoreSetting(settings, paramParent + ".fval.value", p.param.fval.value);
      RestoreSetting(settings, paramParent + ".fval.min", p.param.fval.min);
      RestoreSetting(settings, paramParent + ".fval.step", p.param.fval.step);
      RestoreSetting(settings, paramParent + ".fval.max", p.param.fval.max);
      break;
    case PARAM_BOOLVAL:
      RestoreSetting(settings, paramParent + ".bval.value", p.param.bval.value);
      break;
    case PARAM_STRVAL:
      CheckSettingUnchanged(settings, paramParent + ".sval.value", p.param.sval.value);
      break;
    case PARAM_LISTVAL:
      CheckSettingUnchanged(settings, paramParent + ".slist.value", p.param.slist.value);
      CheckSettingUnchanged(settings, paramParent + ".slist.nbChoices", p.param.slist.nbChoices);
      for (int i=0; i < p.param.slist.nbChoices; i++) {
        CheckSettingUnchanged(settings, paramParent + ".slist.choices_" + std::to_string(i), p.param.slist.choices[i]);
      }
      break;
    default:  
      break; // ERROR ???????????????????????
  }

  std::cout << "LoadPluginParam End" << std::endl;
}  

static void SaveGoomState(std::ostream& f, const std::string& stateParent, const GoomState& goomState)
{
  f << stateParent << ".drawIFS = " << int(goomState.drawIFS) << std::endl;
  f << stateParent << ".drawPoints = " << int(goomState.drawPoints) << std::endl;
  f << stateParent << ".drawTentacle = " << int(goomState.drawTentacle) << std::endl;
  f << stateParent << ".drawScope = " << int(goomState.drawScope) << std::endl;
  f << stateParent << ".farScope = " << goomState.farScope << std::endl;
  f << stateParent << ".rangemin = " << goomState.rangemin << std::endl;
  f << stateParent << ".rangemax = " << goomState.rangemax << std::endl;
}

static void LoadGoomState(const SimpleSettings& settings, const std::string& stateParent, GoomState& goomState)
{
  std::cout << "LoadGoomState Begin" << std::endl;

  RestoreSetting(settings, stateParent + ".drawIFS", goomState.drawIFS);
  RestoreSetting(settings, stateParent + ".drawPoints", goomState.drawPoints);
  RestoreSetting(settings, stateParent + ".drawTentacle", goomState.drawTentacle);
  RestoreSetting(settings, stateParent + ".drawScope", goomState.drawScope);
  RestoreSetting(settings, stateParent + ".farScope", goomState.farScope);
  RestoreSetting(settings, stateParent + ".rangemin", goomState.rangemin);
  RestoreSetting(settings, stateParent + ".rangemax", goomState.rangemax);

  std::cout << "LoadGoomState End" << std::endl;
}

static void SaveZoomFilterData(std::ostream& f, const ZoomFilterData& zoomFilterData)
{
  f << "zoomFilterData.vitesse = " << zoomFilterData.vitesse << std::endl;
  f << "zoomFilterData.pertedec = " << int(zoomFilterData.pertedec) << std::endl;
  f << "zoomFilterData.middleX = " << zoomFilterData.middleX << std::endl;
  f << "zoomFilterData.middleY = " << zoomFilterData.middleY << std::endl;
  f << "zoomFilterData.reverse = " << int(zoomFilterData.reverse) << std::endl;
  f << "zoomFilterData.mode = " << int(zoomFilterData.mode) << std::endl;
  f << "zoomFilterData.hPlaneEffect = " << zoomFilterData.hPlaneEffect << std::endl;
  f << "zoomFilterData.vPlaneEffect = " << zoomFilterData.vPlaneEffect << std::endl;
  f << "zoomFilterData.waveEffect = " << zoomFilterData.waveEffect << std::endl;
  f << "zoomFilterData.hypercosEffect = " << zoomFilterData.hypercosEffect << std::endl;
  f << "zoomFilterData.noisify = " << int(zoomFilterData.noisify) << std::endl;
}

static void LoadZoomFilterData(const SimpleSettings& settings, ZoomFilterData& zoomFilterData)
{
  std::cout << "LoadZoomFilterData Begin" << std::endl;

  RestoreSetting(settings, "zoomFilterData.vitesse", zoomFilterData.vitesse);
  RestoreSetting(settings, "zoomFilterData.pertedec", zoomFilterData.pertedec);
  RestoreSetting(settings, "zoomFilterData.middleX", zoomFilterData.middleX);
  RestoreSetting(settings, "zoomFilterData.middleY", zoomFilterData.middleY);
  RestoreSetting(settings, "zoomFilterData.reverse", zoomFilterData.reverse);
  RestoreSetting(settings, "zoomFilterData.mode", zoomFilterData.mode);
  RestoreSetting(settings, "zoomFilterData.hPlaneEffect", zoomFilterData.hPlaneEffect);
  RestoreSetting(settings, "zoomFilterData.vPlaneEffect", zoomFilterData.vPlaneEffect);
  RestoreSetting(settings, "zoomFilterData.waveEffect", zoomFilterData.waveEffect);
  RestoreSetting(settings, "zoomFilterData.hypercosEffect", zoomFilterData.hypercosEffect);
  RestoreSetting(settings, "zoomFilterData.noisify", zoomFilterData.noisify);

  std::cout << "LoadZoomFilterData End" << std::endl;
}
