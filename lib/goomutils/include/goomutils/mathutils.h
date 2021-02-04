#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_MATHUTILS_H_
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_MATHUTILS_H_

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


namespace stdnew
{
#if __cplusplus <= 201402L
template<typename _Fp>
constexpr _Fp __lerp(_Fp __a, _Fp __b, _Fp __t) noexcept
{
  if ((__a <= 0 && __b >= 0) || (__a >= 0 && __b <= 0))
  {
    return __t * __b + (1 - __t) * __a;
  }

  if (__t == 1)
  {
    return __b; // exact
  }

  // Exact at __t=0, monotonic except near __t=1,
  // bounded, determinate, and consistent:
  const _Fp __x = __a + __t * (__b - __a);
  return (__t > 1) == (__b > __a) ? (__b < __x ? __x : __b)
                                  : (__b > __x ? __x : __b); // monotonic near __t=1
}

constexpr float lerp(float __a, float __b, float __t) noexcept
{
  return __lerp(__a, __b, __t);
}
constexpr double lerp(double __a, double __b, double __t) noexcept
{
  return __lerp(__a, __b, __t);
}

constexpr auto clamp(const float x, const float lo, const float hi) -> float
{
  if (x < lo)
  {
    return lo;
  }
  if (x > hi)
  {
    return hi;
  }
  return x;
}
#else
constexpr float lerp(float __a, float __b, float __t) noexcept
{
  return std::lerp(__a, __b, __t);
}
constexpr double lerp(double __a, double __b, double __t) noexcept
{
  return std::lerp(__a, __b, __t);
}

constexpr auto clamp(const float x, const float lo, const float hi) -> float
{
  return std::clamp(x, lo, hi);
}
#endif
} // namespace stdnew


#if __cplusplus <= 201402L
namespace GOOM
{
namespace UTILS
{
#else
namespace GOOM::UTILS
{
#endif

#if __cplusplus <= 201402L
constexpr float m_pi = 3.14159265358979323846264;
#else
constexpr float m_pi = std::numbers::pi;
#endif
constexpr float m_two_pi = 2.0 * m_pi;
constexpr float m_half_pi = 0.5 * m_pi;

constexpr size_t NUM_SIN_COS_ANGLES = 256;
extern const std::array<float, NUM_SIN_COS_ANGLES> sin256;
extern const std::array<float, NUM_SIN_COS_ANGLES> cos256;

constexpr auto Sq(const float x) -> float
{
  return x * x;
}

constexpr auto SqDistance(const float x, const float y) -> float
{
  return Sq(x) + Sq(y);
}

class VertNum
{
public:
  explicit VertNum(const int xw) noexcept : m_xwidth(xw) {}
  [[nodiscard]] auto GetRowZeroVertNum(const int x) const -> int { return x; }
  auto operator()(const int x, const int z) const -> int { return z * m_xwidth + x; }
  //  int getPrevRowVertNum(const int vertNum) const { return vertNum - xwidth; }
  [[nodiscard]] auto GetXz(const int vertNum) const -> std::tuple<int, int>
  {
    const int z = vertNum / m_xwidth;
    const int x = vertNum % m_xwidth;
    return std::make_tuple(x, z);
  }

private:
  const int m_xwidth;
};

class RangeMapper
{
public:
  RangeMapper() noexcept = default;
  explicit RangeMapper(double x0, double x1) noexcept;
  auto operator()(double r0, double r1, double x) const -> double;
  [[nodiscard]] auto GetXMin() const -> double { return m_xmin; }
  [[nodiscard]] auto GetXMax() const -> double { return m_xmax; }

#if __cplusplus <= 201402L
  auto operator==(const RangeMapper&) const -> bool { return false; };
#else
  auto operator==(const RangeMapper&) const -> bool = default;
#endif

  template<class Archive>
  void serialize(Archive& ar);

private:
  double m_xmin = 0;
  double m_xmax = 0;
  double m_xwidth = 0;
};

class IDampingFunction
{
public:
  IDampingFunction() noexcept = default;
  virtual ~IDampingFunction() noexcept = default;
  IDampingFunction(const IDampingFunction&) noexcept = delete;
  IDampingFunction(IDampingFunction&&) noexcept = delete;
  auto operator=(const IDampingFunction&) -> IDampingFunction& = delete;
  auto operator=(IDampingFunction&&) -> IDampingFunction& = delete;

  virtual auto operator()(double x) -> double = 0;
};

class NoDampingFunction : public IDampingFunction
{
public:
  auto operator()([[maybe_unused]] const double x) -> double override { return 1.0; }
};

class LogDampingFunction : public IDampingFunction
{
public:
  explicit LogDampingFunction(double amplitude, double xmin, double xStart = 2.0) noexcept;
  auto operator()(double x) -> double override;

#if __cplusplus <= 201402L
  auto operator==(const LogDampingFunction&) const -> bool { return false; };
#else
  auto operator==(const LogDampingFunction&) const -> bool = default;
#endif

private:
  const double m_amplitude;
  const double m_xmin;
  const double m_xStart;
};

// Asymptotes to 'amplitude' in negative x direction,
//   and the rate depends on the input parameters.
class ExpDampingFunction : public IDampingFunction
{
public:
  ExpDampingFunction() noexcept = default;
  ExpDampingFunction(
      double amplitude, double xToStartRise, double yAtStartToRise, double xmax, double yAtXMax);
  auto operator()(double x) -> double override;
  [[nodiscard]] auto KVal() const -> double { return m_k; }
  [[nodiscard]] auto BVal() const -> double { return m_b; }

#if __cplusplus <= 201402L
  auto operator==(const ExpDampingFunction&) const -> bool { return false; };
#else
  auto operator==(const ExpDampingFunction&) const -> bool = default;
#endif

  template<class Archive>
  void serialize(Archive& ar);

private:
  double m_amplitude = 1;
  double m_k = 1;
  double m_b = 1;
};

class FlatDampingFunction : public IDampingFunction
{
public:
  FlatDampingFunction() noexcept = default;
  explicit FlatDampingFunction(double y) noexcept;
  auto operator()(double x) -> double override;

#if __cplusplus <= 201402L
  bool operator==(const FlatDampingFunction&) const { return false; };
#else
  bool operator==(const FlatDampingFunction&) const = default;
#endif

  template<class Archive>
  void serialize(Archive& ar);

private:
  double m_y = 0;
};

class LinearDampingFunction : public IDampingFunction
{
public:
  LinearDampingFunction() noexcept = default;
  explicit LinearDampingFunction(double x0, double y0, double x1, double y1) noexcept;
  auto operator()(double x) -> double override;

#if __cplusplus <= 201402L
  auto operator==(const LinearDampingFunction&) const -> bool { return false; };
#else
  auto operator==(const LinearDampingFunction&) const -> bool = default;
#endif

  template<class Archive>
  void serialize(Archive& ar);

private:
  double m_m = 1;
  double m_x1 = 0;
  double m_y1 = 1;
};

class PiecewiseDampingFunction : public IDampingFunction
{
public:
  PiecewiseDampingFunction() noexcept = default;
  explicit PiecewiseDampingFunction(
      std::vector<std::tuple<double, double, std::unique_ptr<IDampingFunction>>>& pieces) noexcept;
  auto operator()(double x) -> double override;

#if __cplusplus <= 201402L
  bool operator==(const PiecewiseDampingFunction&) const { return false; }
#else
  bool operator==(const PiecewiseDampingFunction&) const = default;
#endif

  template<class Archive>
  void serialize(Archive& ar);

private:
  std::vector<std::tuple<double, double, std::unique_ptr<IDampingFunction>>> m_pieces{};
};

class ISequenceFunction
{
public:
  ISequenceFunction() noexcept = default;
  virtual ~ISequenceFunction() noexcept = default;
  ISequenceFunction(const ISequenceFunction&) noexcept = default;
  ISequenceFunction(ISequenceFunction&&) noexcept = delete;
  auto operator=(const ISequenceFunction&) noexcept -> ISequenceFunction& = default;
  auto operator=(ISequenceFunction&&) noexcept -> ISequenceFunction& = delete;

  virtual auto GetNext() -> float = 0;
};

class SineWaveMultiplier : public ISequenceFunction
{
public:
  SineWaveMultiplier(float frequency = 1.0,
                     float lower = -1.0,
                     float upper = 1.0,
                     float x0 = 0.0) noexcept;

  auto GetNext() -> float override;

  void SetX0(const float x0) { m_x = x0; }
  [[nodiscard]] auto GetFrequency() const -> float { return m_frequency; }
  void SetFrequency(const float val) { m_frequency = val; }

  [[nodiscard]] auto GetLower() const -> float { return m_lower; }
  [[nodiscard]] auto GetUpper() const -> float { return m_upper; }
  void SetRange(const float lwr, const float upr)
  {
    m_lower = lwr;
    m_upper = upr;
  }

  void SetPiStepFrac(const float val) { m_piStepFrac = val; }

  auto operator==(const SineWaveMultiplier&) const -> bool;

  template<class Archive>
  void serialize(Archive& ar);

private:
  RangeMapper m_rangeMapper;
  float m_frequency;
  float m_lower;
  float m_upper;
  float m_piStepFrac;
  float m_x;
};

class IIncreasingFunction
{
public:
  IIncreasingFunction() noexcept = delete;
  virtual ~IIncreasingFunction() noexcept = default;
  explicit IIncreasingFunction(double x0, double x1) noexcept;
  IIncreasingFunction(const IIncreasingFunction&) noexcept = delete;
  IIncreasingFunction(IIncreasingFunction&&) noexcept = delete;
  auto operator=(const IIncreasingFunction&) noexcept -> IIncreasingFunction& = delete;
  auto operator=(IIncreasingFunction&&) noexcept -> IIncreasingFunction& = delete;

  virtual auto operator()(double x) -> double = 0;

protected:
  const double m_xmin;
  const double m_xmax;
};

// Asymptotes to 1, depending on 'k' value.
class ExpIncreasingFunction : public IIncreasingFunction
{
public:
  explicit ExpIncreasingFunction(double xmin, double xmax, double k = 0.1) noexcept;
  auto operator()(double x) -> double override;

private:
  const double m_k;
};

class LogIncreasingFunction : public IIncreasingFunction
{
public:
  explicit LogIncreasingFunction(double amplitude, double xmin, double xStart = 2.0) noexcept;
  auto operator()(double x) -> double override;

private:
  const double m_amplitude;
  const double m_xmin;
  const double m_xStart;
};

inline IIncreasingFunction::IIncreasingFunction(const double x0, const double x1) noexcept
  : m_xmin(x0), m_xmax(x1)
{
}

inline RangeMapper::RangeMapper(const double x0, const double x1) noexcept
  : m_xmin(x0), m_xmax(x1), m_xwidth(m_xmax - m_xmin)
{
}

template<class Archive>
void RangeMapper::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_xmin), CEREAL_NVP(m_xmax), CEREAL_NVP(m_xwidth));
}

inline auto RangeMapper::operator()(const double r0, const double r1, const double x) const
    -> double
{
  return stdnew::lerp(r0, r1, (x - m_xmin) / m_xwidth);
}

template<class Archive>
void SineWaveMultiplier::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_rangeMapper), CEREAL_NVP(m_frequency), CEREAL_NVP(m_lower), CEREAL_NVP(m_upper),
     CEREAL_NVP(m_piStepFrac), CEREAL_NVP(m_x));
}

template<class Archive>
void FlatDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_y));
}

template<class Archive>
void ExpDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_amplitude), CEREAL_NVP(m_k), CEREAL_NVP(m_b));
}

template<class Archive>
void LinearDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_m), CEREAL_NVP(m_x1), CEREAL_NVP(m_y1));
}

template<class Archive>
void PiecewiseDampingFunction::serialize(Archive& ar)
{
  ar(CEREAL_NVP(m_pieces));
}

#if __cplusplus <= 201402L
} // namespace UTILS
} // namespace GOOM
#else
} // namespace GOOM::UTILS
#endif

#endif
