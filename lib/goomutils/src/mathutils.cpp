#include "goomutils/mathutils.h"

#include <array>
#include <cereal/types/memory.hpp>
#include <cmath>
#include <format>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

// NOTE: Cereal is not happy with these calls inside the 'goom' namespace.
//   But they work OK here.
CEREAL_REGISTER_TYPE(GOOM::UTILS::SineWaveMultiplier)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::UTILS::ISequenceFunction,
                                     GOOM::UTILS::SineWaveMultiplier)

CEREAL_REGISTER_TYPE(GOOM::UTILS::FlatDampingFunction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::UTILS::IDampingFunction,
                                     GOOM::UTILS::FlatDampingFunction)

CEREAL_REGISTER_TYPE(GOOM::UTILS::ExpDampingFunction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::UTILS::IDampingFunction, GOOM::UTILS::ExpDampingFunction)

CEREAL_REGISTER_TYPE(GOOM::UTILS::LinearDampingFunction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::UTILS::IDampingFunction,
                                     GOOM::UTILS::LinearDampingFunction)

CEREAL_REGISTER_TYPE(GOOM::UTILS::PiecewiseDampingFunction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GOOM::UTILS::IDampingFunction,
                                     GOOM::UTILS::PiecewiseDampingFunction)

#if __cplusplus <= 201402L
namespace GOOM
{
namespace UTILS
{
#else
namespace GOOM::UTILS
{
#endif

ExpIncreasingFunction::ExpIncreasingFunction(const double x0,
                                             const double x1,
                                             const double _k) noexcept
  : IIncreasingFunction(x1, x0), m_k(_k)
{
}

auto ExpIncreasingFunction::operator()(const double x) -> double
{
  return 1.0 - std::exp(-m_k * std::fabs(x));
}

LogIncreasingFunction::LogIncreasingFunction(const double amp,
                                             const double xm,
                                             const double xStrt) noexcept
  : IIncreasingFunction(xm, 10000000000), m_amplitude(amp), m_xmin(xm), m_xStart(xStrt)
{
}

double LogIncreasingFunction::operator()(const double x)
{
  return m_amplitude / std::log(x - m_xmin + m_xStart);
}


LogDampingFunction::LogDampingFunction(const double amp,
                                       const double xm,
                                       const double xStrt) noexcept
  : m_amplitude(amp), m_xmin(xm), m_xStart(xStrt)
{
}

double LogDampingFunction::operator()(const double x)
{
  return m_amplitude * std::log(x - m_xmin + m_xStart);
}

ExpDampingFunction::ExpDampingFunction(const double amp,
                                       const double xToStartRise,
                                       const double yAtStartToRise,
                                       const double xmax,
                                       const double yAtXMax)
  : m_amplitude(amp)
{
  constexpr double MIN_AMP = 0.00001;
  if (std::fabs(m_amplitude) < MIN_AMP)
  {
    throw std::runtime_error(
        std20::format("abs(amplitude) should be >= {}, not {}.", MIN_AMP, m_amplitude));
  }
  if (yAtStartToRise <= m_amplitude)
  {
    throw std::runtime_error(std20::format("yAtStartToRise should be > {} = amplitude, not {}.",
                                           m_amplitude, yAtStartToRise));
  }
  if (yAtXMax <= m_amplitude)
  {
    throw std::runtime_error(
        std20::format("yAtXMax should be > {} = amplitude, not {}.", m_amplitude, yAtXMax));
  }
  const double y0 = yAtStartToRise / m_amplitude - 1.0;
  const double y1 = yAtXMax / m_amplitude - 1.0;
  const double log_y0 = std::log(y0);
  const double log_y1 = std::log(y1);
  m_b = (xToStartRise * log_y1 - xmax * log_y0) / (log_y1 - log_y0);
  m_k = log_y1 / (xmax - m_b);
}

auto ExpDampingFunction::operator()(const double x) -> double
{
  return m_amplitude * (1.0 + std::exp(m_k * (x - m_b)));
}

FlatDampingFunction::FlatDampingFunction(const double y_) noexcept : m_y{y_}
{
}

auto FlatDampingFunction::operator()([[maybe_unused]] const double x) -> double
{
  return m_y;
}

LinearDampingFunction::LinearDampingFunction(const double x0_,
                                             const double y0_,
                                             const double x1_,
                                             const double y1_) noexcept
  : m_m{(y1_ - y0_) / (x1_ - x0_)}, m_x1{x1_}, m_y1{y1_}
{
}

auto LinearDampingFunction::operator()(const double x) -> double
{
  return m_m * (x - m_x1) + m_y1;
}

PiecewiseDampingFunction::PiecewiseDampingFunction(
    std::vector<std::tuple<double, double, std::unique_ptr<IDampingFunction>>>& p) noexcept
  : m_pieces{std::move(p)}
{
}

auto PiecewiseDampingFunction::operator()(const double x) -> double
{
#if __cplusplus <= 201402L
  for (const auto& pieces : m_pieces)
  {
    const auto& x0 = std::get<0>(pieces);
    const auto& x1 = std::get<1>(pieces);
    const auto& func = std::get<2>(pieces);
#else
  for (const auto& [x0, x1, func] : m_pieces)
  {
#endif
    if ((x0 <= x) && (x < x1))
    {
      return (*func)(x);
    }
  }
  return 0.0;
}

SineWaveMultiplier::SineWaveMultiplier(const float freq,
                                       const float lwr,
                                       const float upr,
                                       const float x0) noexcept
  : m_rangeMapper{-1, +1},
    m_frequency{freq},
    m_lower{lwr},
    m_upper{upr},
    m_piStepFrac{1.0 / 16.0},
    m_x{x0}
{
}

auto SineWaveMultiplier::operator==(const SineWaveMultiplier& s) const -> bool
{
  return true;
  return m_rangeMapper == s.m_rangeMapper && m_frequency == s.m_frequency && m_lower == s.m_lower &&
         m_upper == s.m_upper && m_piStepFrac == s.m_piStepFrac && m_x == s.m_x;
}

auto SineWaveMultiplier::GetNext() -> float
{
  const float val = m_rangeMapper(m_lower, m_upper, std::sin(m_frequency * m_x));
  //logInfo("lower = {}, upper = {}, sin({}) = {}.", lower, upper, x, val);
  m_x += m_piStepFrac * m_pi;
  return val;
}

// clang-format off
const std::array<float, NUM_SIN_COS_ANGLES> sin256 = {
    +0,         +0.0245412,  0.0490677,    0.0735646,  0.0980171,  0.122411,   0.14673,
    +0.170962,  +0.19509,    0.219101,     0.24298,    0.266713,   0.290285,   0.313682,
    +0.33689,   +0.359895,   0.382683,     0.405241,   0.427555,   0.449611,   0.471397,
    +0.492898,  +0.514103,   0.534998,     0.55557,    0.575808,   0.595699,   0.615232,
    +0.634393,  +0.653173,   0.671559,     0.689541,   0.707107,   0.724247,   0.740951,
    +0.757209,  +0.77301,    0.788346,     0.803208,   0.817585,   0.83147,    0.844854,
    +0.857729,  +0.870087,   0.881921,     0.893224,   0.903989,   0.91421,    0.92388,
    +0.932993,  +0.941544,   0.949528,     0.95694,    0.963776,   0.970031,   0.975702,
    +0.980785,  +0.985278,   0.989177,     0.99248,    0.995185,   0.99729,    0.998795,
    +0.999699,  +1,          0.999699,     0.998795,   0.99729,    0.995185,   0.99248,
    +0.989177,  +0.985278,   0.980785,     0.975702,   0.970031,   0.963776,   0.95694,
    +0.949528,  +0.941544,   0.932993,     0.92388,    0.91421,    0.903989,   0.893224,
    +0.881921,  +0.870087,   0.857729,     0.844854,   0.83147,    0.817585,   0.803208,
    +0.788346,  +0.77301,    0.757209,     0.740951,   0.724247,   0.707107,   0.689541,
    +0.671559,  +0.653173,   0.634393,     0.615232,   0.595699,   0.575808,   0.55557,
    +0.534998,  +0.514103,   0.492898,     0.471397,   0.449611,   0.427555,   0.405241,
    +0.382683,  +0.359895,   0.33689,      0.313682,   0.290285,   0.266713,   0.24298,
    +0.219101,  +0.19509,    0.170962,     0.14673,    0.122411,   0.0980171,  0.0735646,
    +0.0490677, +0.0245412,  1.22465e-16, -0.0245412, -0.0490677, -0.0735646, -0.0980171,
    -0.122411,  -0.14673,   -0.170962,    -0.19509,   -0.219101,  -0.24298,   -0.266713,
    -0.290285,  -0.313682,  -0.33689,     -0.359895,  -0.382683,  -0.405241,  -0.427555,
    -0.449611,  -0.471397,  -0.492898,    -0.514103,  -0.534998,  -0.55557,   -0.575808,
    -0.595699,  -0.615232,  -0.634393,    -0.653173,  -0.671559,  -0.689541,  -0.707107,
    -0.724247,  -0.740951,  -0.757209,    -0.77301,   -0.788346,  -0.803208,  -0.817585,
    -0.83147,   -0.844854,  -0.857729,    -0.870087,  -0.881921,  -0.893224,  -0.903989,
    -0.91421,   -0.92388,   -0.932993,    -0.941544,  -0.949528,  -0.95694,   -0.963776,
    -0.970031,  -0.975702,  -0.980785,    -0.985278,  -0.989177,  -0.99248,   -0.995185,
    -0.99729,   -0.998795,  -0.999699,    -1,         -0.999699,  -0.998795,  -0.99729,
    -0.995185,  -0.99248,   -0.989177,    -0.985278,  -0.980785,  -0.975702,  -0.970031,
    -0.963776,  -0.95694,   -0.949528,    -0.941544,  -0.932993,  -0.92388,   -0.91421,
    -0.903989,  -0.893224,  -0.881921,    -0.870087,  -0.857729,  -0.844854,  -0.83147,
    -0.817585,  -0.803208,  -0.788346,    -0.77301,   -0.757209,  -0.740951,  -0.724247,
    -0.707107,  -0.689541,  -0.671559,    -0.653173,  -0.634393,  -0.615232,  -0.595699,
    -0.575808,  -0.55557,   -0.534998,    -0.514103,  -0.492898,  -0.471397,  -0.449611,
    -0.427555,  -0.405241,  -0.382683,    -0.359895,  -0.33689,   -0.313682,  -0.290285,
    -0.266713,  -0.24298,   -0.219101,    -0.19509,   -0.170962,  -0.14673,   -0.122411,
    -0.0980171, -0.0735646, -0.0490677,   -0.0245412
};
// clang-format on

// clang-format off
const std::array<float, NUM_SIN_COS_ANGLES> cos256 = {
     0,          0.999699,     0.998795,   0.99729,      0.995185,   0.99248,    0.989177,
     0.985278,   0.980785,     0.975702,   0.970031,     0.963776,   0.95694,    0.949528,
     0.941544,   0.932993,     0.92388,    0.91421,      0.903989,   0.893224,   0.881921,
     0.870087,   0.857729,     0.844854,   0.83147,      0.817585,   0.803208,   0.788346,
     0.77301,    0.757209,     0.740951,   0.724247,     0.707107,   0.689541,   0.671559,
     0.653173,   0.634393,     0.615232,   0.595699,     0.575808,   0.55557,    0.534998,
     0.514103,   0.492898,     0.471397,   0.449611,     0.427555,   0.405241,   0.382683,
     0.359895,   0.33689,      0.313682,   0.290285,     0.266713,   0.24298,    0.219101,
     0.19509,    0.170962,     0.14673,    0.122411,     0.0980171,  0.0735646,  0.0490677,
     0.0245412,  6.12323e-17, -0.0245412, -0.0490677,   -0.0735646, -0.0980171, -0.122411,
    -0.14673,   -0.170962,    -0.19509,   -0.219101,    -0.24298,   -0.266713,  -0.290285,
    -0.313682,  -0.33689,     -0.359895,  -0.382683,    -0.405241,  -0.427555,  -0.449611,
    -0.471397,  -0.492898,    -0.514103,  -0.534998,    -0.55557,   -0.575808,  -0.595699,
    -0.615232,  -0.634393,    -0.653173,  -0.671559,    -0.689541,  -0.707107,  -0.724247,
    -0.740951,  -0.757209,    -0.77301,   -0.788346,    -0.803208,  -0.817585,  -0.83147,
    -0.844854,  -0.857729,    -0.870087,  -0.881921,    -0.893224,  -0.903989,  -0.91421,
    -0.92388,   -0.932993,    -0.941544,  -0.949528,    -0.95694,   -0.963776,  -0.970031,
    -0.975702,  -0.980785,    -0.985278,  -0.989177,    -0.99248,   -0.995185,  -0.99729,
    -0.998795,  -0.999699,    -1,         -0.999699,    -0.998795,  -0.99729,   -0.995185,
    -0.99248,   -0.989177,    -0.985278,  -0.980785,    -0.975702,  -0.970031,  -0.963776,
    -0.95694,   -0.949528,    -0.941544,  -0.932993,    -0.92388,   -0.91421,   -0.903989,
    -0.893224,  -0.881921,    -0.870087,  -0.857729,    -0.844854,  -0.83147,   -0.817585,
    -0.803208,  -0.788346,    -0.77301,   -0.757209,    -0.740951,  -0.724247,  -0.707107,
    -0.689541,  -0.671559,    -0.653173,  -0.634393,    -0.615232,  -0.595699,  -0.575808,
    -0.55557,   -0.534998,    -0.514103,  -0.492898,    -0.471397,  -0.449611,  -0.427555,
    -0.405241,  -0.382683,    -0.359895,  -0.33689,     -0.313682,  -0.290285,  -0.266713,
    -0.24298,   -0.219101,    -0.19509,   -0.170962,    -0.14673,   -0.122411,  -0.0980171,
    -0.0735646, -0.0490677,   -0.0245412, -1.83697e-16,  0.0245412,  0.0490677,  0.0735646,
     0.0980171,  0.122411,     0.14673,    0.170962,     0.19509,    0.219101,   0.24298,
     0.266713,   0.290285,     0.313682,   0.33689,      0.359895,   0.382683,   0.405241,
     0.427555,   0.449611,     0.471397,   0.492898,     0.514103,   0.534998,   0.55557,
     0.575808,   0.595699,     0.615232,   0.634393,     0.653173,   0.671559,   0.689541,
     0.707107,   0.724247,     0.740951,   0.757209,     0.77301,    0.788346,   0.803208,
     0.817585,   0.83147,      0.844854,   0.857729,     0.870087,   0.881921,   0.893224,
     0.903989,   0.91421,      0.92388,    0.932993,     0.941544,   0.949528,   0.95694,
     0.963776,   0.970031,     0.975702,   0.980785,     0.985278,   0.989177,   0.99248,
     0.995185,   0.99729,      0.998795,   0.999699
};
// clang-format on

#if __cplusplus <= 201402L
} // namespace UTILS
} // namespace GOOM
#else
} // namespace GOOM::UTILS
#endif
