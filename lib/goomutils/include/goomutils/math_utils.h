#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_MATH_UTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_MATH_UTILS_H_

#include <cmath>
#include <cstdlib>
#include <format>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

inline float getRandNeg1toPos1();

// Return random sign int, either -1 or +1.
inline int getRandSignInt();
// Return random sign float, either -1.0 or +1.0.
inline float getRandSignFlt();

// Return random int in the range x0 <= n < x1.
inline size_t getRandInRange(const size_t x0, const size_t x1);
// Return random float in the range x0 <= n <= x1.
inline float getRandInRange(const float x0, const float x1);

template <class E>
class Weights {
public:
  Weights() noexcept;
  explicit Weights(const std::vector<std::pair<E, size_t>>&);

  void setWeight(const E, size_t value);
  size_t getWeight(const E) const;
  void clearWeights(const size_t value);
  size_t getSumOfWeights() const { return sumOfWeights; }

  E getRandomWeighted() const;
private:
  std::vector<std::pair<E, size_t>> weights;
  size_t sumOfWeights;
  static size_t getSumOfWeights(const std::vector<std::pair<E, size_t>>&);
};

class RangeMapper {
public:
  explicit RangeMapper(const double x0, const double x1);
  double operator()(const double r0, const double r1, const double x) const;
  double getXMin() const { return xmin; }
  double getXMax() const { return xmax; }
private:
  double xmin;
  double xmax;
  double xwidth;
};

class DampingFunction {
public:
  virtual ~DampingFunction() {}
  virtual double operator()(const double x)=0;
};

class NoDampingFunction: public DampingFunction {
public:
  double operator()([[maybe_unused]]const double x) override { return 1.0; }
};

class LogDampingFunction: public DampingFunction {
public:
  explicit LogDampingFunction(const double amplitude, const double xmin, const double xStart=2.0);
  double operator()(const double x) override;
private:
  const double amplitude;
  const double xmin;
  const double xStart;
};

// Asymptotes to 'amplitude' in negative x direction,
//   and the rate depends on the input parameters.
class ExpDampingFunction: public DampingFunction {
public:
  explicit ExpDampingFunction(
      const double amplitude,
      const double xToStartRise, const double yAtStartToRise,
      const double xmax, const double yAtXMax);
  double operator()(const double x) override;
  double kVal() const { return k; }
  double bVal() const { return b; }
private:
  const double amplitude;
  double k;
  double b;
};

class FlatDampingFunction: public DampingFunction {
public:
  explicit FlatDampingFunction(const double y);
  double operator()(const double x) override;
private:
  const double y;
};

class LinearDampingFunction: public DampingFunction {
public:
  explicit LinearDampingFunction(const double x0, const double y0, const double x1, const double y1);
  double operator()(const double x) override;
private:
  const double m;
  const double x1;
  const double y1;
};

class PiecewiseDampingFunction: public DampingFunction {
public:
  explicit PiecewiseDampingFunction(
      std::vector<std::tuple<double, double, std::unique_ptr<DampingFunction>>>& pieces);
  double operator()(const double x) override;
private:
  std::vector<std::tuple<double, double, std::unique_ptr<DampingFunction>>> pieces;
};

class SequenceFunction {
public:
  virtual ~SequenceFunction() {}
  virtual float getNext()=0;
};

class ConstantSequenceFunction: public SequenceFunction {
public:
  explicit ConstantSequenceFunction(const float v=0): val{ v }{}
  float getNext() override { return val; }
  float getConstVal() { return val; }
  void setConstVal(const double v) { val = v; }
private:
  float val;
};

class RandSequenceFunction: public SequenceFunction {
public:
  explicit RandSequenceFunction(const float _x0=0, const float _x1=1): x0{ _x0 }, x1{ _x1 } {}
  float getNext() override { return getRandInRange(x0, x1); }
  void setX0(const double val) { x0 = val; }
  void setX1(const double val) { x1 = val; }
private:
  float x0;
  float x1;
};

class SineWaveMultiplier: public SequenceFunction {
public:
  SineWaveMultiplier(const float frequency=1.0,
      const float lower=-1.0, const float upper=1.0, const float x0=0.0);

  float getNext() override;

  void setX0(const float x0) { x = x0; }
  float getFrequency() const { return frequency; }
  void setFrequency(const float val) { frequency = val; }

  float getLower() const { return lower; }
  float getUpper() const { return upper; }
  void setRange(const float lwr, const float upr) { lower = lwr; upper = upr; }

  void setPiStepFrac(const float val) { piStepFrac = val; }
private:
  RangeMapper rangeMapper;
  float frequency;
  float lower;
  float upper;
  float piStepFrac;
  float x;
};

class HermitePolynomial: public SequenceFunction {
public:
  explicit HermitePolynomial(const int degree, const float xStart=0.0);
  float getNext() override;
private:
  int degree;
  float stepFrac;
  const float x0;
  const float x1;
  float x;
};

class IncreasingFunction {
public:
  explicit IncreasingFunction(const double xmin, const double xmax);
  virtual ~IncreasingFunction() {}
  virtual double operator()(const double x)=0;
protected:
  const double xmin;
  const double xmax;
};

// Asymptotes to 1, depending on 'k' value.
class ExpIncreasingFunction: public IncreasingFunction {
public:
  explicit ExpIncreasingFunction(const double xmin, const double xmax, const double k=0.1);
  double operator()(const double x) override;
private:
  const double k;
};

class LogIncreasingFunction: public IncreasingFunction {
public:
  explicit LogIncreasingFunction(const double amplitude, const double xmin, const double xStart=2.0);
  double operator()(const double x) override;
private:
  const double amplitude;
  const double xmin;
  const double xStart;
};

inline IncreasingFunction::IncreasingFunction(const double x0, const double x1)
  : xmin(x0)
  , xmax(x1)
{
}

inline RangeMapper::RangeMapper(const double x0, const double x1)
  : xmin(x0)
  , xmax(x1)
  , xwidth(xmax - xmin)
{
}

inline double RangeMapper::operator()(const double r0, const double r1, const double x) const
{
  return std::lerp(r0, r1, (x - xmin)/xwidth);
}

inline float getRandNeg1toPos1()
{
  return 2.0*(float(std::rand())/float(RAND_MAX) - 0.5);
}

inline int getRandSignInt()
{
  return getRandInRange(0ul, 100ul) < 50 ? -1 : +1;
}

inline float getRandSignFlt()
{
  return getRandInRange(0ul, 100ul) < 50 ? -1.0f : +1.0f;
}

inline float getRandInRange(const float x0, const float x1)
{
  return std::lerp(x0, x1, float(std::rand())/float(RAND_MAX));
}

inline size_t getRandInRange(const size_t x0, const size_t x1)
{
  return x0 + size_t(std::rand()) % (x1 - x0);
}

template <class E>
Weights<E>::Weights() noexcept
  : weights{}
  , sumOfWeights{ 0 }
{
}

template <class E>
Weights<E>::Weights(const std::vector<std::pair<E, size_t>>& w)
  : weights{ w }
  , sumOfWeights{ getSumOfWeights(w) }
{
}

template <class E>
size_t Weights<E>::getSumOfWeights(const std::vector<std::pair<E, size_t>>& weights)
{
  size_t sumOfWeights = 0;
  for (const auto& [e, w] : weights) {
    sumOfWeights += w;
  }
  return sumOfWeights;
}

template <class E>
void Weights<E>::setWeight(const E enumClass, size_t value)
{
  for (auto& [e, w] : weights) {
    if (e == enumClass) {
      w = value;
      sumOfWeights = getSumOfWeights(weights);
      return;
    }
  }
  weights.emplace_back(std::make_pair(enumClass, value));
  sumOfWeights = getSumOfWeights(weights);
}

template <class E>
size_t Weights<E>::getWeight(const E enumClass) const
{
  for (const auto& [e, w] : weights) {
    if (e == enumClass) {
      return w;
    }
  }
  return 0;
}

template <class E>
void Weights<E>::clearWeights(const size_t value)
{
  for (auto& [e, w] : weights) {
    w = value;
  }
  sumOfWeights = getSumOfWeights(weights);
}

template <class E>
E Weights<E>::getRandomWeighted() const
{
  if (weights.empty()) {
    throw std::logic_error("The are no weights set.");
  }

  size_t randVal = getRandInRange(0, sumOfWeights);
  for (const auto& [e, w] : weights) {
    if (randVal < w) {
      return e;
    }
    randVal -= w;
  }
  throw std::logic_error(stdnew::format("Should not get here. randVal = {}.", randVal));
}

#endif
