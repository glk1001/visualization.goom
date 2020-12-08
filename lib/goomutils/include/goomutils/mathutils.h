#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_MATHUTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_MATHUTILS_H_

#include "goomrand.h"

#include <array>
#include <cereal/archives/json.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <tuple>
#include <vector>

namespace goom::utils
{

constexpr float m_pi = std::numbers::pi;
constexpr float m_two_pi = 2.0 * std::numbers::pi;
constexpr float m_half_pi = 0.5 * std::numbers::pi;

constexpr size_t numSinCosAngles = 256;
extern const std::array<float, numSinCosAngles> sin256;
extern const std::array<float, numSinCosAngles> cos256;

constexpr float sq(const float x)
{
  return x * x;
}

constexpr float sq_distance(const float x, const float y)
{
  return sq(x) + sq(y);
}

class VertNum
{
public:
  explicit VertNum(const int xw) noexcept : xwidth(xw) {}
  [[nodiscard]] int getRowZeroVertNum(const int x) const { return x; }
  int operator()(const int x, const int z) const { return z * xwidth + x; }
  //  int getPrevRowVertNum(const int vertNum) const { return vertNum - xwidth; }
  [[nodiscard]] std::tuple<int, int> getXZ(const int vertNum) const
  {
    const int z = vertNum / xwidth;
    const int x = vertNum % xwidth;
    return std::make_tuple(x, z);
  }

private:
  const int xwidth;
};

class RangeMapper
{
public:
  RangeMapper() noexcept = default;
  explicit RangeMapper(double x0, double x1) noexcept;
  double operator()(double r0, double r1, double x) const;
  [[nodiscard]] double getXMin() const { return xmin; }
  [[nodiscard]] double getXMax() const { return xmax; }

  bool operator==(const RangeMapper&) const = default;

  template<class Archive>
  void serialize(Archive&);

private:
  double xmin = 0;
  double xmax = 0;
  double xwidth = 0;
};

class DampingFunction
{
public:
  virtual ~DampingFunction() noexcept = default;
  virtual double operator()(double x) = 0;
};

class NoDampingFunction : public DampingFunction
{
public:
  double operator()([[maybe_unused]] const double x) override { return 1.0; }
};

class LogDampingFunction : public DampingFunction
{
public:
  explicit LogDampingFunction(double amplitude, double xmin, double xStart = 2.0) noexcept;
  double operator()(double x) override;

  bool operator==(const LogDampingFunction&) const = default;

private:
  const double amplitude;
  const double xmin;
  const double xStart;
};

// Asymptotes to 'amplitude' in negative x direction,
//   and the rate depends on the input parameters.
class ExpDampingFunction : public DampingFunction
{
public:
  ExpDampingFunction() noexcept = default;
  explicit ExpDampingFunction(
      double amplitude, double xToStartRise, double yAtStartToRise, double xmax, double yAtXMax);
  double operator()(double x) override;
  [[nodiscard]] double kVal() const { return k; }
  [[nodiscard]] double bVal() const { return b; }

  bool operator==(const ExpDampingFunction&) const = default;

  template<class Archive>
  void serialize(Archive&);

private:
  double amplitude = 1;
  double k = 1;
  double b = 1;
};

class FlatDampingFunction : public DampingFunction
{
public:
  FlatDampingFunction() noexcept = default;
  explicit FlatDampingFunction(double y) noexcept;
  double operator()(double x) override;

  bool operator==(const FlatDampingFunction&) const = default;

  template<class Archive>
  void serialize(Archive&);

private:
  double y = 0;
};

class LinearDampingFunction : public DampingFunction
{
public:
  LinearDampingFunction() noexcept = default;
  explicit LinearDampingFunction(double x0, double y0, double x1, double y1) noexcept;
  double operator()(double x) override;

  bool operator==(const LinearDampingFunction&) const = default;

  template<class Archive>
  void serialize(Archive&);

private:
  double m = 1;
  double x1 = 0;
  double y1 = 1;
};

class PiecewiseDampingFunction : public DampingFunction
{
public:
  PiecewiseDampingFunction() noexcept = default;
  explicit PiecewiseDampingFunction(
      std::vector<std::tuple<double, double, std::unique_ptr<DampingFunction>>>& pieces) noexcept;
  double operator()(double x) override;

  bool operator==(const PiecewiseDampingFunction&) const = default;

  template<class Archive>
  void serialize(Archive&);

private:
  std::vector<std::tuple<double, double, std::unique_ptr<DampingFunction>>> pieces{};
};

class SequenceFunction
{
public:
  virtual ~SequenceFunction() noexcept = default;
  virtual float getNext() = 0;
};

class SineWaveMultiplier : public SequenceFunction
{
public:
  SineWaveMultiplier(float frequency = 1.0,
                     float lower = -1.0,
                     float upper = 1.0,
                     float x0 = 0.0) noexcept;

  float getNext() override;

  void setX0(const float x0) { x = x0; }
  [[nodiscard]] float getFrequency() const { return frequency; }
  void setFrequency(const float val) { frequency = val; }

  [[nodiscard]] float getLower() const { return lower; }
  [[nodiscard]] float getUpper() const { return upper; }
  void setRange(const float lwr, const float upr)
  {
    lower = lwr;
    upper = upr;
  }

  void setPiStepFrac(const float val) { piStepFrac = val; }

  bool operator==(const SineWaveMultiplier&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  RangeMapper rangeMapper;
  float frequency;
  float lower;
  float upper;
  float piStepFrac;
  float x;
};

class IncreasingFunction
{
public:
  explicit IncreasingFunction(double xmin, double xmax) noexcept;
  virtual ~IncreasingFunction() noexcept = default;
  virtual double operator()(double x) = 0;

protected:
  const double xmin;
  const double xmax;
};

// Asymptotes to 1, depending on 'k' value.
class ExpIncreasingFunction : public IncreasingFunction
{
public:
  explicit ExpIncreasingFunction(double xmin, double xmax, double k = 0.1) noexcept;
  double operator()(double x) override;

private:
  const double k;
};

class LogIncreasingFunction : public IncreasingFunction
{
public:
  explicit LogIncreasingFunction(double amplitude, double xmin, double xStart = 2.0) noexcept;
  double operator()(double x) override;

private:
  const double amplitude;
  const double xmin;
  const double xStart;
};

inline IncreasingFunction::IncreasingFunction(const double x0, const double x1) noexcept
  : xmin(x0), xmax(x1)
{
}

inline RangeMapper::RangeMapper(const double x0, const double x1) noexcept
  : xmin(x0), xmax(x1), xwidth(xmax - xmin)
{
}

template<class Archive>
void RangeMapper::serialize(Archive& ar)
{
  ar(CEREAL_NVP(xmin), CEREAL_NVP(xmax), CEREAL_NVP(xwidth));
}

inline double RangeMapper::operator()(const double r0, const double r1, const double x) const
{
  return std::lerp(r0, r1, (x - xmin) / xwidth);
}

template<class Archive>
void SineWaveMultiplier::serialize(Archive& ar)
{
  ar(CEREAL_NVP(rangeMapper), CEREAL_NVP(frequency), CEREAL_NVP(lower), CEREAL_NVP(upper),
     CEREAL_NVP(piStepFrac), CEREAL_NVP(x));
}

template<class Archive>
void FlatDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(y));
}

template<class Archive>
void ExpDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(amplitude), CEREAL_NVP(k), CEREAL_NVP(b));
}

template<class Archive>
void LinearDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m), CEREAL_NVP(x1), CEREAL_NVP(y1));
}

template<class Archive>
void PiecewiseDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(pieces));
}

} // namespace goom::utils

#endif
