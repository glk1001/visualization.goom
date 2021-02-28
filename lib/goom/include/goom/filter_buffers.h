#ifndef VISUALIZATION_GOOM_FILTER_BUFFERS_H
#define VISUALIZATION_GOOM_FILTER_BUFFERS_H

#include "goom_graphic.h"
#include "v2d.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace GOOM
{

namespace UTILS
{
class Parallel;
} // namespace UTILS

class PluginInfo;
class PixelBuffer;

class ZoomFilterBuffers
{
public:
  static constexpr int32_t DIM_FILTER_COEFFS = 16;
  static constexpr size_t NUM_NEIGHBOR_COEFFS = 4;
  using NeighborhoodCoeffArray = union
  {
    std::array<uint8_t, NUM_NEIGHBOR_COEFFS> c;
    uint32_t intVal = 0;
  };
  using FilterCoeff2dArray =
      std::array<std::array<NeighborhoodCoeffArray, DIM_FILTER_COEFFS>, DIM_FILTER_COEFFS>;
  using NeighborhoodPixelArray = std::array<Pixel, NUM_NEIGHBOR_COEFFS>;

  ZoomFilterBuffers(UTILS::Parallel& p, const std::shared_ptr<const PluginInfo>& goomInfo);
  ZoomFilterBuffers(const ZoomFilterBuffers&) noexcept = delete;
  ZoomFilterBuffers(ZoomFilterBuffers&&) noexcept = delete;
  ~ZoomFilterBuffers() noexcept;

  auto operator=(const ZoomFilterBuffers&) -> ZoomFilterBuffers& = delete;
  auto operator=(ZoomFilterBuffers&&) -> ZoomFilterBuffers& = delete;

  using GetZoomVectorFunc = std::function<V2dFlt(float xNormalized, float yNormalized)>;
  void SetGetZoomVectorFunc(const GetZoomVectorFunc& f);

  auto GetTranDiffFactor() const -> int32_t;
  void SetTranDiffFactor(int32_t tranDiffFactor);

  static auto GetMaxTranDiffFactor() -> int32_t;

  // TODO Use V2dInt
  auto GetXBuffMidPoint() const -> int32_t;
  void SetXBuffMidPoint(int32_t xBuffMidPoint);
  auto GetYBuffMidPoint() const -> int32_t;
  void SetYBuffMidPoint(int32_t yBuffMidPoint);

  void Start();

  void SettingsChanged();

  auto GetZoomBufferSrceDestLerp(size_t buffPos) const -> V2dInt;

  // TODO Use V2dInt
  auto GetSourceInfo(const V2dInt& tranPoint) const
      -> std::tuple<uint32_t, uint32_t, NeighborhoodCoeffArray>;

private:
  const uint32_t m_screenWidth;
  const uint32_t m_screenHeight;
  const uint32_t m_bufferSize;

  class FilterCoefficients;
  const std::unique_ptr<const FilterCoefficients> m_precalculatedCoeffs;

  const float m_ratioScreenToNormalizedCoord;
  const float m_ratioNormalizedToScreenCoord;
  const float m_minNormalizedCoordVal;
  const float m_maxNormalizedCoordVal;

  UTILS::Parallel* const m_parallel;

  // TODO Use V2dInt
  int32_t m_xBuffMidPoint{};
  int32_t m_yBuffMidPoint{};

  bool m_settingsChanged = false;

  GetZoomVectorFunc m_getZoomVector{};

  // TODO Use V2dInt
  auto NormalizedToScreenCoordFlt(float normalizedCoord) const -> float;
  auto NormalizedToTranCoord(float normalizedCoord) const -> int32_t;
  auto ScreenToNormalizedCoord(int32_t screenCoord) const -> float;
  void IncNormalizedCoord(float& normalizedCoord) const;

  std::vector<int32_t> m_tranXSrce{};
  std::vector<int32_t> m_tranYSrce{};
  std::vector<int32_t> m_tranXDest{};
  std::vector<int32_t> m_tranYDest{};
  std::vector<int32_t> m_tranXTemp{};
  std::vector<int32_t> m_tranYTemp{};
  const uint32_t m_maxTranX;
  const uint32_t m_maxTranY;
  const uint32_t m_tranBuffStripeHeight;
  uint32_t m_tranBuffYLineStart;
  enum class TranBufferState
  {
    RESTART_TRAN_BUFFER,
    RESET_TRAN_BUFFER,
    TRAN_BUFFER_READY,
  };
  TranBufferState m_tranBufferState;
  // modification by jeko : fixedpoint : tranDiffFactor = (16:16) (0 <= tranDiffFactor <= 2^16)
  int32_t m_tranDiffFactor; // in [0, BUFF_POINT_MASK]
  auto GetTranXBuffSrceDestLerp(size_t buffPos) const -> int32_t;
  auto GetTranYBuffSrceDestLerp(size_t buffPos) const -> int32_t;
  // TODO Use V2dInt
  auto GetTranPoint(float xNormalised, float yNormalised) const -> std::tuple<int32_t, int32_t>;

  std::vector<int32_t> m_firedec{};

  void InitTranBuffer();
  void UpdateTranBuffer();
  void RestartTranBuffer();
  void ResetTranBuffer();
  void DoNextTranBufferStripe(uint32_t tranBuffStripeHeight);
  void GenerateWaterFxHorizontalBuffer();
};

} // namespace GOOM
#endif
