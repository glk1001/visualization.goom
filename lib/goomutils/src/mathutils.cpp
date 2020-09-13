#include "goomutils/mathutils.h"

#include <array>
#include <cmath>
#include <format>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

ExpIncreasingFunction::ExpIncreasingFunction(const double x0, const double x1, const double _k)
  : IncreasingFunction(x1, x0), k(_k)
{
}

double ExpIncreasingFunction::operator()(const double x)
{
  return 1.0 - std::exp(-k * std::fabs(x));
}

LogIncreasingFunction::LogIncreasingFunction(const double amp, const double xm, const double xStrt)
  : IncreasingFunction(xm, 10000000000), amplitude(amp), xmin(xm), xStart(xStrt)
{
}

double LogIncreasingFunction::operator()(const double x)
{
  return amplitude / std::log(x - xmin + xStart);
}


LogDampingFunction::LogDampingFunction(const double amp, const double xm, const double xStrt)
  : amplitude(amp), xmin(xm), xStart(xStrt)
{
}

double LogDampingFunction::operator()(const double x)
{
  return amplitude * std::log(x - xmin + xStart);
}

ExpDampingFunction::ExpDampingFunction(const double amp,
                                       const double xToStartRise,
                                       const double yAtStartToRise,
                                       const double xmax,
                                       const double yAtXMax)
    : k{ 0 },
      b{ 0 },
      amplitude(amp)
{
  constexpr double minAmp = 0.00001;
  if (std::fabs(amplitude) < minAmp)
  {
    throw std::runtime_error(
        std20::format("abs(amplitude) should be >= {}, not {}.", minAmp, amplitude));
  }
  if (yAtStartToRise <= amplitude)
  {
    throw std::runtime_error(std20::format("yAtStartToRise should be > {} = amplitude, not {}.",
                                            amplitude, yAtStartToRise));
  }
  if (yAtXMax <= amplitude)
  {
    throw std::runtime_error(
        std20::format("yAtXMax should be > {} = amplitude, not {}.", amplitude, yAtXMax));
  }
  const double y0 = yAtStartToRise / amplitude - 1.0;
  const double y1 = yAtXMax / amplitude - 1.0;
  const double log_y0 = std::log(y0);
  const double log_y1 = std::log(y1);
  b = (xToStartRise * log_y1 - xmax * log_y0) / (log_y1 - log_y0);
  k = log_y1 / (xmax - b);
}

double ExpDampingFunction::operator()(const double x)
{
  return amplitude * (1.0 + std::exp(k * (x - b)));
}

FlatDampingFunction::FlatDampingFunction(const double y_) : y{y_}
{
}

double FlatDampingFunction::operator()([[maybe_unused]] const double x)
{
  return y;
}

LinearDampingFunction::LinearDampingFunction(const double x0_,
                                             const double y0_,
                                             const double x1_,
                                             const double y1_)
  : m{(y1_ - y0_) / (x1_ - x0_)}, x1{x1_}, y1{y1_}
{
}

double LinearDampingFunction::operator()(const double x)
{
  return m * (x - x1) + y1;
}

PiecewiseDampingFunction::PiecewiseDampingFunction(
    std::vector<std::tuple<double, double, std::unique_ptr<DampingFunction>>>& p)
  : pieces{std::move(p)}
{
}

double PiecewiseDampingFunction::operator()(const double x)
{
  for (auto& [x0, x1, func] : pieces)
  {
    if ((x0 <= x) && (x < x1))
    {
      return (*func)(x);
    }
  }
  return 0.0;
}

HermitePolynomial::HermitePolynomial(const int d, const float xStart)
  : degree(d), stepFrac(1.0 / 16.0), x0(-1.5), x1(+2.5), x(xStart)
{
}

float HermitePolynomial::getNext()
{
  const float val = 0.01 * std::hermite(degree, x);
  x += stepFrac;
  if (x > x1)
  {
    x = x0;
  }
  return val;
}

SineWaveMultiplier::SineWaveMultiplier(const float freq,
                                       const float lwr,
                                       const float upr,
                                       const float x0)
    : x{ x0 },
      frequency{ freq },
      lower{ lwr },
      upper{ upr },
      piStepFrac{ 1.0 / 16.0 },
      rangeMapper{ -1, +1 }
{
}

float SineWaveMultiplier::getNext()
{
  const float val = rangeMapper(lower, upper, sin(frequency * x));
  //logInfo("lower = {}, upper = {}, sin({}) = {}.", lower, upper, x, val);
  x += piStepFrac * m_pi;
  return val;
}

// clang-format off
const std::array<float, numSinCosAngles> sin256 = {
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
const std::array<float, numSinCosAngles> cos256 = {
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
