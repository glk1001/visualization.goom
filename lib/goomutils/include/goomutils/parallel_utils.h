#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_PARALLEL_UTILS_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_PARALLEL_UTILS_H_

#include "thread_pool.h"

#include <algorithm>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>

namespace goom::utils
{

class Parallel
{
public:
  // numPoolThreads > 0:  use this number of threads in pool
  // numPoolThreads <= 0: use max cores - this number of threads
  explicit Parallel(int32_t numPoolThreads = 0) noexcept;
  Parallel(const Parallel&) = delete;
  Parallel& operator=(const Parallel&) = delete;

  size_t getNumThreadsUsed() const;

  template<typename Callable>
  void forLoop(uint32_t numIters, Callable loopFunc);

private:
  ThreadPool threadPool;
};

inline Parallel::Parallel(const int32_t numPoolThreads) noexcept
  : threadPool{(numPoolThreads <= 0)
                   ? static_cast<size_t>(static_cast<int32_t>(std::thread::hardware_concurrency()) +
                                         numPoolThreads)
                   : static_cast<size_t>(numPoolThreads)}
{
}

inline size_t Parallel::getNumThreadsUsed() const
{
  return threadPool.NumWorkers();
}

template<typename Callable>
void Parallel::forLoop(const uint32_t numIters, const Callable loopFunc)
{
  const uint32_t numThreads = threadPool.NumWorkers();

  if (numIters < numThreads)
  {
    for (uint32_t i = 0; i < numIters; i++)
    {
      loopFunc(i);
    }
    return;
  }

  const uint32_t chunkSize = numIters / numThreads; // >= 1
  const uint32_t numLeftoverIters = numIters - numThreads * chunkSize;

  const auto loopContents = [&](const uint32_t threadIndex) {
    const uint32_t inclusiveStartIndex = threadIndex * chunkSize;
    const uint32_t exclusiveEndIndex =
        inclusiveStartIndex + chunkSize + (threadIndex < numThreads - 1 ? 0 : numLeftoverIters);

    for (uint32_t k = inclusiveStartIndex; k < exclusiveEndIndex; k++)
    {
      loopFunc(k);
    }
  };

  std::vector<std::future<void>> futures{};
  for (uint32_t j = 0; j < numThreads; j++)
  {
    futures.emplace_back(threadPool.ScheduleAndGetFuture(loopContents, j));
  }

  for (auto& f : futures)
  {
    f.wait();
  }
}

} // namespace goom::utils
#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
