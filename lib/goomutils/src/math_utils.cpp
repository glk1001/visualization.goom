#include "goomutils/math_utils.h"

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
  : amplitude(amp), k{0}, b{0}
{
  constexpr double minAmp = 0.00001;
  if (std::fabs(amplitude) < minAmp)
  {
    throw std::runtime_error(
        stdnew::format("abs(amplitude) should be >= {}, not {}.", minAmp, amplitude));
  }
  if (yAtStartToRise <= amplitude)
  {
    throw std::runtime_error(stdnew::format("yAtStartToRise should be > {} = amplitude, not {}.",
                                            amplitude, yAtStartToRise));
  }
  if (yAtXMax <= amplitude)
  {
    throw std::runtime_error(
        stdnew::format("yAtXMax should be > {} = amplitude, not {}.", amplitude, yAtXMax));
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
  : rangeMapper{-1, +1}, frequency{freq}, lower{lwr}, upper{upr}, piStepFrac{1.0 / 16.0}, x{x0}
{
}

float SineWaveMultiplier::getNext()
{
  const float val = rangeMapper(lower, upper, sin(frequency * x));
  //logInfo(stdnew::format("lower = {}, upper = {}, sin({}) = {}.", lower, upper, x, val));
  x += piStepFrac * M_PI;
  return val;
}
