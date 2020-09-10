/**
* file: goom_core.c
 * author: Jean-Christophe Hoelt (which is not so proud of it)
 *
 * Contains the core of goom's work.
 *
 * (c)2000-2003, by iOS-software.
 */

#include "goom_core.h"

#include "colorutils.h"
#include "filters.h"
#include "gfontlib.h"
#include "goom_config.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/mathutils.h"
#include "ifs.h"
#include "lines.h"
#include "sound_tester.h"
#include "tentacle3d.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

// Return prob(m/n)
inline bool probabilityOfMInN(PluginInfo* goomInfo, const uint32_t m, const uint32_t n)
{
  if (m == 1)
  {
    return goomInfo->getNRand(n) == 0;
  }
  if (m == n - 1)
  {
    return goomInfo->getNRand(n) > 0;
  }
  return goomInfo->getRandInRange(0.0f, 1.0f) <= static_cast<float>(m) / static_cast<float>(n);
}

class GoomEvents
{
public:
  GoomEvents();

  void setGoomInfo(PluginInfo* goomInfo);

  enum class GoomEvent
  {
    changeFilterMode = 0,
    changeFilterFromNormalMode,
    changeState,
    changeNoiseState,
    changeToMegaLentMode,
    changeLineCircleAmplitude,
    changeLineCircleParams,
    changeHLineParams,
    changeVLineParams,
    hypercosEffectOnWithWaveMode,
    waveEffectOnWithWaveMode,
    changeVitesseWithWaveMode,
    waveEffectOnWithCrystalBallMode,
    hypercosEffectOnWithCrystalBallMode,
    hypercosEffectOnWithHyperCos1Mode,
    hypercosEffectOnWithHyperCos2Mode,
    filterReverseOffAndStopSpeed,
    filterReverseOn,
    filterVitesseStopSpeedMinus1,
    filterVitesseStopSpeedPlus1,
    filterZeroHPlaneEffect,
    filterChangeVitesseAndToggleReverse,
    reduceLineMode,
    updateLineMode,
    changeLineToBlack,
    changeGoomLine,
    _size // must be last - gives number of enums
  };

  enum GoomFilterEvent
  {
    waveModeWithHyperCosEffect,
    waveMode,
    crystalBallMode,
    crystalBallModeWithEffects,
    amuletteMode,
    waterMode,
    scrunchMode,
    scrunchModeWithEffects,
    hyperCos1Mode,
    hyperCos2Mode,
    yOnlyMode,
    speedwayMode,
    normalMode,
    _size // must be last - gives number of enums
  };

  bool happens(const GoomEvent) const;
  GoomFilterEvent getRandomFilterEvent() const;
  LineType getRandomLineTypeEvent() const;

private:
  PluginInfo* goomInfo = nullptr;

  static constexpr size_t numGoomEvents = static_cast<size_t>(GoomEvent::_size);
  static constexpr size_t numGoomFilterEvents = static_cast<size_t>(GoomFilterEvent::_size);

  struct WeightedEvent
  {
    GoomEvent event;
    // m out of n
    uint32_t m;
    uint32_t outOf;
  };
  // clang-format off
  // Note: With nomal filter mode, it's the least interesting mode so we
  //   change away from it with higher probability than other filter modes.
  static constexpr std::array<WeightedEvent, numGoomEvents> weightedEvents{ {
    { .event = GoomEvent::changeFilterMode,                    .m = 1, .outOf =  16 },
    { .event = GoomEvent::changeFilterFromNormalMode,          .m = 1, .outOf =   5 },
    { .event = GoomEvent::changeState,                         .m = 2, .outOf =   3 },
    { .event = GoomEvent::changeNoiseState,                    .m = 4, .outOf =   5 },
    { .event = GoomEvent::changeToMegaLentMode,                .m = 1, .outOf = 700 },
    { .event = GoomEvent::changeLineCircleAmplitude,           .m = 1, .outOf =   3 },
    { .event = GoomEvent::changeLineCircleParams,              .m = 1, .outOf =   2 },
    { .event = GoomEvent::changeHLineParams,                   .m = 3, .outOf =   4 },
    { .event = GoomEvent::changeVLineParams,                   .m = 2, .outOf =   3 },
    { .event = GoomEvent::hypercosEffectOnWithWaveMode,        .m = 1, .outOf =   2 },
    { .event = GoomEvent::waveEffectOnWithWaveMode,            .m = 1, .outOf =   3 },
    { .event = GoomEvent::changeVitesseWithWaveMode,           .m = 1, .outOf =   2 },
    { .event = GoomEvent::waveEffectOnWithCrystalBallMode,     .m = 1, .outOf =   4 },
    { .event = GoomEvent::hypercosEffectOnWithCrystalBallMode, .m = 1, .outOf =   2 },
    { .event = GoomEvent::hypercosEffectOnWithHyperCos1Mode,   .m = 1, .outOf =   3 },
    { .event = GoomEvent::hypercosEffectOnWithHyperCos2Mode,   .m = 1, .outOf =   6 },
    { .event = GoomEvent::filterReverseOffAndStopSpeed,        .m = 1, .outOf =   5 },
    { .event = GoomEvent::filterReverseOn,                     .m = 1, .outOf =  10 },
    { .event = GoomEvent::filterVitesseStopSpeedMinus1,        .m = 1, .outOf =  10 },
    { .event = GoomEvent::filterVitesseStopSpeedPlus1,         .m = 1, .outOf =  12 },
    { .event = GoomEvent::filterZeroHPlaneEffect,              .m = 1, .outOf =   2 },
    { .event = GoomEvent::filterChangeVitesseAndToggleReverse, .m = 1, .outOf =  40 },
    { .event = GoomEvent::reduceLineMode,                      .m = 1, .outOf =   5 },
    { .event = GoomEvent::updateLineMode,                      .m = 1, .outOf =   4 },
    { .event = GoomEvent::changeLineToBlack,                   .m = 1, .outOf =   2 },
    { .event = GoomEvent::changeGoomLine,                      .m = 1, .outOf =   3 },
  } };

  static constexpr std::array<std::pair<GoomFilterEvent, size_t>, numGoomFilterEvents> weightedFilterEvents{ {
    { GoomFilterEvent::waveModeWithHyperCosEffect,  2 },
    { GoomFilterEvent::waveMode,                    3 },
    { GoomFilterEvent::crystalBallMode,             1 },
    { GoomFilterEvent::crystalBallModeWithEffects,  2 },
    { GoomFilterEvent::amuletteMode,                2 },
    { GoomFilterEvent::waterMode,                   2 },
    { GoomFilterEvent::scrunchMode,                 2 },
    { GoomFilterEvent::scrunchModeWithEffects,      2 },
    { GoomFilterEvent::hyperCos1Mode,               3 },
    { GoomFilterEvent::hyperCos2Mode,               2 },
    { GoomFilterEvent::yOnlyMode,                   3 },
    { GoomFilterEvent::speedwayMode,                3 },
    { GoomFilterEvent::normalMode,                  2 },
  } };

  static constexpr std::array<std::pair<LineType, size_t>, numLineTypes> weightedLineEvents{ {
    { LineType::circle,  4 },
    { LineType::hline,   2 },
    { LineType::vline,   1 },
  } };
  // clang-format on
  const Weights<GoomFilterEvent> filterWeights;
  const Weights<LineType> lineTypeWeights;
};

using GoomEvent = GoomEvents::GoomEvent;
using GoomFilterEvent = GoomEvents::GoomFilterEvent;

GoomEvents::GoomEvents()
  : filterWeights{{weightedFilterEvents.begin(), weightedFilterEvents.end()}},
    lineTypeWeights{{weightedLineEvents.begin(), weightedLineEvents.end()}}
{
}

void GoomEvents::setGoomInfo(PluginInfo* info)
{
  goomInfo = info;
}

inline bool GoomEvents::happens(const GoomEvent event) const
{
  const WeightedEvent& weightedEvent = weightedEvents[static_cast<size_t>(event)];
  return probabilityOfMInN(goomInfo, weightedEvent.m, weightedEvent.outOf);
}

inline GoomEvents::GoomFilterEvent GoomEvents::getRandomFilterEvent() const
{
  return filterWeights.getRandomWeighted();
}

inline LineType GoomEvents::getRandomLineTypeEvent() const
{
  return lineTypeWeights.getRandomWeighted();
}

static GoomEvents goomEvent{};

inline bool changeFilterModeEventHappens(PluginInfo* goomInfo)
{
  // If we're in normal mode, then get out with a different probability.
  // (Rationale: the other modes are more interesting.)
  if (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::normalMode)
  {
    return goomEvent.happens(GoomEvent::changeFilterFromNormalMode);
  }

  return goomEvent.happens(GoomEvent::changeFilterMode);
}

class GoomStats
{
public:
  GoomStats() {}
  void setStartValues(const uint32_t stateIndex, const ZoomFilterMode filterMode);
  void reset();
  void log();
  void updateChange();
  void stateChange();
  void stateChange(const size_t index);
  void filterModeChange();
  void filterModeChange(const ZoomFilterMode);
  void lockChange();
  void doIFS();
  void doPoints();
  void doLines();
  void doStars();
  void doTentacles();
  void doBigUpdate();
  void lastTimeGoomChange();
  void megaLentChange();
  void doPerlinNoise();
  void doNoise();
  void ifsIncrLessThanEqualZero();
  void ifsIncrGreaterThanZero();

private:
  uint32_t startingState = 0;
  ZoomFilterMode startingFilterMode = ZoomFilterMode::_size;

  uint32_t numUpdates = 0;
  uint32_t totalStateChanges = 0;
  uint32_t totalFilterModeChanges = 0;
  uint32_t numLockChanges = 0;
  uint32_t numDoIFS = 0;
  uint32_t numDoPoints = 0;
  uint32_t numDoLines = 0;
  uint32_t numDoStars = 0;
  uint32_t numDoTentacles = 0;
  uint32_t numBigUpdates = 0;
  uint32_t numLastTimeGoomChanges = 0;
  uint32_t numMegaLentChanges = 0;
  uint32_t numDoPerlinNoise = 0;
  uint32_t numDoNoise = 0;
  uint32_t numIfsIncrLessThanEqualZero = 0;
  uint32_t numIfsIncrGreaterThanZero = 0;
  std::array<int, static_cast<size_t>(ZoomFilterMode::_size)> numFilterModeChanges{0};
  std::vector<int> numStateChanges;
};

void GoomStats::setStartValues(const uint32_t stateIndex, const ZoomFilterMode filterMode)
{
  startingState = stateIndex;
  startingFilterMode = filterMode;
}

void GoomStats::reset()
{
  numUpdates = 0;
  totalStateChanges = 0;
  std::fill(numStateChanges.begin(), numStateChanges.end(), 0);
  totalFilterModeChanges = 0;
  numFilterModeChanges.fill(0);
  numLockChanges = 0;
  numDoIFS = 0;
  numDoPoints = 0;
  numDoLines = 0;
  numDoStars = 0;
  numDoTentacles = 0;
  numBigUpdates = 0;
  numLastTimeGoomChanges = 0;
  numMegaLentChanges = 0;
  numDoPerlinNoise = 0;
  numDoNoise = 0;
}

void GoomStats::log()
{
  logInfo("startingState = {}", startingState);
  logInfo("startingFilterMode = {}", startingFilterMode);
  logInfo("numUpdates = {}", numUpdates);
  logInfo("totalStateChanges = {}", totalStateChanges);
  for (size_t i = 0; i < numStateChanges.size(); i++)
  {
    logInfo("State {}: numChanges = {}", i, numStateChanges[i]);
  }
  logInfo("totalFilterModeChanges = {}", totalFilterModeChanges);
  for (size_t i = 0; i < numFilterModeChanges.size(); i++)
  {
    logInfo("Filter mode {}: numChanges = {}", i, numFilterModeChanges[i]);
  }
  logInfo("numLockChanges = {}", numLockChanges);
  logInfo("numDoIFS = {}", numDoIFS);
  logInfo("numDoPoints = {}", numDoPoints);
  logInfo("numDoLines = {}", numDoLines);
  logInfo("numDoStars = {}", numDoStars);
  logInfo("numDoTentacles = {}", numDoTentacles);
  logInfo("numLastTimeGoomChanges = {}", numLastTimeGoomChanges);
  logInfo("numMegaLentChanges = {}", numMegaLentChanges);
  logInfo("numDoPerlinNoise = {}", numDoPerlinNoise);
  logInfo("numDoNoise = {}", numDoNoise);
  logInfo("numIfsIncrLessThanEqualZero = {}", numIfsIncrLessThanEqualZero);
  logInfo("numIfsIncrGreaterThanZero = {}", numIfsIncrGreaterThanZero);
}

inline void GoomStats::updateChange()
{
  numUpdates++;
}

inline void GoomStats::stateChange()
{
  totalStateChanges++;
}

inline void GoomStats::stateChange(const size_t index)
{
  if (index >= numStateChanges.size())
  {
    for (size_t i = numStateChanges.size(); i <= index; i++)
    {
      numStateChanges.push_back(0);
    }
  }
  numStateChanges[index]++;
}

inline void GoomStats::filterModeChange()
{
  totalFilterModeChanges++;
}

inline void GoomStats::filterModeChange(const ZoomFilterMode mode)
{
  numFilterModeChanges.at(static_cast<size_t>(mode))++;
}

inline void GoomStats::lockChange()
{
  numLockChanges++;
}

inline void GoomStats::doIFS()
{
  numDoIFS++;
}

inline void GoomStats::doPoints()
{
  numDoPoints++;
}

inline void GoomStats::doLines()
{
  numDoLines++;
}

inline void GoomStats::doStars()
{
  numDoStars++;
}

inline void GoomStats::doTentacles()
{
  numDoTentacles++;
}

inline void GoomStats::doBigUpdate()
{
  numBigUpdates++;
}

inline void GoomStats::lastTimeGoomChange()
{
  numLastTimeGoomChanges++;
}

inline void GoomStats::megaLentChange()
{
  numMegaLentChanges++;
}

inline void GoomStats::doPerlinNoise()
{
  numDoPerlinNoise++;
}

inline void GoomStats::doNoise()
{
  numDoNoise++;
}

inline void GoomStats::ifsIncrLessThanEqualZero()
{
  numIfsIncrLessThanEqualZero++;
}

inline void GoomStats::ifsIncrGreaterThanZero()
{
  numIfsIncrGreaterThanZero++;
}

constexpr int32_t stopSpeed = 128;
// TODO: put that as variable in PluginInfo
constexpr int32_t timeBetweenChange = 300;

static void changeState(PluginInfo* goomInfo, const int forceMode);
static void changeFilterMode(PluginInfo* goomInfo);

static void init_buffers(PluginInfo* goomInfo, const uint32_t buffsize)
{
  goomInfo->pixel = (uint32_t*)malloc(buffsize * sizeof(uint32_t) + 128);
  bzero(goomInfo->pixel, buffsize * sizeof(uint32_t) + 128);
  goomInfo->back = (uint32_t*)malloc(buffsize * sizeof(uint32_t) + 128);
  bzero(goomInfo->back, buffsize * sizeof(uint32_t) + 128);
  goomInfo->conv = (Pixel*)malloc(buffsize * sizeof(uint32_t) + 128);
  bzero(goomInfo->conv, buffsize * sizeof(uint32_t) + 128);

  goomInfo->outputBuf = goomInfo->conv;

  goomInfo->p1 = (Pixel*)((1 + ((uintptr_t)(goomInfo->pixel)) / 128) * 128);
  goomInfo->p2 = (Pixel*)((1 + ((uintptr_t)(goomInfo->back)) / 128) * 128);
}

static void swapBuffers(PluginInfo* goomInfo)
{
  Pixel* tmp = goomInfo->p1;
  goomInfo->p1 = goomInfo->p2;
  goomInfo->p2 = tmp;
}

static GoomStats stats{};

PluginInfo* goom_init(const uint16_t resx, const uint16_t resy, const int seed)
{
  logDebug("Initialize goom: resx = {}, resy = {}, seed = {}.", resx, resy, seed);

  stats.reset();

  PluginInfo* goomInfo = new PluginInfo;

  plugin_info_init(goomInfo, 5);

  goomInfo->star_fx = flying_star_create();
  goomInfo->zoomFilter_fx = zoomFilterVisualFXWrapper_create();
  goomInfo->tentacles_fx = tentacle_fx_create();
  goomInfo->convolve_fx = convolve_create();
  goomInfo->ifs_fx = ifs_visualfx_create();

  goomInfo->screen.width = resx;
  goomInfo->screen.height = resy;
  goomInfo->screen.size = resx * resy;

  init_buffers(goomInfo, goomInfo->screen.size);

  if (seed >= 0)
  {
    const uint64_t seedVal =
        seed == 0 ? reinterpret_cast<uint64_t>(goomInfo->pixel) : static_cast<uint64_t>(seed);
    pcg32_init(seedVal);
  }
  goomInfo->gRandom = goom_random_init();

  goomInfo->star_fx.init(&goomInfo->star_fx, goomInfo);
  goomInfo->zoomFilter_fx.init(&goomInfo->zoomFilter_fx, goomInfo);
  goomInfo->tentacles_fx.init(&goomInfo->tentacles_fx, goomInfo);
  goomInfo->convolve_fx.init(&goomInfo->convolve_fx, goomInfo);
  goomInfo->ifs_fx.init(&goomInfo->ifs_fx, goomInfo);
  plugin_info_add_visual(goomInfo, 0, &goomInfo->zoomFilter_fx);
  plugin_info_add_visual(goomInfo, 1, &goomInfo->tentacles_fx);
  plugin_info_add_visual(goomInfo, 2, &goomInfo->star_fx);
  plugin_info_add_visual(goomInfo, 3, &goomInfo->convolve_fx);
  plugin_info_add_visual(goomInfo, 4, &goomInfo->ifs_fx);

  goomInfo->cycle = 0;

  const uint32_t lRed = getRedLineColor();
  const uint32_t lGreen = getGreenLineColor();
  const uint32_t lBlack = getBlackLineColor();
  goomInfo->gmline1 = goomLinesInit(goomInfo, resx, goomInfo->screen.height, LineType::hline,
                                    goomInfo->screen.height, lBlack, LineType::circle,
                                    0.4f * static_cast<float>(goomInfo->screen.height), lGreen);
  goomInfo->gmline2 =
      goomLinesInit(goomInfo, resx, goomInfo->screen.height, LineType::hline, 0, lBlack,
                    LineType::circle, 0.2f * static_cast<float>(goomInfo->screen.height), lRed);

  gfont_load();

  /* goom_set_main_script(goomInfo, goomInfo->main_script_str); */

  goomEvent.setGoomInfo(goomInfo);

  changeState(goomInfo, 0);
  changeFilterMode(goomInfo);
  zoomFilterInitData(goomInfo);

  stats.setStartValues(goomInfo->curGStateIndex, goomInfo->update.zoomFilterData.mode);

  return goomInfo;
}

void goom_set_resolution(PluginInfo* goomInfo, const uint16_t resx, const uint16_t resy)
{
  free(goomInfo->pixel);
  free(goomInfo->back);
  free(goomInfo->conv);

  goomInfo->screen.width = resx;
  goomInfo->screen.height = resy;
  goomInfo->screen.size = resx * resy;

  init_buffers(goomInfo, goomInfo->screen.size);

  /* init_ifs (goomInfo, resx, goomInfo->screen.height); */
  goomInfo->ifs_fx.free(&goomInfo->ifs_fx);
  goomInfo->ifs_fx.init(&goomInfo->ifs_fx, goomInfo);

  goomLinesSetResolution(goomInfo->gmline1, resx, goomInfo->screen.height);
  goomLinesSetResolution(goomInfo->gmline2, resx, goomInfo->screen.height);
}

int goom_set_screenbuffer(PluginInfo* goomInfo, void* buffer)
{
  goomInfo->outputBuf = static_cast<Pixel*>(buffer);
  return 1;
}

/********************************************
 *                  UPDATE                  *
 ********************************************
*/

static float largeSoundFactor(const SoundInfo* sound);
static void updateDecayRecay(PluginInfo* goomInfo);

// baisser regulierement la vitesse
static void regularlyLowerTheSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void lowerSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd);

// on verifie qu'il ne se pas un truc interressant avec le son.
static void changeFilterModeIfMusicChanges(PluginInfo* goomInfo, const int forceMode);

// Changement d'effet de zoom !
static void changeZoomEffect(PluginInfo* goomInfo, ZoomFilterData* pzfd, const int forceMode);

static void applyIfsIfRequired(PluginInfo* goomInfo);

// Affichage tentacule
static void applyTentaclesIfRequired(PluginInfo* goomInfo);

static void applyStarsIfRequired(PluginInfo* goomInfo);

// Affichage de texte
void displayText(PluginInfo* goomInfo, const char* songTitle, const char* message, const float fps);

static void drawPointsIfRequired(PluginInfo* goomInfo,
                                 const uint32_t pointWidth,
                                 const uint32_t pointHeight,
                                 const float largeFactor);
static void drawPoints(PluginInfo* goomInfo,
                       const uint32_t pointWidth,
                       const uint32_t pointHeight,
                       const float largeFactor);

static void chooseGoomLine(PluginInfo* goomInfo,
                           float* param1,
                           float* param2,
                           uint32_t* couleur,
                           LineType* mode,
                           float* amplitude,
                           const int far);

// si on est dans un goom : afficher les lignes
static void displayLinesIfInAGoom(PluginInfo* goomInfo,
                                  const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);
static void displayLines(PluginInfo* goomInfo,
                         const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

// arret demande
static void stopRequest(PluginInfo* goomInfo);
static void stopIfRequested(PluginInfo* goomInfo);

// arret aleatore.. changement de mode de ligne..
static void stopRandomLineChangeMode(PluginInfo* goomInfo);

// Permet de forcer un effet.
static void forceFilterModeIfSet(PluginInfo* oomInfo, ZoomFilterData** pzfd, const int forceMode);
static void forceFilterMode(PluginInfo* goomInfo, const int forceMode, ZoomFilterData** pzfd);

// arreter de decrémenter au bout d'un certain temps
static void stopDecrementingAfterAWhile(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void stopDecrementing(PluginInfo* goomInfo, ZoomFilterData** pzfd);

// tout ceci ne sera fait qu'en cas de non-blocage
static void bigUpdateIfNotLocked(PluginInfo* goomInfo, ZoomFilterData** pzfd, const int forceMode);
static void bigUpdate(PluginInfo* goomInfo, ZoomFilterData** pzfd, const int forceMode);

// gros frein si la musique est calme
static void bigBreakIfMusicIsCalm(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void bigBreak(PluginInfo* goomInfo, ZoomFilterData** pzfd);

static void update_message(PluginInfo* goomInfo, const char* message);

static void pointFilter(PluginInfo* goomInfo,
                        Pixel* pix1,
                        const uint32_t color,
                        const float t1,
                        const float t2,
                        const float t3,
                        const float t4,
                        const uint32_t cycle);


uint32_t* goom_update(PluginInfo* goomInfo,
                      const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                      const int forceMode,
                      const float fps,
                      const char* songTitle,
                      const char* message)
{
  stats.updateChange();

  // elargissement de l'intervalle d'évolution des points
  // ! calcul du deplacement des petits points ...
  const uint16_t pointWidth = (goomInfo->screen.width * 2) / 5;
  const uint16_t pointHeight = (goomInfo->screen.height * 2) / 5;

  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
  logDebug("pcg32_get_last_state = {}", pcg32_get_last_state());
  logDebug("goomInfo->gRandom->pos = {}", goomInfo->gRandom->pos);

  /* ! etude du signal ... */
  evaluate_sound(data, &(goomInfo->sound));
  const float largeFactor = largeSoundFactor(&goomInfo->sound);

  updateDecayRecay(goomInfo);

  applyIfsIfRequired(goomInfo);

  drawPointsIfRequired(goomInfo, pointWidth, pointHeight, largeFactor);

  /* par défaut pas de changement de zoom */
  ZoomFilterData* pzfd = nullptr;
  if (forceMode != 0)
  {
    logDebug("forceMode = {}\n", forceMode);
  }

  goomInfo->update.lockvar--;
  if (goomInfo->update.lockvar < 0)
  {
    goomInfo->update.lockvar = 0;
  }
  stats.lockChange();
  /* note pour ceux qui n'ont pas suivis : le lockvar permet d'empecher un */
  /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
  // -- Note for those who have not followed: the lockvar prevents a change
  // of state of the plugin just after another change of state.

  changeFilterModeIfMusicChanges(goomInfo, forceMode);

  bigUpdateIfNotLocked(goomInfo, &pzfd, forceMode);

  bigBreakIfMusicIsCalm(goomInfo, &pzfd);

  regularlyLowerTheSpeed(goomInfo, &pzfd);

  stopDecrementingAfterAWhile(goomInfo, &pzfd);

  forceFilterModeIfSet(goomInfo, &pzfd, forceMode);

  changeZoomEffect(goomInfo, pzfd, forceMode);

  // Zoom here!
  zoomFilterFastRGB(goomInfo, goomInfo->p1, goomInfo->p2, pzfd, goomInfo->screen.width,
                    goomInfo->screen.height, goomInfo->update.switchIncr,
                    goomInfo->update.switchMult);

  applyTentaclesIfRequired(goomInfo);

  applyStarsIfRequired(goomInfo);

  displayText(goomInfo, songTitle, message, fps);

  // Gestion du Scope - Scope management
  stopIfRequested(goomInfo);
  stopRandomLineChangeMode(goomInfo);
  displayLinesIfInAGoom(goomInfo, data);

  // affichage et swappage des buffers...
  Pixel* returnVal = goomInfo->p1;
  swapBuffers(goomInfo);
  goomInfo->cycle++;
  goomInfo->convolve_fx.apply(&goomInfo->convolve_fx, returnVal, goomInfo->outputBuf, goomInfo);

  logDebug("About to return.");

  return reinterpret_cast<uint32_t*>(goomInfo->outputBuf);
}

/****************************************
*                CLOSE                 *
****************************************/
void goom_close(PluginInfo* goomInfo)
{
  stats.log();

  if (goomInfo->pixel)
  {
    free(goomInfo->pixel);
  }
  if (goomInfo->back)
  {
    free(goomInfo->back);
  }
  if (goomInfo->conv)
  {
    free(goomInfo->conv);
  }

  goomInfo->pixel = goomInfo->back = nullptr;
  goomInfo->conv = nullptr;
  goom_random_free(goomInfo->gRandom);
  goomLinesFree(&goomInfo->gmline1);
  goomLinesFree(&goomInfo->gmline2);

  /* release_ifs (); */
  goomInfo->ifs_fx.free(&goomInfo->ifs_fx);
  goomInfo->convolve_fx.free(&goomInfo->convolve_fx);
  goomInfo->star_fx.free(&goomInfo->star_fx);
  goomInfo->tentacles_fx.free(&goomInfo->tentacles_fx);
  goomInfo->zoomFilter_fx.free(&goomInfo->zoomFilter_fx);

  // Release info visual
  free(goomInfo->params);
  free(goomInfo->sound.params.params);

  // Release PluginInfo
  free(goomInfo->visuals);

  delete goomInfo;
}

static void chooseGoomLine(PluginInfo* goomInfo,
                           float* param1,
                           float* param2,
                           uint32_t* couleur,
                           LineType* mode,
                           float* amplitude,
                           const int far)
{
  *amplitude = 1.0f;
  *mode = goomEvent.getRandomLineTypeEvent();

  switch (*mode)
  {
    case LineType::circle:
      if (far)
      {
        *param1 = *param2 = 0.47f;
        *amplitude = 0.8f;
        break;
      }
      if (goomEvent.happens(GoomEvent::changeLineCircleAmplitude))
      {
        *param1 = *param2 = 0;
        *amplitude = 3.0f;
      }
      else if (goomEvent.happens(GoomEvent::changeLineCircleParams))
      {
        *param1 = 0.40f * goomInfo->screen.height;
        *param2 = 0.22f * goomInfo->screen.height;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.height * 0.35;
      }
      break;
    case LineType::hline:
      if (goomEvent.happens(GoomEvent::changeHLineParams) || far)
      {
        *param1 = goomInfo->screen.height / 7;
        *param2 = 6.0f * goomInfo->screen.height / 7.0f;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.height / 2.0f;
        *amplitude = 2.0f;
      }
      break;
    case LineType::vline:
      if (goomEvent.happens(GoomEvent::changeVLineParams) || far)
      {
        *param1 = goomInfo->screen.width / 7.0f;
        *param2 = 6.0f * goomInfo->screen.width / 7.0f;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.width / 2.0f;
        *amplitude = 1.5f;
      }
      break;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }

  *couleur = getRandomLineColor(goomInfo);
}

constexpr float ECART_VARIATION = 1.5;
constexpr float POS_VARIATION = 3.0;
constexpr int SCROLLING_SPEED = 80;

/*
 * Met a jour l'affichage du message defilant
 */
static void update_message(PluginInfo* goomInfo, const char* message)
{
  int fin = 0;

  if (message)
  {
    strcpy(goomInfo->update_message.message, message);
    uint32_t i = 1;
    for (size_t j = 0; goomInfo->update_message.message[j]; j++)
    {
      if (goomInfo->update_message.message[j] == '\n')
      {
        i++;
      }
    }
    goomInfo->update_message.numberOfLinesInMessage = i;
    goomInfo->update_message.affiche =
        goomInfo->screen.height + goomInfo->update_message.numberOfLinesInMessage * 25 + 105;
    goomInfo->update_message.longueur = strlen(goomInfo->update_message.message);
  }
  if (goomInfo->update_message.affiche)
  {
    uint32_t i = 0;
    char* msg = (char*)malloc((size_t)goomInfo->update_message.longueur + 1);
    char* ptr = msg;
    message = msg;
    strcpy(msg, goomInfo->update_message.message);

    while (!fin)
    {
      while (1)
      {
        if (*ptr == 0)
        {
          fin = 1;
          break;
        }
        if (*ptr == '\n')
        {
          *ptr = 0;
          break;
        }
        ++ptr;
      }
      uint32_t pos = goomInfo->update_message.affiche -
                     (goomInfo->update_message.numberOfLinesInMessage - i) * 25;
      pos += POS_VARIATION * (cos((double)pos / 20.0));
      pos -= SCROLLING_SPEED;
      const float ecart = (ECART_VARIATION * sin((double)pos / 20.0));
      if ((fin) && (2 * pos < goomInfo->screen.height))
      {
        pos = goomInfo->screen.height / 2;
      }
      pos += 7;

      goom_draw_text(goomInfo->p1, goomInfo->screen.width, goomInfo->screen.height,
                     static_cast<int>(goomInfo->screen.width / 2), static_cast<int>(pos), message,
                     ecart, 1);
      message = ++ptr;
      i++;
    }
    goomInfo->update_message.affiche--;
    free(msg);
  }
}

static void drawPointsIfRequired(PluginInfo* goomInfo,
                                 const uint32_t pointWidth,
                                 const uint32_t pointHeight,
                                 const float largeFactor)
{
  if (!goomInfo->curGDrawables.count(GoomDrawable::points))
  {
    return;
  }

  logDebug("goomInfo->curGDrawables points is set.");
  drawPoints(goomInfo, pointWidth, pointHeight, largeFactor);
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
}

constexpr uint32_t WHITE = 0xffffffff;

static void drawPoints(PluginInfo* goomInfo,
                       const uint32_t pointWidth,
                       const uint32_t pointHeight,
                       const float largeFactor)
{
  stats.doPoints();

  const uint32_t speedvarMult80Plus15 = goomInfo->sound.speedvar * 80 + 15;
  const uint32_t speedvarMult50Plus1 = goomInfo->sound.speedvar * 50 + 1;
  logDebug("speedvarMult80Plus15 = {}", speedvarMult80Plus15);
  logDebug("speedvarMult50Plus1 = {}", speedvarMult50Plus1);

  constexpr uint32_t BLACK = 0;
  constexpr uint32_t YELLOW = 0xffffff33;
  constexpr uint32_t ORANGE = 0xffffcc05;
  constexpr uint32_t VIOLET = 0xff5505ff;

  const float pointWidthDiv2 = pointWidth / 2;
  const float pointHeightDiv2 = pointHeight / 2;
  const float pointWidthDiv3 = pointWidth / 3;
  const float pointHeightDiv3 = pointHeight / 3;
  const float pointWidthDiv2MultLarge = pointWidthDiv2 * largeFactor;
  const float pointHeightDiv2MultLarge = pointHeightDiv2 * largeFactor;
  const float pointWidthDiv3MultLarge = (pointWidthDiv3 + 5.0f) * largeFactor;
  const float pointHeightDiv3MultLarge = (pointHeightDiv3 + 5.0f) * largeFactor;
  const float pointWidthMultLarge = pointWidth * largeFactor;
  const float pointHeightMultLarge = pointHeight * largeFactor;

  const float yellow_t1 = (pointWidth - 6.0f) * largeFactor + 5.0f;
  const float yellow_t2 = (pointHeight - 6.0f) * largeFactor + 5.0f;
  const float black_t1 = pointHeightDiv3 * largeFactor + 20.0f;
  const float black_t2 = black_t1;

  logDebug("goomInfo->update.loopvar = {}", goomInfo->update.loopvar);
  for (uint32_t i = 1; i * 15 <= speedvarMult80Plus15; i++)
  {
    goomInfo->update.loopvar += speedvarMult50Plus1;
    logDebug("goomInfo->update.loopvar = {}", goomInfo->update.loopvar);

    const uint32_t loopvar_div_i = goomInfo->update.loopvar / i;
    const float i_mult_10 = 10.0f * i;

    const float yellow_t3 = i * 152.0f;
    const float yellow_t4 = 128.0f;
    const uint32_t yellow_cycle = goomInfo->update.loopvar + i * 2032;

    const float orange_t1 = pointWidthDiv2MultLarge / i + i_mult_10;
    const float orange_t2 = pointHeightDiv2MultLarge / i + i_mult_10;
    const float orange_t3 = 96.0f;
    const float orange_t4 = i * 80.0f;
    const uint32_t orange_cycle = loopvar_div_i;

    const float violet_t1 = pointWidthDiv3MultLarge / i + i_mult_10;
    const float violet_t2 = pointHeightDiv3MultLarge / i + i_mult_10;
    const float violet_t3 = i + 122.0f;
    const float violet_t4 = 134.0f;
    const uint32_t violet_cycle = loopvar_div_i;

    const float black_t3 = 58.0f;
    const float black_t4 = i * 66.0f;
    const uint32_t black_cycle = loopvar_div_i;

    const float white_t1 = (pointWidthMultLarge + i_mult_10) / i;
    const float white_t2 = (pointHeightMultLarge + i_mult_10) / i;
    const float white_t3 = 66.0f;
    const float white_t4 = 74.0f;
    const uint32_t white_cycle = goomInfo->update.loopvar + i * 500;

    pointFilter(goomInfo, goomInfo->p1, YELLOW, yellow_t1, yellow_t2, yellow_t3, yellow_t4,
                yellow_cycle);
    pointFilter(goomInfo, goomInfo->p1, ORANGE, orange_t1, orange_t2, orange_t3, orange_t4,
                orange_cycle);
    pointFilter(goomInfo, goomInfo->p1, VIOLET, violet_t1, violet_t2, violet_t3, violet_t4,
                violet_cycle);
    pointFilter(goomInfo, goomInfo->p1, BLACK, black_t1, black_t2, black_t3, black_t4, black_cycle);
    pointFilter(goomInfo, goomInfo->p1, WHITE, white_t1, white_t2, white_t3, white_t4, white_cycle);
  }
}

static void changeFilterModeIfMusicChanges(PluginInfo* goomInfo, const int forceMode)
{
  logDebug("forceMode = {}", forceMode);
  if (forceMode == -1)
  {
    return;
  }

  logDebug("goomInfo->sound.timeSinceLastGoom = {}, goomInfo->update.cyclesSinceLastChange = {}",
           goomInfo->sound.timeSinceLastGoom, goomInfo->update.cyclesSinceLastChange);
  if ((goomInfo->sound.timeSinceLastGoom == 0) ||
      (goomInfo->update.cyclesSinceLastChange > timeBetweenChange) || (forceMode > 0))
  {
    logDebug("Try to change the filter mode.");
    if (changeFilterModeEventHappens(goomInfo))
    {
      changeFilterMode(goomInfo);
    }
  }
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
}

static void changeFilterMode(PluginInfo* goomInfo)
{
  logDebug("Time to change the filter mode.");

  stats.filterModeChange();

  switch (goomEvent.getRandomFilterEvent())
  {
    case GoomFilterEvent::waveModeWithHyperCosEffect:
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithWaveMode);
      [[fallthrough]];
    case GoomFilterEvent::waveMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::waveMode;
      goomInfo->update.zoomFilterData.reverse = false;
      goomInfo->update.zoomFilterData.waveEffect =
          goomEvent.happens(GoomEvent::waveEffectOnWithWaveMode);
      if (goomEvent.happens(GoomEvent::changeVitesseWithWaveMode))
      {
        goomInfo->update.zoomFilterData.vitesse =
            (goomInfo->update.zoomFilterData.vitesse + 127) >> 1;
      }
      break;
    case GoomFilterEvent::crystalBallMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case GoomFilterEvent::crystalBallModeWithEffects:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomInfo->update.zoomFilterData.waveEffect =
          goomEvent.happens(GoomEvent::waveEffectOnWithCrystalBallMode);
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithCrystalBallMode);
      break;
    case GoomFilterEvent::amuletteMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::amuletteMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case GoomFilterEvent::waterMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::waterMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case GoomFilterEvent::scrunchMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case GoomFilterEvent::scrunchModeWithEffects:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomInfo->update.zoomFilterData.waveEffect = true;
      goomInfo->update.zoomFilterData.hypercosEffect = true;
      break;
    case GoomFilterEvent::hyperCos1Mode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::hyperCos1Mode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithHyperCos1Mode);
      break;
    case GoomFilterEvent::hyperCos2Mode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::hyperCos2Mode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect =
          goomEvent.happens(GoomEvent::hypercosEffectOnWithHyperCos2Mode);
      break;
    case GoomFilterEvent::yOnlyMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::yOnlyMode;
      break;
    case GoomFilterEvent::speedwayMode:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::speedwayMode;
      break;
    case GoomFilterEvent::normalMode:
      if (goomInfo->curGDrawables.count(GoomDrawable::tentacles))
      {
        // Tentacles don't look so good with normal mode so remove tentacles.
        goomInfo->curGDrawables.erase(GoomDrawable::tentacles);
        logInfo("Because of 'normal mode' filter, removed tentacles from current goom state.");
      }
      else
      {
        goomInfo->update.zoomFilterData.mode = ZoomFilterMode::normalMode;
        goomInfo->update.zoomFilterData.waveEffect = false;
        goomInfo->update.zoomFilterData.hypercosEffect = false;
      }
      break;
    default:
      throw std::logic_error("GoomFilterEvent not implemented.");
  }

  stats.filterModeChange(goomInfo->update.zoomFilterData.mode);

  if ((goomInfo->update.zoomFilterData.mode != ZoomFilterMode::normalMode) &&
      goomInfo->curGDrawables != goomInfo->curGState->drawables)
  {
    logInfo("Not 'normal' filter mode: reset drawables (put back anything previously removed).");
    goomInfo->curGDrawables = goomInfo->curGState->drawables;
  }
}

static void changeState(PluginInfo* goomInfo, const int forceMode)
{
  size_t stateIndex = (goomInfo->curGStateIndex + 1) % goomInfo->numStates;
  for (size_t numTry = 0; numTry < 10; numTry++)
  {
    goomInfo->update.stateSelectionRand = goomInfo->getNRand(goomInfo->maxStateSelect + 1);

    for (size_t i = 0; i < goomInfo->numStates; i++)
    {
      if (i == goomInfo->curGStateIndex)
      {
        // Don't pick the same state again.
        continue;
      }
      if ((goomInfo->update.stateSelectionRand >= goomInfo->states[i].minSel) &&
          (goomInfo->update.stateSelectionRand <= goomInfo->states[i].maxSel))
      {
        stateIndex = i;
        break;
      }
    }
  }

  goomInfo->curGState = &(goomInfo->states[stateIndex]);
  goomInfo->curGDrawables = goomInfo->states[stateIndex].drawables;
  goomInfo->curGStateIndex = stateIndex;
  logDebug("Changed goom state to {}", goomInfo->curGStateIndex);
  stats.stateChange();
  stats.stateChange(stateIndex);

  if (goomInfo->curGDrawables.count(GoomDrawable::IFS) && (goomInfo->update.ifs_incr <= 0))
  {
    goomInfo->update.recay_ifs = 5;
    goomInfo->update.ifs_incr = 11;
    ifsRenew(goomInfo);
    stats.ifsIncrLessThanEqualZero();
  }
  if (!goomInfo->curGDrawables.count(GoomDrawable::IFS) && (goomInfo->update.ifs_incr > 0) &&
      (goomInfo->update.decay_ifs <= 0))
  {
    goomInfo->update.decay_ifs = 100;
    ifsRenew(goomInfo);
    stats.ifsIncrGreaterThanZero();
  }

  if (!goomInfo->curGDrawables.count(GoomDrawable::scope))
  {
    goomInfo->update.stop_lines = 0xf000 & 5;
  }
  if (!goomInfo->curGDrawables.count(GoomDrawable::farScope))
  {
    goomInfo->update.stop_lines = 0;
    goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
  }

  if (goomInfo->curGDrawables.count(GoomDrawable::tentacles) && (forceMode == 0) &&
      (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::normalMode))
  {
    // Tentacles and normal filter mode don't look so good - choose another mode.
    logInfo("It's a tentacle state - changing filter mode away from 'normal mode'.");
    changeFilterMode(goomInfo);
  }
}

static void bigUpdate(PluginInfo* goomInfo, ZoomFilterData** pzfd, const int forceMode)
{
  stats.doBigUpdate();

  // reperage de goom (acceleration forte de l'acceleration du volume)
  //   -> coup de boost de la vitesse si besoin..
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
  if (goomInfo->sound.timeSinceLastGoom == 0)
  {
    stats.lastTimeGoomChange();

    logDebug("goomInfo->sound.timeSinceLastGoom = 0.");

    goomInfo->update.goomvar++;

    if (goomInfo->update.stateSelectionBlocker)
    {
      goomInfo->update.stateSelectionBlocker--;
    }
    else if (goomEvent.happens(GoomEvent::changeState))
    {
      goomInfo->update.stateSelectionBlocker = 3;
      changeState(goomInfo, forceMode);
    }

    // if (goomInfo->update.goomvar % 1 == 0)
    {
      goomInfo->update.lockvar = 50;
      stats.lockChange();
      const int32_t newvit =
          stopSpeed + 1 - static_cast<int32_t>(3.5f * log10(goomInfo->sound.speedvar * 60 + 1));
      // retablir le zoom avant..
      if ((goomInfo->update.zoomFilterData.reverse) && (!(goomInfo->cycle % 13)) &&
          goomEvent.happens(GoomEvent::filterReverseOffAndStopSpeed))
      {
        goomInfo->update.zoomFilterData.reverse = false;
        goomInfo->update.zoomFilterData.vitesse = stopSpeed - 2;
        goomInfo->update.lockvar = 75;
        stats.lockChange();
      }
      if (goomEvent.happens(GoomEvent::filterReverseOn))
      {
        goomInfo->update.zoomFilterData.reverse = true;
        goomInfo->update.lockvar = 100;
        stats.lockChange();
      }

      if (goomEvent.happens(GoomEvent::filterVitesseStopSpeedMinus1))
      {
        goomInfo->update.zoomFilterData.vitesse = stopSpeed - 1;
      }
      if (goomEvent.happens(GoomEvent::filterVitesseStopSpeedPlus1))
      {
        goomInfo->update.zoomFilterData.vitesse = stopSpeed + 1;
      }

      // changement de milieu..
      switch (goomInfo->getNRand(25))
      {
        case 0:
        case 1:
        case 2:
          goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height - 1;
          goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
          break;
        case 3:
        case 4:
          goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width - 1;
          break;
        case 5:
        case 6:
          goomInfo->update.zoomFilterData.middleX = 1;
          break;
        default:
          goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
          goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
      }

      if ((goomInfo->update.zoomFilterData.mode == ZoomFilterMode::waterMode) ||
          (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::yOnlyMode) ||
          (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode))
      {
        goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
        goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
      }

      switch (const uint32_t vtmp = goomInfo->getNRand(15))
      {
        case 0:
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goomInfo->getNRand(3) - goomInfo->getNRand(3));
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(goomInfo->getNRand(3) - goomInfo->getNRand(3));
          break;
        case 3:
          goomInfo->update.zoomFilterData.vPlaneEffect = 0;
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(goomInfo->getNRand(8) - goomInfo->getNRand(8));
          break;
        case 4:
        case 5:
        case 6:
        case 7:
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goomInfo->getNRand(5) - goomInfo->getNRand(5));
          goomInfo->update.zoomFilterData.hPlaneEffect =
              -goomInfo->update.zoomFilterData.vPlaneEffect;
          break;
        case 8:
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(5 + goomInfo->getNRand(8));
          goomInfo->update.zoomFilterData.vPlaneEffect =
              -goomInfo->update.zoomFilterData.hPlaneEffect;
          break;
        case 9:
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(5 + goomInfo->getNRand(8));
          goomInfo->update.zoomFilterData.hPlaneEffect =
              -goomInfo->update.zoomFilterData.hPlaneEffect;
          break;
        case 13:
          goomInfo->update.zoomFilterData.hPlaneEffect = 0;
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goomInfo->getNRand(10) - goomInfo->getNRand(10));
          break;
        case 14:
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(goomInfo->getNRand(10) - goomInfo->getNRand(10));
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goomInfo->getNRand(10) - goomInfo->getNRand(10));
          break;
        default:
          if (vtmp < 10)
          {
            goomInfo->update.zoomFilterData.vPlaneEffect = 0;
            goomInfo->update.zoomFilterData.hPlaneEffect = 0;
          }
      }

      if (goomEvent.happens(GoomEvent::changeNoiseState))
      {
        goomInfo->update.zoomFilterData.perlinNoisify = false;
      }
      else
      {
        goomInfo->update.zoomFilterData.perlinNoisify = false;
        //        goomInfo->update.lockvar *= 2;
        //        stats.doPerlinNoise();
      }

      if (goomEvent.happens(GoomEvent::changeNoiseState))
      {
        goomInfo->update.zoomFilterData.noisify = false;
      }
      else
      {
        goomInfo->update.zoomFilterData.noisify = true;
        goomInfo->update.lockvar *= 2;
        stats.lockChange();
        stats.doNoise();
      }

      if (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode)
      {
        goomInfo->update.zoomFilterData.vPlaneEffect = 0;
        goomInfo->update.zoomFilterData.hPlaneEffect = 0;
        goomInfo->update.zoomFilterData.noisify = false;
      }

      if ((goomInfo->update.zoomFilterData.middleX == 1) ||
          (goomInfo->update.zoomFilterData.middleX == goomInfo->screen.width - 1))
      {
        goomInfo->update.zoomFilterData.vPlaneEffect = 0;
        if (goomEvent.happens(GoomEvent::filterZeroHPlaneEffect))
        {
          goomInfo->update.zoomFilterData.hPlaneEffect = 0;
        }
      }

      logDebug("newvit = {}, goomInfo->update.zoomFilterData.vitesse = {}", newvit,
               goomInfo->update.zoomFilterData.vitesse);
      if (newvit < goomInfo->update.zoomFilterData.vitesse)
      { // on accelere
        logDebug("newvit = {} < {} = goomInfo->update.zoomFilterData.vitesse", newvit,
                 goomInfo->update.zoomFilterData.vitesse);
        *pzfd = &goomInfo->update.zoomFilterData;
        if (((newvit < (stopSpeed - 7)) &&
             (goomInfo->update.zoomFilterData.vitesse < stopSpeed - 6) &&
             (goomInfo->cycle % 3 == 0)) ||
            goomEvent.happens(GoomEvent::filterChangeVitesseAndToggleReverse))
        {
          goomInfo->update.zoomFilterData.vitesse = stopSpeed -
                                                    static_cast<int32_t>(goomInfo->getNRand(2)) +
                                                    static_cast<int32_t>(goomInfo->getNRand(2));
          goomInfo->update.zoomFilterData.reverse = !goomInfo->update.zoomFilterData.reverse;
        }
        else
        {
          goomInfo->update.zoomFilterData.vitesse =
              (newvit + goomInfo->update.zoomFilterData.vitesse * 7) / 8;
        }
        goomInfo->update.lockvar += 50;
        stats.lockChange();
      }
    }

    if (goomInfo->update.lockvar > 150)
    {
      goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
      goomInfo->update.switchMult = 1.0f;
    }
  }

  // mode mega-lent
  if (goomEvent.happens(GoomEvent::changeToMegaLentMode))
  {
    stats.megaLentChange();
    logDebug("mega lent change");
    *pzfd = &goomInfo->update.zoomFilterData;
    goomInfo->update.zoomFilterData.vitesse = stopSpeed - 1;
    goomInfo->update.zoomFilterData.pertedec = 8;
    goomInfo->update.goomvar = 1;
    goomInfo->update.lockvar += 50;
    stats.lockChange();
    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
    goomInfo->update.switchMult = 1.0f;
  }
}

/* Changement d'effet de zoom !
 */
static void changeZoomEffect(PluginInfo* goomInfo, ZoomFilterData* pzfd, const int forceMode)
{
  if (pzfd)
  {
    logDebug("pzfd != nullptr");

    goomInfo->update.cyclesSinceLastChange = 0;
    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;

    int dif = static_cast<int>(goomInfo->update.zoomFilterData.vitesse -
                               goomInfo->update.previousZoomSpeed);
    if (dif < 0)
    {
      dif = -dif;
    }

    if (dif > 2)
    {
      goomInfo->update.switchIncr *= (dif + 2) / 2;
    }
    goomInfo->update.previousZoomSpeed = goomInfo->update.zoomFilterData.vitesse;
    goomInfo->update.switchMult = 1.0f;

    if (((goomInfo->sound.timeSinceLastGoom == 0) && (goomInfo->sound.totalgoom < 2)) ||
        (forceMode > 0))
    {
      goomInfo->update.switchIncr = 0;
      goomInfo->update.switchMult = goomInfo->update.switchMultAmount;
    }
  }
  else
  {
    logDebug("pzfd = nullptr");
    logDebug("goomInfo->update.cyclesSinceLastChange = {}", goomInfo->update.cyclesSinceLastChange);
    if (goomInfo->update.cyclesSinceLastChange > timeBetweenChange)
    {
      logDebug("goomInfo->update.cyclesSinceLastChange = {} > {} = timeBetweenChange",
               goomInfo->update.cyclesSinceLastChange, timeBetweenChange);
      pzfd = &goomInfo->update.zoomFilterData;
      goomInfo->update.cyclesSinceLastChange = 0;
    }
    else
    {
      goomInfo->update.cyclesSinceLastChange++;
    }
  }

  if (pzfd)
  {
    logDebug("pzfd->mode = {}", pzfd->mode);
  }
}

static void applyTentaclesIfRequired(PluginInfo* goomInfo)
{
  if (goomInfo->curGDrawables.count(GoomDrawable::tentacles))
  {
    logDebug("goomInfo->curGDrawables tentacles is set.");
    stats.doTentacles();
  }
  goomInfo->tentacles_fx.apply(&goomInfo->tentacles_fx, goomInfo->p2, goomInfo->p1, goomInfo);
}

static void applyStarsIfRequired(PluginInfo* goomInfo)
{
  if (!goomInfo->curGDrawables.count(GoomDrawable::stars))
  {
    return;
  }

  logDebug("goomInfo->curGDrawables stars is set.");
  stats.doStars();
  goomInfo->star_fx.apply(&goomInfo->star_fx, goomInfo->p2, goomInfo->p1, goomInfo);
}

void displayText(PluginInfo* goomInfo, const char* songTitle, const char* message, const float fps)
{
  // Le message
  update_message(goomInfo, message);

  if (fps > 0)
  {
    char text[256];
    sprintf(text, "%2.0f fps", fps);
    goom_draw_text(goomInfo->p1, goomInfo->screen.width, goomInfo->screen.height, 10, 24, text, 1,
                   0);
  }

  // Le titre
  if (songTitle)
  {
    strncpy(goomInfo->update.titleText, songTitle, 1023);
    goomInfo->update.titleText[1023] = 0;
    goomInfo->update.timeOfTitleDisplay = 200;
  }

  if (goomInfo->update.timeOfTitleDisplay)
  {
    goom_draw_text(goomInfo->p1, goomInfo->screen.width, goomInfo->screen.height,
                   static_cast<int>(goomInfo->screen.width / 2),
                   static_cast<int>(goomInfo->screen.height / 2 + 7), goomInfo->update.titleText,
                   static_cast<float>(190 - goomInfo->update.timeOfTitleDisplay) / 10.0f, 1);
    goomInfo->update.timeOfTitleDisplay--;
    if (goomInfo->update.timeOfTitleDisplay < 4)
    {
      goom_draw_text(goomInfo->p2, goomInfo->screen.width, goomInfo->screen.height,
                     static_cast<int>(goomInfo->screen.width / 2),
                     static_cast<int>(goomInfo->screen.height / 2 + 7), goomInfo->update.titleText,
                     static_cast<float>(190 - goomInfo->update.timeOfTitleDisplay) / 10.0f, 1);
    }
  }
}

static void stopRequest(PluginInfo* goomInfo)
{
  logDebug("goomInfo->update.stop_lines = {},"
           " goomInfo->curGDrawables.count(GoomDrawable::scope) = {}",
           goomInfo->update.stop_lines, goomInfo->curGDrawables.count(GoomDrawable::scope));

  float param1 = 0;
  float param2 = 0;
  float amplitude = 0;
  uint32_t couleur = 0;
  LineType mode;
  chooseGoomLine(goomInfo, &param1, &param2, &couleur, &mode, &amplitude, 1);
  couleur = getBlackLineColor();

  switchGoomLines(goomInfo->gmline1, mode, param1, amplitude, couleur);
  switchGoomLines(goomInfo->gmline2, mode, param2, amplitude, couleur);
  goomInfo->update.stop_lines &= 0x0fff;
}

/* arret aleatore.. changement de mode de ligne..
  */
static void stopRandomLineChangeMode(PluginInfo* goomInfo)
{
  if (goomInfo->update.lineMode != goomInfo->update.drawLinesDuration)
  {
    goomInfo->update.lineMode--;
    if (goomInfo->update.lineMode == -1)
    {
      goomInfo->update.lineMode = 0;
    }
  }
  else if ((goomInfo->cycle % 80 == 0) && goomEvent.happens(GoomEvent::reduceLineMode) &&
           goomInfo->update.lineMode)
  {
    goomInfo->update.lineMode--;
  }

  if ((goomInfo->cycle % 120 == 0) && goomEvent.happens(GoomEvent::updateLineMode) &&
      goomInfo->curGDrawables.count(GoomDrawable::scope))
  {
    if (goomInfo->update.lineMode == 0)
    {
      goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
    }
    else if (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration)
    {
      goomInfo->update.lineMode--;

      float param1 = 0;
      float param2 = 0;
      float amplitude = 0;
      uint32_t couleur1 = 0;
      LineType mode;
      chooseGoomLine(goomInfo, &param1, &param2, &couleur1, &mode, &amplitude,
                     goomInfo->update.stop_lines);

      uint32_t couleur2 = getRandomLineColor(goomInfo);
      if (goomInfo->update.stop_lines)
      {
        goomInfo->update.stop_lines--;
        if (goomEvent.happens(GoomEvent::changeLineToBlack))
        {
          couleur2 = couleur1 = getBlackLineColor();
        }
      }

      logDebug("goomInfo->update.lineMode = {} == {} = goomInfo->update.drawLinesDuration",
               goomInfo->update.lineMode, goomInfo->update.drawLinesDuration);
      switchGoomLines(goomInfo->gmline1, mode, param1, amplitude, couleur1);
      switchGoomLines(goomInfo->gmline2, mode, param2, amplitude, couleur2);
    }
  }
}

static void displayLines(PluginInfo* goomInfo,
                         const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  if (!goomInfo->curGDrawables.count(GoomDrawable::lines))
  {
    return;
  }

  logDebug("goomInfo->curGDrawables lines is set.");

  stats.doLines();

  goomInfo->gmline2->power = goomInfo->gmline1->power;

  drawGoomLines(goomInfo, goomInfo->gmline1, data[0], goomInfo->p2);
  drawGoomLines(goomInfo, goomInfo->gmline2, data[1], goomInfo->p2);

  if (((goomInfo->cycle % 121) == 9) && goomEvent.happens(GoomEvent::changeGoomLine) &&
      ((goomInfo->update.lineMode == 0) ||
       (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration)))
  {
    logDebug("goomInfo->cycle % 121 etc.: goomInfo->cycle = {}, rand1_3 = ?", goomInfo->cycle);
    float param1 = 0;
    float param2 = 0;
    float amplitude = 0;
    uint32_t couleur1 = 0;
    LineType mode;
    chooseGoomLine(goomInfo, &param1, &param2, &couleur1, &mode, &amplitude,
                   goomInfo->update.stop_lines);

    uint32_t couleur2 = getRandomLineColor(goomInfo);
    if (goomInfo->update.stop_lines)
    {
      goomInfo->update.stop_lines--;
      if (goomEvent.happens(GoomEvent::changeLineToBlack))
      {
        couleur2 = couleur1 = getBlackLineColor();
      }
    }
    switchGoomLines(goomInfo->gmline1, mode, param1, amplitude, couleur1);
    switchGoomLines(goomInfo->gmline2, mode, param2, amplitude, couleur2);
  }
}

static void bigBreakIfMusicIsCalm(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->sound.speedvar = {:.2}, goomInfo->update.zoomFilterData.vitesse = {}, "
           "goomInfo->cycle = {}",
           goomInfo->sound.speedvar, goomInfo->update.zoomFilterData.vitesse, goomInfo->cycle);
  if ((goomInfo->sound.speedvar < 0.01f) &&
      (goomInfo->update.zoomFilterData.vitesse < (stopSpeed - 4)) && (goomInfo->cycle % 16 == 0))
  {
    logDebug("goomInfo->sound.speedvar = {:.2}", goomInfo->sound.speedvar);
    bigBreak(goomInfo, pzfd);
  }
}

static void bigBreak(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse += 3;
  goomInfo->update.zoomFilterData.pertedec = 8;
  goomInfo->update.goomvar = 0;
}

static void forceFilterMode(PluginInfo* goomInfo, const int forceMode, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  (*pzfd)->mode = static_cast<ZoomFilterMode>(forceMode - 1);
}

static void lowerSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse++;
}

static void stopDecrementing(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.pertedec = 8;
}

static float largeSoundFactor(const SoundInfo* sound)
{
  float largefactor = sound->speedvar / 150.0f + sound->volume / 1.5f;
  if (largefactor > 1.5f)
  {
    largefactor = 1.5f;
  }
  return largefactor;
}

static void updateDecayRecay(PluginInfo* goomInfo)
{
  goomInfo->update.decay_ifs--;
  if (goomInfo->update.decay_ifs > 0)
  {
    goomInfo->update.ifs_incr += 2;
  }
  if (goomInfo->update.decay_ifs == 0)
  {
    goomInfo->update.ifs_incr = 0;
  }

  if (goomInfo->update.recay_ifs)
  {
    goomInfo->update.ifs_incr -= 2;
    goomInfo->update.recay_ifs--;
    if ((goomInfo->update.recay_ifs == 0) && (goomInfo->update.ifs_incr <= 0))
    {
      goomInfo->update.ifs_incr = 1;
    }
  }
}

static void bigUpdateIfNotLocked(PluginInfo* goomInfo, ZoomFilterData** pzfd, const int forceMode)
{
  logDebug("goomInfo->update.lockvar = {}", goomInfo->update.lockvar);
  if (goomInfo->update.lockvar == 0)
  {
    logDebug("goomInfo->update.lockvar = 0");
    bigUpdate(goomInfo, pzfd, forceMode);
  }
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
}

static void forceFilterModeIfSet(PluginInfo* goomInfo, ZoomFilterData** pzfd, const int forceMode)
{
  constexpr size_t numFilterFx = static_cast<size_t>(ZoomFilterMode::_size);

  logDebug("forceMode = {}", forceMode);
  if ((forceMode > 0) && (size_t(forceMode) <= numFilterFx))
  {
    logDebug("forceMode = {} <= numFilterFx = {}.", forceMode, numFilterFx);
    forceFilterMode(goomInfo, forceMode, pzfd);
  }
  if (forceMode == -1)
  {
    pzfd = nullptr;
  }
}

static void stopIfRequested(PluginInfo* goomInfo)
{
  logDebug("goomInfo->update.stop_lines = {}, goomInfo->curGState->scope = {}",
           goomInfo->update.stop_lines, goomInfo->curGDrawables.count(GoomDrawable::scope));
  if ((goomInfo->update.stop_lines & 0xf000) || !goomInfo->curGDrawables.count(GoomDrawable::scope))
  {
    stopRequest(goomInfo);
  }
}

static void displayLinesIfInAGoom(PluginInfo* goomInfo,
                                  const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  logDebug("goomInfo->update.lineMode = {} != 0 || goomInfo->sound.timeSinceLastGoom = {}",
           goomInfo->update.lineMode, goomInfo->sound.timeSinceLastGoom);
  if ((goomInfo->update.lineMode != 0) || (goomInfo->sound.timeSinceLastGoom < 5))
  {
    logDebug("goomInfo->update.lineMode = {} != 0 || goomInfo->sound.timeSinceLastGoom = {} < 5",
             goomInfo->update.lineMode, goomInfo->sound.timeSinceLastGoom);

    displayLines(goomInfo, data);
  }
}

static void applyIfsIfRequired(PluginInfo* goomInfo)
{
  logDebug("goomInfo->update.ifs_incr = {}", goomInfo->update.ifs_incr);
  if (goomInfo->update.ifs_incr > 0)
  {
    logDebug("goomInfo->update.ifs_incr = {} > 0", goomInfo->update.ifs_incr);
    stats.doIFS();
    goomInfo->ifs_fx.apply(&goomInfo->ifs_fx, goomInfo->p2, goomInfo->p1, goomInfo);
  }
}

static void regularlyLowerTheSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->update.zoomFilterData.vitesse = {}, goomInfo->cycle = {}",
           goomInfo->update.zoomFilterData.vitesse, goomInfo->cycle);
  if ((goomInfo->cycle % 73 == 0) && (goomInfo->update.zoomFilterData.vitesse < (stopSpeed - 5)))
  {
    logDebug("goomInfo->cycle % 73 = 0 && dgoomInfo->update.zoomFilterData.vitesse = {} < {} - 5, ",
             goomInfo->cycle, goomInfo->update.zoomFilterData.vitesse, stopSpeed);
    lowerSpeed(goomInfo, pzfd);
  }
}

static void stopDecrementingAfterAWhile(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->cycle = {}, goomInfo->update.zoomFilterData.pertedec = {}", goomInfo->cycle,
           goomInfo->update.zoomFilterData.pertedec);
  if ((goomInfo->cycle % 101 == 0) && (goomInfo->update.zoomFilterData.pertedec == 7))
  {
    logDebug("goomInfo->cycle % 101 = 0 && dgoomInfo->update.zoomFilterData.pertedec = 7, ",
             goomInfo->cycle, goomInfo->update.zoomFilterData.vitesse);
    stopDecrementing(goomInfo, pzfd);
  }
}

static void pointFilter(PluginInfo* goomInfo,
                        Pixel* pix1,
                        const uint32_t color,
                        const float t1,
                        const float t2,
                        const float t3,
                        const float t4,
                        const uint32_t cycle)
{
  const uint32_t xOffset = static_cast<uint32_t>(t1 * cos(static_cast<float>(cycle) / t3));
  const uint32_t yOffset = static_cast<uint32_t>(t2 * sin(static_cast<float>(cycle) / t4));
  const uint32_t x = goomInfo->screen.width / 2 + xOffset;
  const uint32_t y = goomInfo->screen.height / 2 + yOffset;

  const uint32_t widthLess2 = goomInfo->screen.width - 2;
  const uint32_t heightLess2 = goomInfo->screen.height - 2;

  if ((x > 1) && (y > 1) && (x < widthLess2) && (y < heightLess2))
  {
    setPixelRGB(pix1, x + 1, y, goomInfo->screen.width, color);
    setPixelRGB(pix1, x, y + 1, goomInfo->screen.width, color);
    setPixelRGB(pix1, x + 1, y + 1, goomInfo->screen.width, WHITE);
    setPixelRGB(pix1, x + 2, y + 1, goomInfo->screen.width, color);
    setPixelRGB(pix1, x + 1, y + 2, goomInfo->screen.width, color);
  }
}
