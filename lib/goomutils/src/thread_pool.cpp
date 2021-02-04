#include "thread_pool.h"

#include <cassert>
#include <functional>
#include <thread>
#include <utility>

#if __cplusplus <= 201402L
namespace GOOM
{
namespace UTILS
{
#else
namespace GOOM::UTILS
{
#endif

ThreadPool::ThreadPool(const size_t numWorkers) noexcept : m_numWorkers(numWorkers)
{
  assert(m_numWorkers > 0);
  // TODO(cbraley): Handle thread construction exceptions.
  m_workers.reserve(m_numWorkers);
  for (size_t i = 0; i < m_numWorkers; ++i)
  {
    m_workers.emplace_back(&ThreadPool::ThreadLoop, this);
  }
}

ThreadPool::~ThreadPool() noexcept
{
  // TODO(cbraley): The current thread could help out to drain the work_ queue
  // faster - for example, if there is work that hasn't yet been scheduled this
  // thread could "pitch in" to help finish faster.

  {
    const std::lock_guard<std::mutex> lock{m_mutex};
    m_finished = true;
  }
  m_newWorkCondition.notify_all(); // Tell *all* workers we are ready.

  for (std::thread& thread : m_workers)
  {
    thread.join();
  }
}

auto ThreadPool::OutstandingWorkSize() const -> int
{
  const std::lock_guard<std::mutex> lock{m_mutex};
  return m_workQueue.size();
}

auto ThreadPool::NumWorkers() const -> size_t
{
  return m_numWorkers;
}

void ThreadPool::SetWorkDoneCallback(std::function<void(int)> func)
{
  m_workDoneCallback = std::move(func);
}

void ThreadPool::Wait()
{
  std::unique_lock<std::mutex> lock{m_mutex};
  if (!m_workQueue.empty())
  {
    m_workDoneCondition.wait(lock, [this] { return m_workQueue.empty(); });
  }
}

void ThreadPool::ThreadLoop()
{
  // Wait until the ThreadPool sends us work.
  while (true)
  {
    WorkItem workItem{};

    int prevWorkSize = -1;
    {
      std::unique_lock<std::mutex> lock{m_mutex};
      m_newWorkCondition.wait(lock, [this] { return m_finished || (!m_workQueue.empty()); });
      // ...after the wait(), we hold the lock.

      // If all the work is done and exit_ is true, break out of the loop.
      if (m_finished && m_workQueue.empty())
      {
        break;
      }

      // Pop the work off of the queue - we are careful to execute the
      // work_item.func callback only after we have released the lock.
      prevWorkSize = m_workQueue.size();
      workItem = std::move(m_workQueue.front());
      m_workQueue.pop();
    }

    // We are careful to do the work without the lock held!
    // TODO(cbraley): Handle exceptions properly.
    workItem.func(); // Do work.

    if (m_workDoneCallback)
    {
      m_workDoneCallback(prevWorkSize - 1);
    }

    // Notify if all work is done.
    {
      const std::unique_lock<std::mutex> lock{m_mutex};
      if (m_workQueue.empty() && prevWorkSize == 1)
      {
        m_workDoneCondition.notify_all();
      }
    }
  }
}

#if __cplusplus <= 201402L
} // namespace UTILS
} // namespace GOOM
#else
} // namespace GOOM::UTILS
#endif
