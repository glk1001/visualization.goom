#ifndef VISUALIZATION_GOOM_TENTACLES_H
#define VISUALIZATION_GOOM_TENTACLES_H

#include "goom_graphic.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>
#include <vivid/vivid.h>

namespace goom
{

class ITentacleColorizer
{
public:
  ITentacleColorizer() noexcept = default;
  virtual ~ITentacleColorizer() noexcept = default;
  ITentacleColorizer(const ITentacleColorizer&) noexcept = delete;
  ITentacleColorizer(const ITentacleColorizer&&) noexcept = delete;
  auto operator=(const ITentacleColorizer&) noexcept -> ITentacleColorizer& = delete;
  auto operator=(const ITentacleColorizer&&) noexcept -> ITentacleColorizer& = delete;

  [[nodiscard]] virtual auto GetColorMapGroup() const -> utils::ColorMapGroup = 0;
  virtual void SetColorMapGroup(utils::ColorMapGroup) = 0;
  virtual void ChangeColorMap() = 0;
  [[nodiscard]] virtual auto GetColor(size_t nodeNum) const -> Pixel = 0;
};

class Tentacle2D
{
private:
  using XAndYVectors = std::tuple<std::vector<double>&, std::vector<double>&>;

public:
  static constexpr size_t MIN_NUM_NODES = 10;

  Tentacle2D() noexcept;
  ~Tentacle2D() noexcept = default;
  Tentacle2D(size_t id,
             size_t numNodes,
             double xmin,
             double xmax,
             double ymin,
             double ymax,
             double basePrevYWeight,
             double baseCurrentYWeight) noexcept;
  Tentacle2D(const Tentacle2D&) = delete;
  Tentacle2D(const Tentacle2D&&) = delete;
  auto operator=(const Tentacle2D&) -> Tentacle2D& = delete;
  auto operator=(const Tentacle2D&&) -> Tentacle2D& = delete;

  [[nodiscard]] auto GetID() const -> size_t;

  void StartIterating();
  void FinishIterating();

  [[nodiscard]] auto GetIterNum() const -> size_t;
  void Iterate();
  void IterateNTimes(size_t n);

  [[nodiscard]] auto GetXAndYVectors() const -> const XAndYVectors&;
  [[nodiscard]] auto GetDampedXAndYVectors() const -> const XAndYVectors&;

  [[nodiscard]] auto GetLength() const -> double;

  [[nodiscard]] auto GetXMin() const -> double;
  [[nodiscard]] auto GetXMax() const -> double;
  void SetXDimensions(double x0, double y0);

  [[nodiscard]] auto GetYMin() const -> double;
  [[nodiscard]] auto GetYMax() const -> double;

  [[nodiscard]] auto GetNumNodes() const -> size_t;

  [[nodiscard]] auto GetPrevYWeight() const -> double;
  [[nodiscard]] auto GetCurrentYWeight() const -> double;

  [[nodiscard]] auto GetIterZeroYVal() const -> double;
  void SetIterZeroYVal(double val);

  [[nodiscard]] auto GetIterZeroLerpFactor() const -> double;
  void SetIterZeroLerpFactor(double val);

  [[nodiscard]] auto GetDoDamping() const -> bool;
  void SetDoDamping(bool val);

  auto operator==(const Tentacle2D&) const -> bool;

  template<class Archive>
  void serialize(Archive&);

private:
  size_t m_id = 0;
  size_t m_numNodes = 0;
  double m_xmin = 0.0;
  double m_xmax = 0.0;
  double m_ymin = 0.0;
  double m_ymax = 0.0;
  double m_basePrevYWeight = 0.770;
  double m_baseCurrentYWeight = 0.230;
  double m_iterZeroYVal = 0.9;
  double m_iterZeroLerpFactor = 0.8;
  size_t m_iterNum = 0;
  bool m_startedIterating = false;
  std::vector<double> m_xvec{};
  std::vector<double> m_yvec{};
  XAndYVectors m_vecs{std::make_tuple(std::ref(m_xvec), std::ref(m_yvec))};
  std::vector<double> m_dampedYVec{};
  std::vector<double> m_dampingCache{};
  XAndYVectors m_dampedVecs{std::make_tuple(std::ref(m_xvec), std::ref(m_dampedYVec))};
  std::unique_ptr<utils::DampingFunction> m_dampingFunc{};
  bool m_doDamping = true;

  auto GetFirstY() -> float;
  auto GetNextY(size_t nodeNum) -> float;
  auto GetDamping(double x) -> double;
  [[nodiscard]] auto GetDampedVal(size_t nodeNum, float val) const -> float;
  void UpdateDampedVals(const std::vector<double>& yvals);
  [[nodiscard]] auto Damp(size_t nodeNum) const -> double;
  void ValidateSettings() const;
  void ValidateXDimensions() const;
  void ValidateYDimensions() const;
  void ValidateNumNodes() const;
  void ValidatePrevYWeight() const;
  void ValidateCurrentYWeight() const;

  using DampingFuncPtr = std::unique_ptr<utils::DampingFunction>;
  static auto CreateDampingFunc(double prevYWeight, double xmin, double xmax) -> DampingFuncPtr;
  static auto CreateExpDampingFunc(double xmin, double xmax) -> DampingFuncPtr;
  static auto CreateLinearDampingFunc(double xmin, double xmax) -> DampingFuncPtr;
};

struct V3d
{
  float x = 0.0;
  float y = 0.0;
  float z = 0.0;
  bool ignore = false;

  auto operator==(const V3d&) const -> bool = default;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(ignore));
  }
};

class Tentacle3D
{
public:
  Tentacle3D() noexcept = default;
  ~Tentacle3D() noexcept = default;
  Tentacle3D(std::unique_ptr<Tentacle2D>,
             const Pixel& headColor,
             const Pixel& headColorLow,
             const V3d& head,
             size_t numHeadNodes) noexcept;
  Tentacle3D(std::unique_ptr<Tentacle2D>,
             std::shared_ptr<const ITentacleColorizer>,
             const Pixel& headColor,
             const Pixel& headColorLow,
             const V3d& head,
             size_t numHeadNodes) noexcept;
  Tentacle3D(Tentacle3D&&) noexcept;
  Tentacle3D(const Tentacle3D&) = delete;
  auto operator=(const Tentacle3D&) -> Tentacle3D& = delete;
  auto operator=(const Tentacle3D&&) -> Tentacle3D& = delete;

  auto Get2DTentacle() -> Tentacle2D& { return *m_tentacle; }
  [[nodiscard]] auto Get2DTentacle() const -> const Tentacle2D& { return *m_tentacle; }

  [[nodiscard]] auto GetId() const -> size_t { return m_tentacle->GetID(); }

  [[nodiscard]] auto GetColor(size_t nodeNum) const -> Pixel;
  [[nodiscard]] auto GetMixedColors(size_t nodeNum, const Pixel& color, const Pixel& colorLow) const
      -> std::tuple<Pixel, Pixel>;
  [[nodiscard]] auto GetMixedColors(size_t nodeNum,
                                    const Pixel& color,
                                    const Pixel& colorLow,
                                    float brightness) const -> std::tuple<Pixel, Pixel>;

  [[nodiscard]] auto GetReverseColorMix() const -> bool { return m_reverseColorMix; }
  void SetReverseColorMix(const bool val) { m_reverseColorMix = val; }

  [[nodiscard]] auto GetAllowOverexposed() const -> bool { return m_allowOverexposed; }
  void SetAllowOverexposed(const bool val) { m_allowOverexposed = val; }

  [[nodiscard]] auto GetHead() const -> const V3d& { return m_head; }
  void SetHead(const V3d& val) { m_head = val; }

  [[nodiscard]] auto GetNumHeadNodes() const -> size_t { return m_numHeadNodes; }
  void SetNumHeadNodes(const size_t val) { m_numHeadNodes = val; }

  [[nodiscard]] auto GetVertices() const -> std::vector<V3d>;

  auto operator==(const Tentacle3D&) const -> bool;

  template<class Archive>
  void serialize(Archive&);

private:
  std::unique_ptr<Tentacle2D> m_tentacle{};
  std::shared_ptr<const ITentacleColorizer> m_colorizer{};
  Pixel m_headColor{};
  Pixel m_headColorLow{};
  V3d m_head{};
  size_t m_numHeadNodes{};
  bool m_reverseColorMix = false;
  bool m_allowOverexposed = true;
};

class Tentacles3D
{
private:
  class Iter
  {
  public:
    Iter(Tentacles3D* const tents, const size_t p) : m_pos(p), m_tentacles(tents) {}
    auto operator!=(const Iter& other) const -> bool { return m_pos != other.m_pos; }
    auto operator*() const -> Tentacle3D&;
    auto operator++() -> const Iter&
    {
      ++m_pos;
      return *this;
    }

  private:
    size_t m_pos;
    Tentacles3D* m_tentacles;
  };

public:
  Tentacles3D() noexcept = default;

  void AddTentacle(Tentacle3D&& t);

  Iter begin() { return Iter(this, 0); }
  Iter end() { return Iter(this, m_tentacles.size()); }

  auto operator[](const size_t i) const -> const Tentacle3D& { return m_tentacles.at(i); }
  auto operator[](const size_t i) -> Tentacle3D& { return m_tentacles.at(i); }

  void SetAllowOverexposed(bool val);

  auto operator==(const Tentacles3D&) const -> bool;

  template<class Archive>
  void serialize(Archive&);

private:
  std::vector<Tentacle3D> m_tentacles{};
};

inline auto Tentacle2D::GetID() const -> size_t
{
  return m_id;
}

inline auto Tentacle2D::GetIterZeroYVal() const -> double
{
  return m_iterZeroYVal;
}
inline auto Tentacle2D::GetIterZeroLerpFactor() const -> double
{
  return m_iterZeroLerpFactor;
}

inline void Tentacle2D::SetIterZeroYVal(const double val)
{
  m_iterZeroYVal = val;
}

inline void Tentacle2D::SetIterZeroLerpFactor(const double val)
{
  m_iterZeroLerpFactor = val;
}

inline auto Tentacle2D::GetNumNodes() const -> size_t
{
  return m_numNodes;
}

inline auto Tentacle2D::GetPrevYWeight() const -> double
{
  return m_basePrevYWeight;
}

inline auto Tentacle2D::GetCurrentYWeight() const -> double
{
  return m_baseCurrentYWeight;
}

inline auto Tentacle2D::GetIterNum() const -> size_t
{
  return m_iterNum;
}

inline auto Tentacle2D::GetLength() const -> double
{
  return m_xmax - m_xmin;
}

inline auto Tentacle2D::GetXMin() const -> double
{
  return m_xmin;
}
inline auto Tentacle2D::GetXMax() const -> double
{
  return m_xmax;
}

inline auto Tentacle2D::GetYMin() const -> double
{
  return m_ymin;
}
inline auto Tentacle2D::GetYMax() const -> double
{
  return m_ymax;
}

inline auto Tentacle2D::GetDoDamping() const -> bool
{
  return m_doDamping;
}

inline void Tentacle2D::SetDoDamping(const bool val)
{
  m_doDamping = val;
}

inline auto Tentacle2D::GetDamping(const double x) -> double
{
  return (*m_dampingFunc)(x);
}

inline void Tentacle2D::FinishIterating()
{
  m_startedIterating = false;
}

template<class Archive>
void Tentacle2D::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_id), CEREAL_NVP(m_numNodes), CEREAL_NVP(m_xmin), CEREAL_NVP(m_xmax),
     CEREAL_NVP(m_ymin), CEREAL_NVP(m_ymax), CEREAL_NVP(m_basePrevYWeight),
     CEREAL_NVP(m_baseCurrentYWeight), CEREAL_NVP(m_iterZeroYVal), CEREAL_NVP(m_iterZeroLerpFactor),
     CEREAL_NVP(m_iterNum), CEREAL_NVP(m_startedIterating), CEREAL_NVP(m_xvec), CEREAL_NVP(m_yvec),
     CEREAL_NVP(m_dampedYVec), CEREAL_NVP(m_dampingCache), CEREAL_NVP(m_dampingFunc),
     CEREAL_NVP(m_doDamping));
}

inline auto Tentacles3D::Iter::operator*() const -> Tentacle3D&
{
  return (*m_tentacles)[m_pos];
}

template<class Archive>
void Tentacle3D::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_tentacle), CEREAL_NVP(m_colorizer), CEREAL_NVP(m_headColor),
     CEREAL_NVP(m_headColorLow), CEREAL_NVP(m_head), CEREAL_NVP(m_numHeadNodes),
     CEREAL_NVP(m_reverseColorMix), CEREAL_NVP(m_allowOverexposed));
}

template<class Archive>
void Tentacles3D::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_tentacles));
}

} // namespace goom
#endif
