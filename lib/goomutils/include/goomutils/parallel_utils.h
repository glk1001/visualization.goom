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
  explicit Parallel(const int32_t numPoolThreads = 0) noexcept;
  Parallel(const Parallel&) = delete;
  Parallel& operator=(const Parallel&) = delete;

  template<typename Callable>
  void forLoop(const int32_t numIters, const Callable loopFunc);

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

//
/// \param n The number of iterations. I.e., { 0, 1, ..., n - 1 } will be visited.
/// \param function The function that will be called in the for-loop. This can be specified as a lambda expression. The type should be equivalent to std::function<void(int)>.
template<typename Callable>
void Parallel::forLoop(const int32_t numIters, const Callable loopFunc)
{
  const int32_t useNumThreads = threadPool.NumWorkers();
  const int32_t numThreads = std::min(numIters, (useNumThreads == 0) ? 4 : useNumThreads);

  const int32_t maxTasksPerThread = (numIters / numThreads) + (numIters % numThreads == 0 ? 0 : 1);
  const int32_t numUnneededTasks = maxTasksPerThread * numThreads - numIters;

  const auto loopContents = [&](const int32_t threadIndex) {
    const int32_t numUnneededTasksSoFar = std::max(threadIndex - numThreads + numUnneededTasks, 0);
    const int32_t inclusiveStartIndex = threadIndex * maxTasksPerThread - numUnneededTasksSoFar;
    const int32_t exclusiveEndIndex = inclusiveStartIndex + maxTasksPerThread -
                                      (threadIndex - numThreads + numUnneededTasks >= 0 ? 1 : 0);

    for (uint32_t k = static_cast<uint32_t>(inclusiveStartIndex);
         k < static_cast<uint32_t>(exclusiveEndIndex); k++)
    {
      loopFunc(k);
    }
  };

  std::vector<std::future<void>> futures{};
  for (int32_t j = 0; j < numThreads; j++)
  {
    futures.emplace_back(threadPool.ScheduleAndGetFuture(loopContents, j));
    //    threadPool.ScheduleAndGetFuture(loopContents, j);
  }

  //  threadPool.Wait();
  for (auto& f : futures)
  {
    f.wait();
  }
}

} // namespace goom::utils
#endif /* LIBS_GOOMUTILS_INCLUDE_GOOMUTILS_COLORMAP_H_ */
