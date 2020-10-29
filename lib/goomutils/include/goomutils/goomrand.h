#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_GOOMRAND_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_GOOMRAND_H_

#include <cmath>
#include <cstdlib>
#include <format>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace goom::utils
{

uint64_t getRandSeed();
void setRandSeed(const uint64_t seed);
extern const uint32_t randMax;

void saveRandState(std::ostream&);
void restoreRandState(std::istream&);

// Return random sign integer, either -1 or +1.
inline int getRandSignInt();
// Return random sign float, either -1.0 or +1.0.
inline float getRandSignFlt();

// Return random positive integer in the range n0 <= n < n1.
uint32_t getRandInRange(const uint32_t n0, const uint32_t n1);
// Return random integer in the range 0 <= n < nmax.
uint32_t getNRand(const uint32_t nmax);
// Return random integer in the range 0 <= n < randMax.
uint32_t getRand();
// Return random integer in the range n0 <= n < n1.
int32_t getRandInRange(const int32_t n0, const int32_t n1);
// Return random float in the range x0 <= n <= x1.
float getRandInRange(const float x0, const float x1);

// Return prob(m/n)
inline bool probabilityOfMInN(const uint32_t m, const uint32_t n);

template<class E>
class Weights
{
public:
  Weights() noexcept;
  explicit Weights(const std::vector<std::pair<E, size_t>>&);

  void clearWeights(const size_t value);
  size_t getNumElements() const;
  void setWeight(const E, size_t value);
  size_t getWeight(const E) const;

  size_t getSumOfWeights() const { return sumOfWeights; }

  E getRandomWeighted() const;

private:
  std::vector<std::pair<E, size_t>> weights;
  size_t sumOfWeights;
  static size_t getSumOfWeights(const std::vector<std::pair<E, size_t>>&);
};

inline int getRandSignInt()
{
  return getRandInRange(0u, 100u) < 50 ? -1 : +1;
}

inline float getRandSignFlt()
{
  return getRandInRange(0u, 100u) < 50 ? -1.0f : +1.0f;
}

inline uint32_t getNRand(const uint32_t nmax)
{
  return getRandInRange(0u, nmax);
}

inline uint32_t getRand()
{
  return getRandInRange(0u, randMax);
}

inline bool probabilityOfMInN(const uint32_t m, const uint32_t n)
{
  if (m == 1)
  {
    return getNRand(n) == 0;
  }
  if (m == n - 1)
  {
    return getNRand(n) > 0;
  }
  return getRandInRange(0.0f, 1.0f) <= static_cast<float>(m) / static_cast<float>(n);
}

template<class E>
Weights<E>::Weights() noexcept : weights{}, sumOfWeights{0}
{
}

template<class E>
Weights<E>::Weights(const std::vector<std::pair<E, size_t>>& w)
  : weights{w}, sumOfWeights{getSumOfWeights(w)}
{
}

template<class E>
size_t Weights<E>::getSumOfWeights(const std::vector<std::pair<E, size_t>>& weights)
{
  size_t sumOfWeights = 0;
  for (const auto& [e, w] : weights)
  {
    sumOfWeights += w;
  }
  return sumOfWeights;
}

template<class E>
size_t Weights<E>::getNumElements() const
{
  return weights.size();
}

template<class E>
void Weights<E>::setWeight(const E enumClass, size_t value)
{
  for (auto& [e, w] : weights)
  {
    if (e == enumClass)
    {
      w = value;
      sumOfWeights = getSumOfWeights(weights);
      return;
    }
  }
  weights.emplace_back(std::make_pair(enumClass, value));
  sumOfWeights = getSumOfWeights(weights);
}

template<class E>
size_t Weights<E>::getWeight(const E enumClass) const
{
  for (const auto& [e, w] : weights)
  {
    if (e == enumClass)
    {
      return w;
    }
  }
  return 0;
}

template<class E>
void Weights<E>::clearWeights(const size_t value)
{
  for (auto& [e, w] : weights)
  {
    w = value;
  }
  sumOfWeights = getSumOfWeights(weights);
}

template<class E>
E Weights<E>::getRandomWeighted() const
{
  if (weights.empty())
  {
    throw std::logic_error("The are no weights set.");
  }

  uint32_t randVal = getRandInRange(0u, sumOfWeights);
  for (const auto& [e, w] : weights)
  {
    if (randVal < w)
    {
      return e;
    }
    randVal -= w;
  }
  throw std::logic_error(std20::format("Should not get here. randVal = {}.", randVal));
}

} // namespace goom::utils
#endif
