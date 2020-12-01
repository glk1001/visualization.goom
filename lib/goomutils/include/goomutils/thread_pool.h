#ifndef LIB_GOOMUTILS_INCLUDE_GOOMUTILS_THREAD_POOL_H_
#define LIB_GOOMUTILS_INCLUDE_GOOMUTILS_THREAD_POOL_H_

#include "thread_pool_macros.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace goom::utils
{

class ThreadPool
{
public:
  explicit ThreadPool(const size_t numWorkers) noexcept;
  ThreadPool() noexcept = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) noexcept = delete;
  ThreadPool& operator=(ThreadPool&&) noexcept = delete;

  // The `ThreadPool` destructor blocks until all outstanding work is complete.
  ~ThreadPool() noexcept;

  // Add `func` to the thread pool, and return a std::future that can be used to
  // access the function's return value.
  //
  // *** Usage example ***
  //   Don't be alarmed by this function's tricky looking signature - this is
  //   very easy to use. Here's an example:
  //
  //   int ComputeSum(std::vector<int>& values) {
  //     int sum = 0;
  //     for (const int& v : values) {
  //       sum += v;
  //     }
  //     return sum;
  //   }
  //
  //   ThreadPool pool = ...;
  //   std::vector<int> numbers = ...;
  //
  //   std::future<int> sum_future = ScheduleAndGetFuture(
  //     []() {
  //       return ComputeSum(numbers);
  //     });
  //
  //   // Do other work...
  //
  //   std::cout << "The sum is " << sum_future.get() << std::endl;
  //
  // *** Details ***
  //   Given a callable `func` that returns a value of type `RetT`, this
  //   function returns a std::future<RetT> that can be used to access
  //   `func`'s results.
  template<typename FuncT, typename... ArgsT>
  auto ScheduleAndGetFuture(FuncT&& func, ArgsT&&... args)
      -> std::future<decltype(INVOKE_MACRO(func, ArgsT, args))>;

  // Wait for all outstanding work to be completed.
  void Wait();

  // Return the number of outstanding functions to be executed.
  int OutstandingWorkSize() const;

  size_t NumWorkers() const;

  void SetWorkDoneCallback(std::function<void(int)> func);

private:
  size_t numWorkers = 0;

  // The destructor sets 'finished' to true and then notifies all workers.
  // 'finished' causes each thread to break out of their work loop.
  bool finished = false;

  mutable std::mutex mu{};

  // Work queue. Guarded by 'mu'.
  struct WorkItem
  {
    std::function<void(void)> func{};
  };
  std::queue<WorkItem> workQueue{};

  std::vector<std::thread> workers{};
  std::condition_variable newWorkCondition{};
  std::condition_variable workDoneCondition{};

  // Whenever a work item is complete, we call this callback. If this is empty,
  // nothing is done.
  std::function<void(int)> workDoneCallback{};

  void threadLoop();
};

namespace impl
{

// This helper class simply returns a std::function that executes:
//   ReturnT x = func();
//   promise->set_value(x);
// However, this is tricky in the case where T == void. The code above won't
// compile if ReturnT == void, and neither will
//   promise->set_value(func());
// To workaround this, we use a template specialization for the case where
// ReturnT is void. If the "regular void" proposal is accepted, this could be
// simpler:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0146r1.html.

// The non-specialized `FuncWrapper` implementation handles callables that
// return a non-void value.
template<typename ReturnT>
struct FuncWrapper
{
  template<typename FuncT, typename... ArgsT>
  std::function<void()> GetWrapped(FuncT&& func,
                                   std::shared_ptr<std::promise<ReturnT>> promise,
                                   ArgsT&&... args)
  {
    // TODO(cbraley): Capturing by value is inefficient. It would be more
    // efficient to move-capture everything, but we can't do this until C++14
    // generalized lambda capture is available. Can we use std::bind instead to
    // make this more efficient and still use C++11?
    return
        [promise, func, args...]() mutable { promise->set_value(INVOKE_MACRO(func, ArgsT, args)); };
  }
};

template<typename FuncT, typename... ArgsT>
void InvokeVoidRet(FuncT&& func, std::shared_ptr<std::promise<void>> promise, ArgsT&&... args)
{
  INVOKE_MACRO(func, ArgsT, args);
  promise->set_value();
}

// This `FuncWrapper` specialization handles callables that return void.
template<>
struct FuncWrapper<void>
{
  template<typename FuncT, typename... ArgsT>
  std::function<void()> GetWrapped(FuncT&& func,
                                   std::shared_ptr<std::promise<void>> promise,
                                   ArgsT&&... args)
  {
    return [promise, func, args...]() mutable {
      INVOKE_MACRO(func, ArgsT, args);
      promise->set_value();
    };
  }
};

} // namespace impl

template<typename FuncT, typename... ArgsT>
auto ThreadPool::ScheduleAndGetFuture(FuncT&& func, ArgsT&&... args)
    -> std::future<decltype(INVOKE_MACRO(func, ArgsT, args))>
{
  using ReturnT = decltype(INVOKE_MACRO(func, ArgsT, args));

  // We are only allocating this std::promise in a shared_ptr because
  // std::promise is non-copyable.
  std::shared_ptr<std::promise<ReturnT>> promise = std::make_shared<std::promise<ReturnT>>();
  std::future<ReturnT> ret_future = promise->get_future();

  impl::FuncWrapper<ReturnT> funcWrapper{};
  std::function<void()> wrappedFunc =
      funcWrapper.GetWrapped(std::move(func), std::move(promise), std::forward<ArgsT>(args)...);

  // Acquire the lock, and then push the WorkItem onto the queue.
  {
    const std::lock_guard<std::mutex> lock{mu};
    WorkItem work{};
    work.func = std::move(wrappedFunc);
    workQueue.emplace(std::move(work));
  }
  newWorkCondition.notify_one(); // Tell one worker we are ready.

  return ret_future;
}

} // namespace goom::utils

#endif // LIB_GOOMUTILS_INCLUDE_GOOMUTILS_THREAD_POOL_H_
