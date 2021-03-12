/**
* file: goom_control.c (onetime goom_core.c)
 * author: Jean-Christophe Hoelt (which is not so proud of it)
 *
 * Contains the core of goom's work.
 *
 * (c)2000-2003, by iOS-software.
 *
 *	- converted to C++14 2021-02-01 (glk)
 *
 */

#include "goom_control.h"

#include "convolve_fx.h"
#include "filter_control.h"
#include "filters.h"
#include "flying_stars_fx.h"
#include "goom_dots_fx.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/goomrand.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/parallel_utils.h"
#include "goomutils/random_colormaps.h"
#include "goomutils/small_image_bitmaps.h"
#include "goomutils/strutils.h"
#include "ifs_dancers_fx.h"
#include "image_fx.h"
#include "lines_fx.h"
#include "stats/goom_stats.h"
#include "tentacles_fx.h"
#include "text_draw.h"

#include <array>
#include <cmath>
#include <cstdint>
#if __cplusplus > 201402L
#include <filesystem>
#endif
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#if __cplusplus > 201402L
#include <variant>
#endif
#include <vector>

//#define SHOW_STATE_TEXT_ON_SCREEN

namespace GOOM
{

using namespace GOOM::UTILS;

// TODO: put that as variable in PluginInfo
constexpr int32_t MAX_TIME_BETWEEN_ZOOM_EFFECTS_CHANGE = 200;

enum class GoomDrawable
{
  IFS = 0,
  DOTS,
  TENTACLES,
  STARS,
  LINES,
  SCOPE,
  FAR_SCOPE,
  IMAGE,
  _SIZE
};

class GoomEvents
{
public:
  GoomEvents() noexcept;
  ~GoomEvents() = default;
  GoomEvents(const GoomEvents&) noexcept = delete;
  GoomEvents(GoomEvents&&) noexcept = delete;
  auto operator=(const GoomEvents&) -> GoomEvents& = delete;
  auto operator=(GoomEvents&&) -> GoomEvents& = delete;

  enum class GoomEvent
  {
    CHANGE_FILTER_MODE = 0,
    CHANGE_STATE,
    CHANGE_TO_MEGA_LENT_MODE,
    CHANGE_LINE_CIRCLE_AMPLITUDE,
    CHANGE_LINE_CIRCLE_PARAMS,
    CHANGE_H_LINE_PARAMS,
    CHANGE_V_LINE_PARAMS,
    REDUCE_LINE_MODE,
    UPDATE_LINE_MODE,
    CHANGE_LINE_TO_BLACK,
    CHANGE_GOOM_LINE,
    FILTER_REVERSE_ON,
    FILTER_REVERSE_OFF_AND_STOP_SPEED,
    FILTER_VITESSE_STOP_SPEED_MINUS1,
    FILTER_VITESSE_STOP_SPEED,
    FILTER_CHANGE_VITESSE_AND_TOGGLE_REVERSE,
    TURN_OFF_NOISE,
    FILTER_ZERO_H_PLANE_EFFECT,
    FILTER_TOGGLE_ROTATION,
    FILTER_INCREASE_ROTATION,
    FILTER_DECREASE_ROTATION,
    FILTER_STOP_ROTATION,
    IFS_RENEW,
    CHANGE_BLOCKY_WAVY_TO_ON,
    CHANGE_ZOOM_FILTER_ALLOW_OVEREXPOSED_TO_ON,
    _SIZE // must be last - gives number of enums
  };

  auto Happens(GoomEvent event) -> bool;
  auto GetRandomLineTypeEvent() const -> LinesFx::LineType;

private:
  static constexpr size_t NUM_GOOM_EVENTS = static_cast<size_t>(GoomEvent::_SIZE);

  struct WeightedEvent
  {
    GoomEvent event;
    // m out of n
    uint32_t m;
    uint32_t outOf;
  };

  //@formatter:off
  // clang-format off
  const std::array<WeightedEvent, NUM_GOOM_EVENTS> m_weightedEvents{{
    { /*.event = */GoomEvent::CHANGE_FILTER_MODE,                         /*.m = */1, /*.outOf = */ 20 },
    { /*.event = */GoomEvent::CHANGE_STATE,                               /*.m = */1, /*.outOf = */  2 },
    { /*.event = */GoomEvent::CHANGE_TO_MEGA_LENT_MODE,                   /*.m = */1, /*.outOf = */700 },
    { /*.event = */GoomEvent::CHANGE_LINE_CIRCLE_AMPLITUDE,               /*.m = */1, /*.outOf = */  3 },
    { /*.event = */GoomEvent::CHANGE_LINE_CIRCLE_PARAMS,                  /*.m = */1, /*.outOf = */  2 },
    { /*.event = */GoomEvent::CHANGE_H_LINE_PARAMS,                       /*.m = */3, /*.outOf = */  4 },
    { /*.event = */GoomEvent::CHANGE_V_LINE_PARAMS,                       /*.m = */2, /*.outOf = */  3 },
    { /*.event = */GoomEvent::REDUCE_LINE_MODE,                           /*.m = */1, /*.outOf = */  5 },
    { /*.event = */GoomEvent::UPDATE_LINE_MODE,                           /*.m = */1, /*.outOf = */  4 },
    { /*.event = */GoomEvent::CHANGE_LINE_TO_BLACK,                       /*.m = */1, /*.outOf = */  2 },
    { /*.event = */GoomEvent::CHANGE_GOOM_LINE,                           /*.m = */1, /*.outOf = */  3 },
    { /*.event = */GoomEvent::FILTER_REVERSE_ON,                          /*.m = */1, /*.outOf = */ 10 },
    { /*.event = */GoomEvent::FILTER_REVERSE_OFF_AND_STOP_SPEED,          /*.m = */1, /*.outOf = */  5 },
    { /*.event = */GoomEvent::FILTER_VITESSE_STOP_SPEED_MINUS1,           /*.m = */1, /*.outOf = */  5 },
    { /*.event = */GoomEvent::FILTER_VITESSE_STOP_SPEED,                  /*.m = */1, /*.outOf = */ 10 },
    { /*.event = */GoomEvent::FILTER_CHANGE_VITESSE_AND_TOGGLE_REVERSE,   /*.m = */1, /*.outOf = */ 20 },
    { /*.event = */GoomEvent::FILTER_ZERO_H_PLANE_EFFECT,                 /*.m = */1, /*.outOf = */  2 },
    { /*.event = */GoomEvent::TURN_OFF_NOISE,                             /*.m = */5, /*.outOf = */  5 },
    { /*.event = */GoomEvent::FILTER_TOGGLE_ROTATION,                     /*.m =*/ 5, /*.outOf = */ 40 },
    { /*.event = */GoomEvent::FILTER_INCREASE_ROTATION,                   /*.m =*/10, /*.outOf = */ 40 },
    { /*.event = */GoomEvent::FILTER_DECREASE_ROTATION,                   /*.m =*/35, /*.outOf = */ 40 },
    { /*.event = */GoomEvent::FILTER_STOP_ROTATION,                       /*.m =*/20, /*.outOf = */ 40 },
    { /*.event = */GoomEvent::IFS_RENEW,                                  /*.m = */2, /*.outOf = */  3 },
    { /*.event = */GoomEvent::CHANGE_BLOCKY_WAVY_TO_ON,                   /*.m = */1, /*.outOf = */ 20 },
    { /*.event = */GoomEvent::CHANGE_ZOOM_FILTER_ALLOW_OVEREXPOSED_TO_ON, /*.m = */8, /*.outOf = */ 10 },
  }};

  const std::array<std::pair<LinesFx::LineType, size_t>, LinesFx::NUM_LINE_TYPES> m_weightedLineEvents{{
    { LinesFx::LineType::circle, 10 },
    { LinesFx::LineType::hline,   2 },
    { LinesFx::LineType::vline,   2 },
  }};
  // clang-format on
  //@formatter:on

  const Weights<LinesFx::LineType> m_lineTypeWeights;
};

using GoomEvent = GoomEvents::GoomEvent;

class GoomStates
{
public:
  using DrawablesState = std::unordered_set<GoomDrawable>;

  GoomStates();

  [[nodiscard]] auto IsCurrentlyDrawable(GoomDrawable drawable) const -> bool;
  [[nodiscard]] auto GetCurrentStateIndex() const -> size_t;
  [[nodiscard]] auto GetCurrentDrawables() const -> DrawablesState;
  [[nodiscard]] auto GetCurrentBuffSettings(GoomDrawable theFx) const -> FXBuffSettings;

  void DoRandomStateChange();

private:
  struct DrawableInfo
  {
    GoomDrawable fx;
    FXBuffSettings buffSettings;
  };
  using DrawableInfoArray = std::vector<DrawableInfo>;
  struct State
  {
    uint32_t weight;
    const DrawableInfoArray drawables;
  };
  using WeightedStatesArray = std::vector<State>;
  static const WeightedStatesArray STATES;
  static auto GetWeightedStates(const WeightedStatesArray& theStates)
      -> std::vector<std::pair<uint16_t, size_t>>;
  const Weights<uint16_t> m_weightedStates;
  size_t m_currentStateIndex{0};
};

//@formatter:off
// clang-format off
const GoomStates::WeightedStatesArray GoomStates::STATES{{
  {
    /*.weight = */1,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IMAGE,     /*.buffSettings = */{ /*.buffIntensity = */0.8, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */1,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */1,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */1,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */1.0, /*.allowOverexposed = */false  } },
    }},
  },
  {
    /*.weight = */1,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */false } },
  }},
 },
  {
    /*.weight = */200,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.1, /*.allowOverexposed = */false } },
    }},
  },
  {
    /*.weight = */200,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */false } },
    }},
  },
  {
    /*.weight = */200,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.1, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::IMAGE,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */false } },
    }},
  },
  {
    /*.weight = */200,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::SCOPE,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */100,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */false } },
    }},
  },
  {
    /*.weight = */20,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */false } },
    }},
  },
  {
    /*.weight = */60,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.4, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::SCOPE,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */70,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */false } },
    }},
  },
  {
    /*.weight = */70,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::IFS,       /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true } },
    }},
  },
  {
    /*.weight = */40,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::SCOPE,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */40,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.2, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::SCOPE,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */100,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.4, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.4, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::IMAGE,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */70,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::IMAGE,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */100,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::TENTACLES, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.4, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::IMAGE,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */60,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.4, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.7, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::SCOPE,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */60,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::STARS,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */false } },
      { /*.fx = */GoomDrawable::IMAGE,     /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
    }},
  },
  {
    /*.weight = */60,
    /*.drawables */{{
      { /*.fx = */GoomDrawable::DOTS,      /*.buffSettings = */{ /*.buffIntensity = */0.3, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::LINES,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::SCOPE,     /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
      { /*.fx = */GoomDrawable::FAR_SCOPE, /*.buffSettings = */{ /*.buffIntensity = */0.5, /*.allowOverexposed = */true  } },
    }},
  },
}};
// clang-format on
//@formatter:on

class GoomImageBuffers
{
public:
  GoomImageBuffers() noexcept = default;
  GoomImageBuffers(uint32_t resx, uint32_t resy) noexcept;
  ~GoomImageBuffers() noexcept;
  GoomImageBuffers(const GoomImageBuffers&) noexcept = delete;
  GoomImageBuffers(GoomImageBuffers&&) noexcept = delete;
  auto operator=(const GoomImageBuffers&) -> GoomImageBuffers& = delete;
  auto operator=(GoomImageBuffers&&) -> GoomImageBuffers& = delete;

  void SetResolution(uint32_t resx, uint32_t resy);

  [[nodiscard]] auto GetP1() const -> PixelBuffer& { return *m_p1; }
  [[nodiscard]] auto GetP2() const -> PixelBuffer& { return *m_p2; }

  [[nodiscard]] auto GetOutputBuff() const -> PixelBuffer& { return *m_outputBuff; }
  void SetOutputBuff(PixelBuffer& val) { m_outputBuff = &val; }

  static constexpr size_t MAX_NUM_BUFFS = 2;
  static constexpr size_t MAX_BUFF_INC = MAX_NUM_BUFFS / 2;
  void SetBuffInc(size_t i);
  void RotateBuffers();

private:
  std::vector<std::unique_ptr<PixelBuffer>> m_buffs{};
  PixelBuffer* m_p1{};
  PixelBuffer* m_p2{};
  PixelBuffer* m_outputBuff{};
  size_t m_nextBuff = 0;
  size_t m_buffInc = 1;
  static auto GetPixelBuffs(uint32_t resx, uint32_t resy)
      -> std::vector<std::unique_ptr<PixelBuffer>>;
};

auto GoomImageBuffers::GetPixelBuffs(const uint32_t resx, const uint32_t resy)
    -> std::vector<std::unique_ptr<PixelBuffer>>
{
  std::vector<std::unique_ptr<PixelBuffer>> newBuffs(MAX_NUM_BUFFS);
  for (auto& b : newBuffs)
  {
    b = std::make_unique<PixelBuffer>(resx, resy);
  }
  return newBuffs;
}

GoomImageBuffers::GoomImageBuffers(const uint32_t resx, const uint32_t resy) noexcept
  : m_buffs{GetPixelBuffs(resx, resy)},
    m_p1{m_buffs[0].get()},
    m_p2{m_buffs[1].get()},
    m_nextBuff{MAX_NUM_BUFFS == 2 ? 0 : 2}
{
}

GoomImageBuffers::~GoomImageBuffers() noexcept = default;

void GoomImageBuffers::SetResolution(const uint32_t resx, const uint32_t resy)
{
  m_buffs = GetPixelBuffs(resx, resy);
  m_p1 = m_buffs[0].get();
  m_p2 = m_buffs[1].get();
  m_nextBuff = MAX_NUM_BUFFS == 2 ? 0 : 2;
  m_buffInc = 1;
}

void GoomImageBuffers::SetBuffInc(const size_t i)
{
  if (i < 1 || i > MAX_BUFF_INC)
  {
    throw std::logic_error("Cannot set buff inc: i out of range.");
  }
  m_buffInc = i;
}

void GoomImageBuffers::RotateBuffers()
{
  m_p1 = m_p2;
  m_p2 = m_buffs[m_nextBuff].get();
  m_nextBuff += +m_buffInc;
  if (m_nextBuff >= m_buffs.size())
  {
    m_nextBuff = 0;
  }
}

struct GoomMessage
{
  std::string message;
  uint32_t numberOfLinesInMessage = 0;
  uint32_t affiche = 0;
};

struct GoomVisualFx
{
  GoomVisualFx() noexcept = default;
  explicit GoomVisualFx(Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo) noexcept;

  std::shared_ptr<ConvolveFx> convolve_fx{};
  std::shared_ptr<ZoomFilterFx> zoomFilter_fx{};
  std::shared_ptr<FlyingStarsFx> star_fx{};
  std::shared_ptr<GoomDotsFx> goomDots_fx{};
  std::shared_ptr<IfsDancersFx> ifs_fx{};
  std::shared_ptr<TentaclesFx> tentacles_fx{};
  std::shared_ptr<ImageFx> image_fx{};

  std::vector<std::shared_ptr<IVisualFx>> list{};
};

GoomVisualFx::GoomVisualFx(Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo) noexcept
  : convolve_fx{new ConvolveFx{p, goomInfo}},
    zoomFilter_fx{new ZoomFilterFx{p, goomInfo}},
    star_fx{new FlyingStarsFx{goomInfo}},
    goomDots_fx{new GoomDotsFx{goomInfo}},
    ifs_fx{new IfsDancersFx{goomInfo}},
    tentacles_fx{new TentaclesFx{goomInfo}},
    image_fx{new ImageFx{goomInfo}},
    // clang-format off
    list{
      convolve_fx,
      zoomFilter_fx,
      star_fx,
      ifs_fx,
      goomDots_fx,
      tentacles_fx,
      image_fx,
    }
// clang-format on
{
}

struct GoomData
{
  int32_t lockVar = 0; // pour empecher de nouveaux changements
  int32_t stopLines = 0;
  int32_t updatesSinceLastZoomEffectsChange = 0; // nombre de Cycle Depuis Dernier Changement
  // duree de la transition entre afficher les lignes ou pas
  int32_t drawLinesDuration = LinesFx::MIN_LINE_DURATION;
  int32_t lineMode = LinesFx::MIN_LINE_DURATION; // l'effet lineaire a dessiner

  static constexpr float SWITCH_MULT_AMOUNT = 29.0 / 30.0;
  float switchMult = SWITCH_MULT_AMOUNT;
  static constexpr int SWITCH_INCR_AMOUNT = 0x7f;
  int32_t switchIncr = SWITCH_INCR_AMOUNT;
  uint32_t stateSelectionBlocker = 0;
  int32_t previousZoomSpeed = 128;

  static constexpr int MAX_TITLE_DISPLAY_TIME = 200;
  static constexpr int TIME_TO_SPACE_TITLE_DISPLAY = 100;
  static constexpr int TIME_TO_FADE_TITLE_DISPLAY = 50;
  int32_t timeOfTitleDisplay = 0;
  std::string title{};
};

class GoomControl::GoomControlImpl
{
public:
  GoomControlImpl(uint32_t screenWidth, uint32_t screenHeight, std::string resourcesDirectory);
  ~GoomControlImpl() noexcept;
  GoomControlImpl(const GoomControlImpl&) noexcept = delete;
  GoomControlImpl(GoomControlImpl&&) noexcept = delete;
  auto operator=(const GoomControlImpl&) noexcept -> GoomControlImpl& = delete;
  auto operator=(GoomControlImpl&&) noexcept -> GoomControlImpl& = delete;

  void Swap(GoomControl::GoomControlImpl& other) noexcept = delete;

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);

  [[nodiscard]] auto GetScreenBuffer() const -> PixelBuffer&;
  void SetScreenBuffer(PixelBuffer& buff);
  void SetFontFile(const std::string& f);

  [[nodiscard]] auto GetScreenWidth() const -> uint32_t;
  [[nodiscard]] auto GetScreenHeight() const -> uint32_t;

  void Start();
  void Finish();

  void Update(const AudioSamples& soundData,
              float fps,
              const std::string& songTitle,
              const std::string& message);

private:
  Parallel m_parallel;
  const std::shared_ptr<WritablePluginInfo> m_goomInfo;
  GoomImageBuffers m_imageBuffers{};
  GoomVisualFx m_visualFx{};
  GoomStats m_stats{};
  GoomStates m_states{};
  GoomEvents m_goomEvent{};
  uint32_t m_timeInState = 0;
  uint32_t m_timeWithFilter = 0;
  uint32_t m_cycle = 0;
  FilterControl m_filterControl;
  std::unordered_set<GoomDrawable> m_curGDrawables{};
  GoomMessage m_messageData{};
  GoomData m_goomData;
  TextDraw m_text{};
  const IColorMap* m_textColorMap{};
  Pixel m_textOutlineColor{};
  std::unique_ptr<TextDraw> m_updateMessageText{};

  std::string m_resourcesDirectory{};
  const SmallImageBitmaps m_smallBitmaps;

  bool m_singleBufferDots = true;

  // Line Fx
  LinesFx m_gmline1;
  LinesFx m_gmline2;

  void ProcessAudio(const AudioSamples& soundData) const;

  // Changement d'effet de zoom !
  void ChangeZoomEffect();
  void ApplyZoom();
  void UpdateBuffers();

  void ApplyCurrentStateToSingleBuffer();
  void ApplyCurrentStateToMultipleBuffers();
  void ApplyDotsIfRequired();
  void ApplyDotsToBothBuffersIfRequired();
  void ApplyIfsToBothBuffersIfRequired();
  void ApplyTentaclesToBothBuffersIfRequired();
  void ApplyStarsToBothBuffersIfRequired();
  void ApplyImageIfRequired();

  void DoIfsRenew();

  void SetLockVar(int32_t val);
  void UpdateLockVar();

  void ChangeState();
  void DoChangeState();
  void SetNextState();

  void BigNormalUpdate();
  void MegaLentUpdate();

  void ChangeAllowOverexposed();
  void ChangeBlockyWavy();
  void ChangeHPlaneEffect();
  void ChangeNoise();
  void ChangeRotation();
  void ChangeSwitchValues();
  void ChangeSpeedReverse();
  void ChangeVitesse();
  void ChangeStopSpeeds();

  // on verifie qu'il ne se pas un truc interressant avec le son.
  void ChangeFilterModeIfMusicChanges();
  [[nodiscard]] auto ChangeFilterModeEventHappens() -> bool;
  void ChangeFilterMode();
  void SetNextFilterMode();
  void ChangeMilieu();

  // baisser regulierement la vitesse
  void RegularlyLowerTheSpeed();

  // arret demande
  void StopLinesRequest();
  void StopLinesIfRequested();

  // arret aleatore.. changement de mode de ligne..
  void StopRandomLineChangeMode();

  // arreter de decrémenter au bout d'un certain temps
  void StopDecrementingAfterAWhile();
  void StopDecrementing();

  // tout ceci ne sera fait qu'en cas de non-blocage
  void BigUpdateIfNotLocked();
  void BigUpdate();

  // gros frein si la musique est calme
  void BigBreakIfMusicIsCalm();
  void BigBreak();

  void DisplayLinesIfInAGoom(const AudioSamples& soundData);
  void DisplayLines(const AudioSamples& soundData);
  void ChooseGoomLine(float* param1,
                      float* param2,
                      Pixel* couleur,
                      LinesFx::LineType* mode,
                      float* amplitude,
                      int far);

  void UpdateMessage(const std::string& message);
  void DrawText(const std::string& str, int xPos, int yPos, float spacing, PixelBuffer& buff);
  void DisplayText(const std::string& songTitle, const std::string& message, float fps);
#ifdef SHOW_STATE_TEXT_ON_SCREEN
  void DisplayStateText();
#endif
};

auto GoomControl::GetRandSeed() -> uint64_t
{
  return GOOM::UTILS::GetRandSeed();
}

void GoomControl::SetRandSeed(const uint64_t seed)
{
  logDebug("Set goom seed = {}.", seed);
  GOOM::UTILS::SetRandSeed(seed);
}

GoomControl::GoomControl(const uint32_t resx, const uint32_t resy, std::string resourcesDirectory)
  : m_controller{new GoomControlImpl{resx, resy, resourcesDirectory}}
{
}

GoomControl::~GoomControl() noexcept = default;

auto GoomControl::GetResourcesDirectory() const -> const std::string&
{
  return m_controller->GetResourcesDirectory();
}

void GoomControl::SetResourcesDirectory(const std::string& dirName)
{
  m_controller->SetResourcesDirectory(dirName);
}

void GoomControl::SetScreenBuffer(PixelBuffer& buffer)
{
  m_controller->SetScreenBuffer(buffer);
}

void GoomControl::Start()
{
  m_controller->Start();
}

void GoomControl::Finish()
{
  m_controller->Finish();
}

void GoomControl::Update(const AudioSamples& soundData,
                         const float fps,
                         const std::string& songTitle,
                         const std::string& message)
{
  m_controller->Update(soundData, fps, songTitle, message);
}

static const Pixel RED_LINE = GetRedLineColor();
static const Pixel GREEN_LINE = GetGreenLineColor();
static const Pixel BLACK_LINE = GetBlackLineColor();

GoomControl::GoomControlImpl::GoomControlImpl(const uint32_t screenWidth,
                                              const uint32_t screenHeight,
                                              std::string resourcesDirectory)
  : m_parallel{-1}, // max cores - 1
    m_goomInfo{new WritablePluginInfo{screenWidth, screenHeight}},
    m_imageBuffers{screenWidth, screenHeight},
    m_visualFx{m_parallel, std::const_pointer_cast<const PluginInfo>(
                               std::dynamic_pointer_cast<PluginInfo>(m_goomInfo))},
    m_filterControl{m_goomInfo},
    m_goomData{},
    m_text{screenWidth, screenHeight},
    m_resourcesDirectory{std::move(resourcesDirectory)},
    m_smallBitmaps{m_resourcesDirectory},
    m_gmline1{std::const_pointer_cast<const PluginInfo>(
                  std::dynamic_pointer_cast<PluginInfo>(m_goomInfo)),
              LinesFx::LineType::hline,
              static_cast<float>(screenHeight),
              BLACK_LINE,
              LinesFx::LineType::circle,
              0.4F * static_cast<float>(screenHeight),
              GREEN_LINE},
    m_gmline2{std::const_pointer_cast<const PluginInfo>(
                  std::dynamic_pointer_cast<PluginInfo>(m_goomInfo)),
              LinesFx::LineType::hline,
              0,
              BLACK_LINE,
              LinesFx::LineType::circle,
              0.2F * static_cast<float>(screenHeight),
              RED_LINE}
{
  logDebug("Initialize goom: screenWidth = {}, screenHeight = {}.", screenWidth, screenHeight);
}

GoomControl::GoomControlImpl::~GoomControlImpl() noexcept = default;

auto GoomControl::GoomControlImpl::GetResourcesDirectory() const -> const std::string&
{
  return m_resourcesDirectory;
}

void GoomControl::GoomControlImpl::SetResourcesDirectory(const std::string& dirName)
{
#if __cplusplus > 201402L
  if (!std::filesystem::exists(dirName))
  {
    throw std::runtime_error(std20::format("Could not find directory \"{}\".", dirName));
  }
#endif

  m_resourcesDirectory = dirName;
}

auto GoomControl::GoomControlImpl::GetScreenBuffer() const -> PixelBuffer&
{
  return m_imageBuffers.GetOutputBuff();
}

void GoomControl::GoomControlImpl::SetScreenBuffer(PixelBuffer& buffer)
{
  m_imageBuffers.SetOutputBuff(buffer);
}

void GoomControl::GoomControlImpl::SetFontFile(const std::string& filename)
{
  m_stats.SetFontFileUsed(filename);

  m_text.SetFontFile(filename);
  m_text.SetFontSize(35);
  m_text.SetOutlineWidth(2);
  m_text.SetAlignment(TextDraw::TextAlignment::left);
}

auto GoomControl::GoomControlImpl::GetScreenWidth() const -> uint32_t
{
  return m_goomInfo->GetScreenInfo().width;
}

auto GoomControl::GoomControlImpl::GetScreenHeight() const -> uint32_t
{
  return m_goomInfo->GetScreenInfo().height;
}

inline auto GoomControl::GoomControlImpl::ChangeFilterModeEventHappens() -> bool
{
  return m_goomEvent.Happens(GoomEvent::CHANGE_FILTER_MODE);
}

void GoomControl::GoomControlImpl::Start()
{
  if (m_resourcesDirectory.empty())
  {
    throw std::logic_error("Cannot start Goom - resource directory not set.");
  }

  SetFontFile(m_resourcesDirectory + PATH_SEP + FONTS_DIR + PATH_SEP + "verdana.ttf");

  m_timeInState = 0;
  m_timeWithFilter = 0;

  m_curGDrawables = m_states.GetCurrentDrawables();

  m_stats.Reset();
  m_stats.SetStateStartValue(m_states.GetCurrentStateIndex());
  m_stats.SetZoomFilterStartValue(m_filterControl.GetFilterSettings().mode);
  m_stats.SetSeedStartValue(GetRandSeed());

  // TODO MAKE line a visual FX
  m_gmline1.SetResourcesDirectory(m_resourcesDirectory);
  m_gmline1.SetSmallImageBitmaps(m_smallBitmaps);
  m_gmline1.Start();
  m_gmline2.SetResourcesDirectory(m_resourcesDirectory);
  m_gmline2.SetSmallImageBitmaps(m_smallBitmaps);
  m_gmline2.Start();

  m_visualFx.goomDots_fx->SetSmallImageBitmaps(m_smallBitmaps);
  m_visualFx.ifs_fx->SetSmallImageBitmaps(m_smallBitmaps);
  m_visualFx.star_fx->SetSmallImageBitmaps(m_smallBitmaps);

  m_filterControl.SetResourcesDirectory(m_resourcesDirectory);
  m_filterControl.Start();

  SetNextFilterMode();
  m_visualFx.zoomFilter_fx->ChangeFilterSettings(m_filterControl.GetFilterSettings());
  m_stats.DoChangeFilterMode(m_filterControl.GetFilterSettings().mode);

  for (auto& v : m_visualFx.list)
  {
    v->SetResourcesDirectory(m_resourcesDirectory);
    v->Start();
  }

  DoChangeState();
}

void GoomControl::GoomControlImpl::Finish()
{
  for (auto& v : m_visualFx.list)
  {
    v->Finish();
  }

  m_stats.SetStateLastValue(m_states.GetCurrentStateIndex());
  m_stats.SetZoomFilterLastValue(&m_filterControl.GetFilterSettings());
  m_stats.SetSeedLastValue(GetRandSeed());
  m_stats.SetNumThreadsUsedValue(m_parallel.GetNumThreadsUsed());

  m_stats.Log(GoomStats::LogStatsValue);

  m_goomInfo->GetSoundInfo().Log(GoomStats::LogStatsValue);

  for (auto& v : m_visualFx.list)
  {
    v->Log(GoomStats::LogStatsValue);
  }
}

void GoomControl::GoomControlImpl::Update(const AudioSamples& soundData,
                                          const float fps,
                                          const std::string& songTitle,
                                          const std::string& message)
{
  m_stats.UpdateChange(m_states.GetCurrentStateIndex(), m_filterControl.GetFilterSettings().mode);

  m_timeInState++;
  m_timeWithFilter++;

  // Elargissement de l'intervalle d'évolution des points!
  // Calcul du deplacement des petits points ...
  // Widening of the interval of evolution of the points!
  // Calculation of the displacement of small points ...

  ProcessAudio(soundData);

  UpdateLockVar();

  ChangeFilterModeIfMusicChanges();
  BigUpdateIfNotLocked();
  BigBreakIfMusicIsCalm();
  RegularlyLowerTheSpeed();
  StopDecrementingAfterAWhile();
  ChangeZoomEffect();

  ApplyCurrentStateToSingleBuffer();

  ApplyZoom();

  ApplyCurrentStateToMultipleBuffers();

  // Gestion du Scope - Scope management
  StopLinesIfRequested();
  StopRandomLineChangeMode();
  DisplayLinesIfInAGoom(soundData);

  UpdateBuffers();

#ifdef SHOW_STATE_TEXT_ON_SCREEN
  DisplayStateText();
#endif
  DisplayText(songTitle, message, fps);

  m_cycle++;
}

void GoomControl::GoomControlImpl::ProcessAudio(const AudioSamples& soundData) const
{
  /* ! etude du signal ... */
  m_goomInfo->ProcessSoundSample(soundData);
}

void GoomControl::GoomControlImpl::ApplyCurrentStateToSingleBuffer()
{
  // applyIfsIfRequired();
  ApplyDotsIfRequired();
}

void GoomControl::GoomControlImpl::ApplyCurrentStateToMultipleBuffers()
{
  ApplyDotsToBothBuffersIfRequired();
  ApplyIfsToBothBuffersIfRequired();
  ApplyTentaclesToBothBuffersIfRequired();
  ApplyStarsToBothBuffersIfRequired();
  //  ApplyImageIfRequired();
}

void GoomControl::GoomControlImpl::UpdateBuffers()
{
  // affichage et swappage des buffers...
  m_visualFx.convolve_fx->Convolve(m_imageBuffers.GetP2(), m_imageBuffers.GetOutputBuff());

  m_imageBuffers.SetBuffInc(GetRandInRange(1U, GoomImageBuffers::MAX_BUFF_INC + 1));
  m_imageBuffers.RotateBuffers();
}

void GoomControl::GoomControlImpl::UpdateLockVar()
{
  m_stats.DoLockChange();

  m_goomData.lockVar--;
  if (m_goomData.lockVar < 0)
  {
    m_goomData.lockVar = 0;
  }
  /* note pour ceux qui n'ont pas suivis : le lockVar permet d'empecher un */
  /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
  // -- Note for those who have not followed: the lockVar prevents a change
  // of state of the plugin just after another change of state.
}

void GoomControl::GoomControlImpl::SetLockVar(const int32_t val)
{
  m_stats.DoLockChange();

  m_goomData.lockVar = val;
}

void GoomControl::GoomControlImpl::ChangeFilterModeIfMusicChanges()
{
  if ((m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() == 0) ||
      (m_goomData.updatesSinceLastZoomEffectsChange > MAX_TIME_BETWEEN_ZOOM_EFFECTS_CHANGE))
  {
    logDebug("Try to change the filter mode.");
    if (ChangeFilterModeEventHappens())
    {
      ChangeFilterMode();
    }
  }
  logDebug("sound GetTimeSinceLastGoom() = {}", m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom());
}

void GoomControl::GoomControlImpl::ChangeFilterMode()
{
  m_stats.DoChangeFilterMode();

  SetNextFilterMode();

  m_stats.DoChangeFilterMode(m_filterControl.GetFilterSettings().mode);

  DoIfsRenew();
}

void GoomControl::GoomControlImpl::SetNextFilterMode()
{
  m_filterControl.SetRandomFilterSettings();
  m_filterControl.ChangeMilieu();
  m_curGDrawables = m_states.GetCurrentDrawables();
}

void GoomControl::GoomControlImpl::ChangeState()
{
  m_stats.DoStateChangeRequest();

  if (m_goomData.stateSelectionBlocker)
  {
    m_goomData.stateSelectionBlocker--;
  }
  else if (m_goomEvent.Happens(GoomEvent::CHANGE_STATE))
  {
    m_goomData.stateSelectionBlocker = 3;
    DoChangeState();
  }
}

void GoomControl::GoomControlImpl::DoChangeState()
{
  m_stats.DoStateChange(m_timeInState);

  const auto oldGDrawables = m_states.GetCurrentDrawables();

  SetNextState();

  m_stats.DoStateChange(m_states.GetCurrentStateIndex(), m_timeInState);

  m_curGDrawables = m_states.GetCurrentDrawables();
  m_timeInState = 0;

  if (m_states.IsCurrentlyDrawable(GoomDrawable::IFS))
  {
#if __cplusplus <= 201402L
    if (oldGDrawables.find(GoomDrawable::IFS) == oldGDrawables.end())
#else
    if (!oldGDrawables.contains(GoomDrawable::IFS))
#endif
    {
      m_visualFx.ifs_fx->Init();
    }
    else if (m_goomEvent.Happens(GoomEvent::IFS_RENEW))
    {
      DoIfsRenew();
    }
    m_visualFx.ifs_fx->UpdateIncr();
  }

  if (!m_states.IsCurrentlyDrawable(GoomDrawable::SCOPE))
  {
    m_goomData.stopLines = 0xf000 & 5;
  }
  if (!m_states.IsCurrentlyDrawable(GoomDrawable::FAR_SCOPE))
  {
    m_goomData.stopLines = 0;
    m_goomData.lineMode = m_goomData.drawLinesDuration;
  }
}

void GoomControl::GoomControlImpl::SetNextState()
{
  const size_t oldStateIndex = m_states.GetCurrentStateIndex();

  for (size_t numTry = 0; numTry < 10; numTry++)
  {
    m_states.DoRandomStateChange();
    if (oldStateIndex != m_states.GetCurrentStateIndex())
    {
      // Only pick a different state.
      break;
    }
  }
}

void GoomControl::GoomControlImpl::BigBreakIfMusicIsCalm()
{
  if ((m_goomInfo->GetSoundInfo().GetSpeed() < 0.01F) &&
      (m_filterControl.GetVitesseSetting().GetVitesse() < (Vitesse::STOP_SPEED - 4)) &&
      (m_cycle % 16 == 0))
  {
    BigBreak();
  }
}

void GoomControl::GoomControlImpl::BigBreak()
{
  m_stats.DoBigBreak();

  m_filterControl.GetVitesseSetting().GoSlowerBy(3);
}

void GoomControl::GoomControlImpl::BigUpdateIfNotLocked()
{
  if (m_goomData.lockVar == 0)
  {
    BigUpdate();
  }
}

void GoomControl::GoomControlImpl::BigUpdate()
{
  m_stats.DoBigUpdate();

  // Reperage de goom (acceleration forte de l'acceleration du volume).
  // Coup de boost de la vitesse si besoin.
  // Goom tracking (strong acceleration of volume acceleration).
  // Speed boost if needed.
  if (m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() == 0)
  {
    BigNormalUpdate();
  }

  // mode mega-lent
  if (m_goomEvent.Happens(GoomEvent::CHANGE_TO_MEGA_LENT_MODE))
  {
    MegaLentUpdate();
  }
}

void GoomControl::GoomControlImpl::BigNormalUpdate()
{
  m_stats.DoBigNormalUpdate();

  SetLockVar(50);

  ChangeState();
  ChangeSpeedReverse();
  ChangeStopSpeeds();
  ChangeRotation();
  ChangeMilieu();
  ChangeNoise();
  ChangeBlockyWavy();
  ChangeAllowOverexposed();
  ChangeVitesse();
  ChangeSwitchValues();

  m_singleBufferDots = ProbabilityOfMInN(1, 2);
}

void GoomControl::GoomControlImpl::MegaLentUpdate()
{
  m_stats.DoMegaLentChange();

  SetLockVar(m_goomData.lockVar + 50);

  m_filterControl.GetVitesseSetting().SetVitesse(Vitesse::STOP_SPEED - 1);
  m_goomData.switchIncr = GoomData::SWITCH_INCR_AMOUNT;
  m_goomData.switchMult = 1.0F;
}

void GoomControl::GoomControlImpl::ChangeMilieu()
{
  m_stats.DoChangeMilieu();

  m_filterControl.ChangeMilieu();
  ChangeHPlaneEffect();
}

void GoomControl::GoomControlImpl::ChangeHPlaneEffect()
{
  if ((m_filterControl.GetFilterSettings().middleX == 1) ||
      (m_filterControl.GetFilterSettings().middleX == GetScreenWidth() - 1))
  {
    m_filterControl.SetVPlaneEffectSetting(0);
    if (m_goomEvent.Happens(GoomEvent::FILTER_ZERO_H_PLANE_EFFECT))
    {
      m_filterControl.SetHPlaneEffectSetting(0);
    }
  }
}

void GoomControl::GoomControlImpl::ChangeVitesse()
{
  const auto goFasterVal = static_cast<int32_t>(
      std::lround(3.5F * std::log10(1.0F + 500.0F * m_goomInfo->GetSoundInfo().GetSpeed())));
  const int32_t newVitesse = Vitesse::STOP_SPEED - goFasterVal;
  const int32_t oldVitesse = m_filterControl.GetVitesseSetting().GetVitesse();

  if (newVitesse >= oldVitesse)
  {
    return;
  }

  m_stats.DoChangeVitesse();

  // on accelere
  if (((newVitesse < (Vitesse::STOP_SPEED - 7)) && (oldVitesse < (Vitesse::STOP_SPEED - 6)) &&
       (m_cycle % 3 == 0)) ||
      m_goomEvent.Happens(GoomEvent::FILTER_CHANGE_VITESSE_AND_TOGGLE_REVERSE))
  {
    m_filterControl.GetVitesseSetting().SetVitesse(Vitesse::STOP_SPEED - 1);
    m_filterControl.GetVitesseSetting().ToggleReverseVitesse();
  }
  else
  {
    m_filterControl.GetVitesseSetting().SetVitesse(static_cast<int32_t>(std::lround(
        stdnew::lerp(static_cast<float>(oldVitesse), static_cast<float>(newVitesse), 0.4F))));
  }

  SetLockVar(m_goomData.lockVar + 50);
}

void GoomControl::GoomControlImpl::ChangeSpeedReverse()
{
  // Retablir le zoom avant.
  // Restore zoom in.

  if ((m_filterControl.GetVitesseSetting().GetReverseVitesse()) && (m_cycle % 13 != 0) &&
      m_goomEvent.Happens(GoomEvent::FILTER_REVERSE_OFF_AND_STOP_SPEED))
  {
    m_stats.DoChangeReverseSpeedOff();
    m_filterControl.GetVitesseSetting().SetReverseVitesse(false);
    m_filterControl.GetVitesseSetting().SetVitesse(Vitesse::STOP_SPEED - 2);
    SetLockVar(75);
  }
  if (m_goomEvent.Happens(GoomEvent::FILTER_REVERSE_ON))
  {
    m_stats.DoChangeReverseSpeedOn();
    m_filterControl.GetVitesseSetting().SetReverseVitesse(true);
    SetLockVar(100);
  }
}

void GoomControl::GoomControlImpl::ChangeStopSpeeds()
{
  if (m_goomEvent.Happens(GoomEvent::FILTER_VITESSE_STOP_SPEED_MINUS1))
  {
    m_stats.DoReduceStopSpeed();
    m_filterControl.GetVitesseSetting().SetVitesse(Vitesse::STOP_SPEED - 1);
  }
  if (m_goomEvent.Happens(GoomEvent::FILTER_VITESSE_STOP_SPEED))
  {
    m_stats.DoIncreaseStopSpeed();
    m_filterControl.GetVitesseSetting().SetVitesse(Vitesse::STOP_SPEED);
  }
}

void GoomControl::GoomControlImpl::ChangeSwitchValues()
{
  if (m_goomData.lockVar > 150)
  {
    m_stats.DoChangeSwitchValues();
    m_goomData.switchIncr = GoomData::SWITCH_INCR_AMOUNT;
    m_goomData.switchMult = 1.0F;
  }
}

void GoomControl::GoomControlImpl::ChangeNoise()
{
  if (m_goomEvent.Happens(GoomEvent::TURN_OFF_NOISE))
  {
    m_stats.DoTurnOffNoise();
    m_filterControl.SetNoisifySetting(false);
  }
  else
  {
    m_stats.DoNoise();
    m_filterControl.SetNoisifySetting(true);
    m_filterControl.SetNoiseFactorSetting(1.0);
    SetLockVar(2 * m_goomData.lockVar);
  }
}

void GoomControl::GoomControlImpl::ChangeRotation()
{
  if (m_goomEvent.Happens(GoomEvent::FILTER_STOP_ROTATION))
  {
    m_stats.DoStopRotation();
    m_filterControl.SetRotateSetting(0.0F);
  }
  else if (m_goomEvent.Happens(GoomEvent::FILTER_DECREASE_ROTATION))
  {
    m_stats.DoDecreaseRotation();
    m_filterControl.MultiplyRotateSetting(0.9F);
  }
  else if (m_goomEvent.Happens(GoomEvent::FILTER_INCREASE_ROTATION))
  {
    m_stats.DoIncreaseRotation();
    m_filterControl.MultiplyRotateSetting(1.1F);
  }
  else if (m_goomEvent.Happens(GoomEvent::FILTER_TOGGLE_ROTATION))
  {
    m_stats.DoToggleRotation();
    m_filterControl.ToggleRotateSetting();
  }
}

void GoomControl::GoomControlImpl::ChangeBlockyWavy()
{
  if (!m_goomEvent.Happens(GoomEvent::CHANGE_BLOCKY_WAVY_TO_ON))
  {
    m_stats.DoBlockyWavyOff();
    m_filterControl.SetBlockyWavySetting(false);
  }
  else
  {
    m_stats.DoBlockyWavyOn();
    m_filterControl.SetBlockyWavySetting(true);
  }
}

void GoomControl::GoomControlImpl::ChangeAllowOverexposed()
{
  if (!m_goomEvent.Happens(GoomEvent::CHANGE_ZOOM_FILTER_ALLOW_OVEREXPOSED_TO_ON))
  {
    m_stats.DoZoomFilterAllowOverexposedOff();
    m_visualFx.zoomFilter_fx->SetBuffSettings(
        {/*.buffIntensity = */ 0.5, /*.allowOverexposed = */ false});
  }
  else
  {
    m_stats.DoZoomFilterAllowOverexposedOn();
    m_visualFx.zoomFilter_fx->SetBuffSettings(
        {/*.buffIntensity = */ 0.5, /*.allowOverexposed = */ true});
  }
}

/* Changement d'effet de zoom !
 */
void GoomControl::GoomControlImpl::ChangeZoomEffect()
{
  ChangeBlockyWavy();
  ChangeAllowOverexposed();

  if (!m_filterControl.GetSettingsChangedSinceMark())
  {
    if (m_goomData.updatesSinceLastZoomEffectsChange > MAX_TIME_BETWEEN_ZOOM_EFFECTS_CHANGE)
    {
      m_goomData.updatesSinceLastZoomEffectsChange = 0;

      ChangeRotation();
      DoIfsRenew();
    }
    else
    {
      m_goomData.updatesSinceLastZoomEffectsChange++;
    }
  }
  else
  {
    m_goomData.updatesSinceLastZoomEffectsChange = 0;
    m_goomData.switchIncr = GoomData::SWITCH_INCR_AMOUNT;

    int diff = m_filterControl.GetVitesseSetting().GetVitesse() - m_goomData.previousZoomSpeed;
    if (diff < 0)
    {
      diff = -diff;
    }

    if (diff > 2)
    {
      m_goomData.switchIncr *= (diff + 2) / 2;
    }
    m_goomData.previousZoomSpeed = m_filterControl.GetVitesseSetting().GetVitesse();
    m_goomData.switchMult = 1.0F;

    if (m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() == 0 &&
        m_goomInfo->GetSoundInfo().GetTotalGoomsInCurrentCycle() < 2)
    {
      m_goomData.switchIncr = 0;
      m_goomData.switchMult = GoomData::SWITCH_MULT_AMOUNT;

      ChangeRotation();
      DoIfsRenew();
    }
  }
}

void GoomControl::GoomControlImpl::StopDecrementingAfterAWhile()
{
  if (m_cycle % 101 == 0)
  {
    StopDecrementing();
  }
}

void GoomControl::GoomControlImpl::StopDecrementing()
{
  // TODO What used to be here???
}

void GoomControl::GoomControlImpl::RegularlyLowerTheSpeed()
{
  if ((m_cycle % 73 == 0) &&
      (m_filterControl.GetVitesseSetting().GetVitesse() < (Vitesse::STOP_SPEED - 5)))
  {
    m_stats.DoLowerVitesse();
    m_filterControl.GetVitesseSetting().GoSlowerBy(1);
  }
}

void GoomControl::GoomControlImpl::ApplyZoom()
{
  if (m_filterControl.GetSettingsChangedSinceMark())
  {
    if (m_filterControl.GetFilterSettings().mode !=
        m_visualFx.zoomFilter_fx->GetFilterSettings().mode)
    {
      m_stats.DoApplyChangeFilterSettings(m_timeWithFilter);
      m_timeWithFilter = 0;
    }
    m_visualFx.zoomFilter_fx->ChangeFilterSettings(m_filterControl.GetFilterSettings());
    m_filterControl.ClearUnchangedMark();
  }

  uint32_t numClipped = 0;
  m_visualFx.zoomFilter_fx->ZoomFilterFastRgb(m_imageBuffers.GetP1(), m_imageBuffers.GetP2(),
                                              m_goomData.switchIncr, m_goomData.switchMult,
                                              numClipped);

  m_stats.SetLastNumClipped(numClipped);
  if (numClipped > m_goomInfo->GetScreenInfo().size / 100)
  {
    m_stats.TooManyClipped();
    m_goomData.switchIncr = -m_goomData.switchIncr;
    m_goomData.switchMult = 1.0F;
    // ChangeFilterMode();
  }

  if (m_filterControl.GetFilterSettings().noisify)
  {
    m_filterControl.SetNoiseFactorSetting(m_filterControl.GetFilterSettings().noiseFactor * 0.999F);
  }
}

void GoomControl::GoomControlImpl::ApplyDotsIfRequired()
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::DOTS) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::DOTS))
#endif
  {
    return;
  }

  if (!m_singleBufferDots)
  {
    return;
  }

  m_stats.DoDots();
  m_visualFx.goomDots_fx->SetBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::DOTS));
  m_visualFx.goomDots_fx->Apply(m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::ApplyDotsToBothBuffersIfRequired()
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::DOTS) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::DOTS))
#endif
  {
    return;
  }

  if (m_singleBufferDots)
  {
    return;
  }

  m_stats.DoDots();
  m_visualFx.goomDots_fx->SetBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::DOTS));
  m_visualFx.goomDots_fx->Apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::ApplyIfsToBothBuffersIfRequired()
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::IFS) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::IFS))
#endif
  {
    m_visualFx.ifs_fx->ApplyNoDraw();
    return;
  }

  m_stats.DoIfs();
  m_visualFx.ifs_fx->SetBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::IFS));
  m_visualFx.ifs_fx->Apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::DoIfsRenew()
{
  m_visualFx.ifs_fx->Renew();
  m_stats.DoIfsRenew();
}

void GoomControl::GoomControlImpl::ApplyTentaclesToBothBuffersIfRequired()
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::TENTACLES) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::TENTACLES))
#endif
  {
    m_visualFx.tentacles_fx->ApplyNoDraw();
    return;
  }

  m_stats.DoTentacles();
  m_visualFx.tentacles_fx->SetBuffSettings(
      m_states.GetCurrentBuffSettings(GoomDrawable::TENTACLES));
  m_visualFx.tentacles_fx->Apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::ApplyStarsToBothBuffersIfRequired()
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::STARS) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::STARS))
#endif
  {
    return;
  }

  logDebug("curGDrawables stars is set.");
  m_stats.DoStars();
  m_visualFx.star_fx->SetBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::STARS));
  //  visualFx.star_fx->apply(imageBuffers.getP2(), imageBuffers.getP1());
  m_visualFx.star_fx->Apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::ApplyImageIfRequired()
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::IMAGE) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::IMAGE))
#endif
  {
    return;
  }

  //m_stats.DoImage();
  m_visualFx.image_fx->SetBuffSettings(m_states.GetCurrentBuffSettings(GoomDrawable::IMAGE));
  //  m_visualFx.image_fx->Apply(m_imageBuffers.GetP2());
  m_visualFx.image_fx->Apply(m_imageBuffers.GetP2(), m_imageBuffers.GetP1());
}

void GoomControl::GoomControlImpl::StopLinesIfRequested()
{
  if ((m_goomData.stopLines & 0xf000) || !m_states.IsCurrentlyDrawable(GoomDrawable::SCOPE))
  {
    StopLinesRequest();
  }
}

void GoomControl::GoomControlImpl::StopLinesRequest()
{
  if (!m_gmline1.CanResetDestLine() || !m_gmline2.CanResetDestLine())
  {
    return;
  }

  m_stats.DoStopLinesRequest();

  float param1 = 0.0;
  float param2 = 0.0;
  float amplitude = 0.0;
  Pixel couleur{};
  LinesFx::LineType mode;
  ChooseGoomLine(&param1, &param2, &couleur, &mode, &amplitude, 1);
  couleur = GetBlackLineColor();

  m_gmline1.ResetDestLine(mode, param1, amplitude, couleur);
  m_gmline2.ResetDestLine(mode, param2, amplitude, couleur);
  m_stats.DoSwitchLines();
  m_goomData.stopLines &= 0x0fff;
}

void GoomControl::GoomControlImpl::ChooseGoomLine(float* param1,
                                                  float* param2,
                                                  Pixel* couleur,
                                                  LinesFx::LineType* mode,
                                                  float* amplitude,
                                                  const int far)
{
  *amplitude = 1.0F;
  *mode = m_goomEvent.GetRandomLineTypeEvent();

  switch (*mode)
  {
    case LinesFx::LineType::circle:
      if (far)
      {
        *param1 = *param2 = 0.47F;
        *amplitude = 0.8F;
        break;
      }
      if (m_goomEvent.Happens(GoomEvent::CHANGE_LINE_CIRCLE_AMPLITUDE))
      {
        *param1 = *param2 = 0.0F;
        *amplitude = 3.0F;
      }
      else if (m_goomEvent.Happens(GoomEvent::CHANGE_LINE_CIRCLE_PARAMS))
      {
        *param1 = 0.40F * static_cast<float>(GetScreenHeight());
        *param2 = 0.22F * static_cast<float>(GetScreenHeight());
      }
      else
      {
        *param1 = *param2 = static_cast<float>(GetScreenHeight()) * 0.35F;
      }
      break;
    case LinesFx::LineType::hline:
      if (m_goomEvent.Happens(GoomEvent::CHANGE_H_LINE_PARAMS) || far)
      {
        *param1 = static_cast<float>(GetScreenHeight()) / 7.0F;
        *param2 = 6.0F * static_cast<float>(GetScreenHeight()) / 7.0F;
      }
      else
      {
        *param1 = *param2 = static_cast<float>(GetScreenHeight()) / 2.0F;
        *amplitude = 2.0F;
      }
      break;
    case LinesFx::LineType::vline:
      if (m_goomEvent.Happens(GoomEvent::CHANGE_V_LINE_PARAMS) || far)
      {
        *param1 = static_cast<float>(GetScreenWidth()) / 7.0F;
        *param2 = 6.0F * static_cast<float>(GetScreenWidth()) / 7.0F;
      }
      else
      {
        *param1 = *param2 = static_cast<float>(GetScreenWidth()) / 2.0F;
        *amplitude = 1.5F;
      }
      break;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }

  m_stats.DoChangeLineColor();
  *couleur = m_gmline1.GetRandomLineColor();
}

/* arret aleatore.. changement de mode de ligne..
  */
void GoomControl::GoomControlImpl::StopRandomLineChangeMode()
{
  if (m_goomData.lineMode != m_goomData.drawLinesDuration)
  {
    m_goomData.lineMode--;
    if (m_goomData.lineMode == -1)
    {
      m_goomData.lineMode = 0;
    }
  }
  else if ((m_cycle % 80 == 0) && m_goomEvent.Happens(GoomEvent::REDUCE_LINE_MODE) &&
           m_goomData.lineMode)
  {
    m_goomData.lineMode--;
  }

  if ((m_cycle % 120 == 0) && m_goomEvent.Happens(GoomEvent::UPDATE_LINE_MODE) &&
#if __cplusplus <= 201402L
      m_curGDrawables.find(GoomDrawable::SCOPE) != m_curGDrawables.end())
#else
      m_curGDrawables.contains(GoomDrawable::SCOPE))
#endif
  {
    if (m_goomData.lineMode == 0)
    {
      m_goomData.lineMode = m_goomData.drawLinesDuration;
    }
    else if (m_goomData.lineMode == m_goomData.drawLinesDuration && m_gmline1.CanResetDestLine() &&
             m_gmline2.CanResetDestLine())
    {
      m_goomData.lineMode--;

      float param1 = 0.0;
      float param2 = 0.0;
      float amplitude = 0.0;
      Pixel color1{};
      LinesFx::LineType mode;
      ChooseGoomLine(&param1, &param2, &color1, &mode, &amplitude, m_goomData.stopLines);

      Pixel color2 = m_gmline2.GetRandomLineColor();
      if (m_goomData.stopLines)
      {
        m_goomData.stopLines--;
        if (m_goomEvent.Happens(GoomEvent::CHANGE_LINE_TO_BLACK))
        {
          color2 = color1 = GetBlackLineColor();
        }
      }

      logDebug("goomData.lineMode = {} == {} = goomData.drawLinesDuration", m_goomData.lineMode,
               m_goomData.drawLinesDuration);
      m_gmline1.ResetDestLine(mode, param1, amplitude, color1);
      m_gmline2.ResetDestLine(mode, param2, amplitude, color2);
      m_stats.DoSwitchLines();
    }
  }
}

void GoomControl::GoomControlImpl::DisplayLines(const AudioSamples& soundData)
{
#if __cplusplus <= 201402L
  if (m_curGDrawables.find(GoomDrawable::LINES) == m_curGDrawables.end())
#else
  if (!m_curGDrawables.contains(GoomDrawable::LINES))
#endif
  {
    return;
  }

  logDebug("curGDrawables lines is set.");

  m_stats.DoLines();

  m_gmline2.SetPower(m_gmline1.GetPower());

  const std::vector<int16_t>& audioSample = soundData.GetSample(0);
  m_gmline1.DrawLines(audioSample, m_imageBuffers.GetP1(), m_imageBuffers.GetP2());
  m_gmline2.DrawLines(audioSample, m_imageBuffers.GetP1(), m_imageBuffers.GetP2());
  //  gmline2.drawLines(soundData.GetSample(1), imageBuffers.getP1(), imageBuffers.getP2());

  if (((m_cycle % 121) == 9) && m_goomEvent.Happens(GoomEvent::CHANGE_GOOM_LINE) &&
      ((m_goomData.lineMode == 0) || (m_goomData.lineMode == m_goomData.drawLinesDuration)) &&
      m_gmline1.CanResetDestLine() && m_gmline2.CanResetDestLine())
  {
    logDebug("cycle % 121 etc.: goomInfo->cycle = {}, rand1_3 = ?", m_cycle);
    float param1 = 0.0;
    float param2 = 0.0;
    float amplitude = 0.0;
    Pixel color1{};
    LinesFx::LineType mode;
    ChooseGoomLine(&param1, &param2, &color1, &mode, &amplitude, m_goomData.stopLines);

    Pixel color2 = m_gmline2.GetRandomLineColor();
    if (m_goomData.stopLines)
    {
      m_goomData.stopLines--;
      if (m_goomEvent.Happens(GoomEvent::CHANGE_LINE_TO_BLACK))
      {
        color2 = color1 = GetBlackLineColor();
      }
    }
    m_gmline1.ResetDestLine(mode, param1, amplitude, color1);
    m_gmline2.ResetDestLine(mode, param2, amplitude, color2);
  }
}

void GoomControl::GoomControlImpl::DisplayLinesIfInAGoom(const AudioSamples& soundData)
{
  logDebug("goomData.lineMode = {} != 0 || sound GetTimeSinceLastGoom() = {}", m_goomData.lineMode,
           m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom());
  if ((m_goomData.lineMode != 0) || (m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom() < 5))
  {
    logDebug("goomData.lineMode = {} != 0 || sound GetTimeSinceLastGoom() = {} < 5",
             m_goomData.lineMode, m_goomInfo->GetSoundInfo().GetTimeSinceLastGoom());

    DisplayLines(soundData);
  }
}

#ifdef SHOW_STATE_TEXT_ON_SCREEN

void GoomControl::GoomControlImpl::DisplayStateText()
{
  std::string message = "";

  message += std20::format("State: {}\n", m_states.GetCurrentStateIndex());
  message += std20::format("Filter: {}\n",
                           EnumToString(m_visualFx.zoomFilter_fx->GetFilterSettings().mode));
  message += std20::format("Filter Settings Pending: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettingsArePending());

  message += std20::format("middleX: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().middleX);
  message += std20::format("middleY: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().middleY);

  message += std20::format("switchIncr: {}\n", m_goomData.switchIncr);
  message += std20::format("switchMult: {}\n", m_goomData.switchMult);

  message += std20::format("vitesse: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().vitesse.GetVitesse());
  message += std20::format("previousZoomSpeed: {}\n", m_goomData.previousZoomSpeed);
  message += std20::format(
      "reverse: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().vitesse.GetReverseVitesse());
  message +=
      std20::format("relative speed: {}\n",
                    m_visualFx.zoomFilter_fx->GetFilterSettings().vitesse.GetRelativeSpeed());

  message += std20::format("hPlaneEffect: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().hPlaneEffect);
  message += std20::format("hPlaneEffectAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().hPlaneEffectAmplitude);
  message += std20::format("vPlaneEffect: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().vPlaneEffect);
  message += std20::format("vPlaneEffectAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().vPlaneEffectAmplitude);
  message +=
      std20::format("rotateSpeed: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().rotateSpeed);
  message +=
      std20::format("tanEffect: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().tanEffect);

  message += std20::format("noisify: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().noisify);
  message +=
      std20::format("noiseFactor: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().noiseFactor);

  message +=
      std20::format("blockyWavy: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().blockyWavy);

  message +=
      std20::format("waveEffectType: {}\n",
                    EnumToString(m_visualFx.zoomFilter_fx->GetFilterSettings().waveEffectType));
  message += std20::format("waveFreqFactor: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().waveFreqFactor);
  message += std20::format("waveAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().waveAmplitude);

  message += std20::format("amuletAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().amuletAmplitude);
  message += std20::format("crystalBallAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().crystalBallAmplitude);
  message += std20::format("scrunchAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().scrunchAmplitude);
  message += std20::format("speedwayAmplitude: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().speedwayAmplitude);

  message +=
      std20::format("hypercosEffect: {}\n",
                    EnumToString(m_visualFx.zoomFilter_fx->GetFilterSettings().hypercosEffect));
  message += std20::format("hypercosFreqX: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().hypercosFreqX);
  message += std20::format("hypercosFreqY: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().hypercosFreqY);
  message += std20::format("hypercosAmplitudeX: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().hypercosAmplitudeX);
  message += std20::format("hypercosAmplitudeY: {}\n",
                           m_visualFx.zoomFilter_fx->GetFilterSettings().hypercosAmplitudeY);

  message +=
      std20::format("updatesSinceLastChange: {}\n", m_goomData.updatesSinceLastZoomEffectsChange);
  //  message += std20::format("GetGeneralSpeed: {}\n", m_visualFx.zoomFilter_fx->GetGeneralSpeed());
  //  message += std20::format("pertedec: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().pertedec);
  //  message +=
  //    std20::format("waveEffect: {}\n", m_visualFx.zoomFilter_fx->GetFilterSettings().waveEffect);
  //  message += std20::format("lineMode: {}\n", m_goomData.lineMode);
  //  message += std20::format("lockVar: {}\n", m_goomData.lockVar);
  //  message += std20::format("stopLines: {}\n", m_goomData.stopLines);

  UpdateMessage(message);
}

#endif

void GoomControl::GoomControlImpl::DisplayText(const std::string& songTitle,
                                               const std::string& message,
                                               const float fps)
{
  UpdateMessage(message);

  if (fps > 0.0)
  {
    const std::string text = std20::format("{.0f} fps", fps);
    DrawText(text, 10, 24, 0.0, m_imageBuffers.GetOutputBuff());
  }

  if (!songTitle.empty())
  {
    m_stats.SetSongTitle(songTitle);
    m_goomData.title = songTitle;
    m_goomData.timeOfTitleDisplay = GoomData::MAX_TITLE_DISPLAY_TIME;
  }

  if (m_goomData.timeOfTitleDisplay > 0)
  {
    if (m_goomData.timeOfTitleDisplay == GoomData::MAX_TITLE_DISPLAY_TIME)
    {
      m_textColorMap = &(RandomColorMaps{}.GetRandomColorMap(ColorMapGroup::DIVERGING_BLACK));
      m_textOutlineColor = Pixel::WHITE;
    }
    const auto xPos = static_cast<int>(0.085F * static_cast<float>(GetScreenWidth()));
    const auto yPos = static_cast<int>(0.300F * static_cast<float>(GetScreenHeight()));
    const auto timeGone =
        static_cast<float>(GoomData::TIME_TO_SPACE_TITLE_DISPLAY - m_goomData.timeOfTitleDisplay);
    const float spacing = std::max(0.0F, 0.056F * timeGone);
    const int xExtra = static_cast<int>(
        std::max(0.0F, 3.0F * timeGone / static_cast<float>(m_goomData.timeOfTitleDisplay)));

    DrawText(m_goomData.title, xPos + xExtra, yPos, spacing, m_imageBuffers.GetOutputBuff());

    m_goomData.timeOfTitleDisplay--;

    if (m_goomData.timeOfTitleDisplay < GoomData::TIME_TO_FADE_TITLE_DISPLAY)
    {
      DrawText(m_goomData.title, xPos, yPos, spacing, m_imageBuffers.GetP1());
    }
  }
}

void GoomControl::GoomControlImpl::DrawText(const std::string& str,
                                            const int xPos,
                                            const int yPos,
                                            const float spacing,
                                            PixelBuffer& buffer)
{
  const float t = static_cast<float>(m_goomData.timeOfTitleDisplay) /
                  static_cast<float>(GoomData::MAX_TITLE_DISPLAY_TIME);
  const float brightness = t;

  const IColorMap& charColorMap =
      m_goomData.timeOfTitleDisplay > GoomData::TIME_TO_SPACE_TITLE_DISPLAY
          ? RandomColorMaps{}.GetRandomColorMap(ColorMapGroup::DIVERGING)
          : RandomColorMaps{}.GetRandomColorMap(/*ColorMapGroup::diverging*/);
  const auto lastTextIndex = static_cast<float>(str.size() - 1);
  //  const ColorMap& colorMap2 = colorMaps.getColorMap(colordata::ColorMapName::Blues);
  //  const Pixel fontColor = m_textColorMap->GetColor(t);
  const Pixel outlineColor = m_textOutlineColor;
  static GammaCorrection s_gammaCorrect{4.2, 0.01};
  const auto getFontColor = [&](const size_t textIndexOfChar, float x, float y, float width,
                                float height) {
    const float tChar = 1.0F - static_cast<float>(textIndexOfChar) / lastTextIndex;
    const Pixel fontColor = m_textColorMap->GetColor(y / static_cast<float>(height));
    const Pixel charColor = charColorMap.GetColor(tChar);
    return s_gammaCorrect.GetCorrection(
        brightness, IColorMap::GetColorMix(fontColor, charColor, x / static_cast<float>(width)));
  };
  const auto getOutlineFontColor =
      [&]([[maybe_unused]] size_t textIndexOfChar, [[maybe_unused]] float x,
          [[maybe_unused]] float y, [[maybe_unused]] float width, [[maybe_unused]] float height) {
        return s_gammaCorrect.GetCorrection(brightness, outlineColor);
      };

  //  CALL UP TO PREPARE ONCE ONLY
  m_text.SetText(str);
  m_text.SetFontColorFunc(getFontColor);
  m_text.SetOutlineFontColorFunc(getOutlineFontColor);
  m_text.SetCharSpacing(spacing);
  m_text.Prepare();
  m_text.Draw(xPos, yPos, buffer);
}

/*
 * Met a jour l'affichage du message defilant
 */
void GoomControl::GoomControlImpl::UpdateMessage(const std::string& message)
{
  constexpr int FONT_SIZE = 10;
  constexpr int VERTICAL_SPACING = 10;
  constexpr int LINE_HEIGHT = FONT_SIZE + VERTICAL_SPACING;
  constexpr int Y_START = 50;

  if (!message.empty())
  {
    m_messageData.message = message;
    const std::vector<std::string> msgLines = SplitString(m_messageData.message, "\n");
    m_messageData.numberOfLinesInMessage = msgLines.size();
    m_messageData.affiche = 20 + LINE_HEIGHT * m_messageData.numberOfLinesInMessage;
  }
  if (m_messageData.affiche)
  {
    if (m_updateMessageText == nullptr)
    {
      const auto getFontColor = []([[maybe_unused]] const size_t textIndexOfChar,
                                   [[maybe_unused]] float x, [[maybe_unused]] float y,
                                   [[maybe_unused]] float width,
                                   [[maybe_unused]] float height) { return Pixel::WHITE; };
      const auto getOutlineFontColor =
          []([[maybe_unused]] const size_t textIndexOfChar, [[maybe_unused]] float x,
             [[maybe_unused]] float y, [[maybe_unused]] float width,
             [[maybe_unused]] float height) { return Pixel{0xfafafafaU}; };
      m_updateMessageText = std::make_unique<TextDraw>(m_goomInfo->GetScreenInfo().width,
                                                       m_goomInfo->GetScreenInfo().height);
      m_updateMessageText->SetFontFile(m_text.GetFontFile());
      m_updateMessageText->SetFontSize(FONT_SIZE);
      m_updateMessageText->SetOutlineWidth(1);
      m_updateMessageText->SetAlignment(TextDraw::TextAlignment::left);
      m_updateMessageText->SetFontColorFunc(getFontColor);
      m_updateMessageText->SetOutlineFontColorFunc(getOutlineFontColor);
    }
    const std::vector<std::string> msgLines = SplitString(m_messageData.message, "\n");
    for (size_t i = 0; i < msgLines.size(); i++)
    {
      const auto yPos = static_cast<int>(Y_START + m_messageData.affiche -
                                         (m_messageData.numberOfLinesInMessage - i) * LINE_HEIGHT);
      m_updateMessageText->SetText(msgLines[i]);
      m_updateMessageText->Prepare();
      m_updateMessageText->Draw(50, yPos, m_imageBuffers.GetOutputBuff());
    }
    m_messageData.affiche--;
  }
}

GoomEvents::GoomEvents() noexcept
  : m_lineTypeWeights{{m_weightedLineEvents.begin(), m_weightedLineEvents.end()}}
{
}

inline auto GoomEvents::Happens(const GoomEvent event) -> bool
{
  const WeightedEvent& weightedEvent = m_weightedEvents[static_cast<size_t>(event)];
  return ProbabilityOfMInN(weightedEvent.m, weightedEvent.outOf);
}

inline auto GoomEvents::GetRandomLineTypeEvent() const -> LinesFx::LineType
{
  return m_lineTypeWeights.GetRandomWeighted();
}

GoomStates::GoomStates() : m_weightedStates{GetWeightedStates(STATES)}
{
  DoRandomStateChange();
}

inline auto GoomStates::IsCurrentlyDrawable(const GoomDrawable drawable) const -> bool
{
#if __cplusplus <= 201402L
  return GetCurrentDrawables().find(drawable) != GetCurrentDrawables().end();
#else
  return GetCurrentDrawables().contains(drawable);
#endif
}

inline auto GoomStates::GetCurrentStateIndex() const -> size_t
{
  return m_currentStateIndex;
}

inline auto GoomStates::GetCurrentDrawables() const -> GoomStates::DrawablesState
{
  GoomStates::DrawablesState currentDrawables{};
  for (const auto d : STATES[m_currentStateIndex].drawables)
  {
    currentDrawables.insert(d.fx);
  }
  return currentDrawables;
}

auto GoomStates::GetCurrentBuffSettings(const GoomDrawable theFx) const -> FXBuffSettings
{
  for (const auto& d : STATES[m_currentStateIndex].drawables)
  {
    if (d.fx == theFx)
    {
      return d.buffSettings;
    }
  }
  return FXBuffSettings{};
}

inline void GoomStates::DoRandomStateChange()
{
  m_currentStateIndex = static_cast<size_t>(m_weightedStates.GetRandomWeighted());
}

auto GoomStates::GetWeightedStates(const GoomStates::WeightedStatesArray& theStates)
    -> std::vector<std::pair<uint16_t, size_t>>
{
  std::vector<std::pair<uint16_t, size_t>> weightedVals(theStates.size());
  for (size_t i = 0; i < theStates.size(); i++)
  {
    weightedVals[i] = std::make_pair(i, theStates[i].weight);
  }
  return weightedVals;
}

} // namespace GOOM
