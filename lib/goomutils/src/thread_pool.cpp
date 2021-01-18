#include "thread_pool.h"

#include <cassert>
#include <functional>
#include <thread>
#include <utility>

namespace GOOM::UTILS
{

ThreadPool::ThreadPool(const size_t numWrkers) noexcept : numWorkers(numWrkers)
{
  assert(numWorkers > 0);
  // TODO(cbraley): Handle thread construction exceptions.
  workers.reserve(numWorkers);
  for (size_t i = 0; i < numWorkers; ++i)
  {
    workers.emplace_back(&ThreadPool::threadLoop, this);
  }
}

ThreadPool::~ThreadPool() noexcept
{
  // TODO(cbraley): The current thread could help out to drain the work_ queue
  // faster - for example, if there is work that hasn't yet been scheduled this
  // thread could "pitch in" to help finish faster.

  {
    const std::lock_guard<std::mutex> lock{mu};
    finished = true;
  }
  newWorkCondition.notify_all(); // Tell *all* workers we are ready.

  for (std::thread& thread : workers)
  {
    thread.join();
  }
}

int ThreadPool::OutstandingWorkSize() const
{
  const std::lock_guard<std::mutex> lock{mu};
  return workQueue.size();
}

size_t ThreadPool::NumWorkers() const
{
  return numWorkers;
}

void ThreadPool::SetWorkDoneCallback(std::function<void(int)> func)
{
  workDoneCallback = std::move(func);
}

void ThreadPool::Wait()
{
  std::unique_lock<std::mutex> lock{mu};
  if (!workQueue.empty())
  {
    workDoneCondition.wait(lock, [this] { return workQueue.empty(); });
  }
}

void ThreadPool::threadLoop()
{
  // Wait until the ThreadPool sends us work.
  while (true)
  {
    WorkItem workItem{};

    int prevWorkSize = -1;
    {
      std::unique_lock<std::mutex> lock{mu};
      newWorkCondition.wait(lock, [this] { return finished || (!workQueue.empty()); });
      // ...after the wait(), we hold the lock.

      // If all the work is done and exit_ is true, break out of the loop.
      if (finished && workQueue.empty())
      {
        break;
      }

      // Pop the work off of the queue - we are careful to execute the
      // work_item.func callback only after we have released the lock.
      prevWorkSize = workQueue.size();
      workItem = std::move(workQueue.front());
      workQueue.pop();
    }

    // We are careful to do the work without the lock held!
    // TODO(cbraley): Handle exceptions properly.
    workItem.func(); // Do work.

    if (workDoneCallback)
    {
      workDoneCallback(prevWorkSize - 1);
    }

    // Notify if all work is done.
    {
      const std::unique_lock<std::mutex> lock{mu};
      if (workQueue.empty() && prevWorkSize == 1)
      {
        workDoneCondition.notify_all();
      }
    }
  }
}

} // namespace GOOM::UTILS
