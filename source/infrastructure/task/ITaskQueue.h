
#pragma once
#include <future>
#include <functional>
class ITaskQueue 
{
public:
    explicit ITaskQueue();
    ~ITaskQueue()=default;
public:
  virtual std::future<void> submit(std::function<void()> task,
                                   int priority = 0) = 0;
  template<typename T>
  auto submitWithResult(T&& f, int priority = 0) -> std::future<decltype(f())>
  {
    auto promise = std::make_shared<std::promise<decltype(f())>>();
    auto future = promise->get_future();
    submit(
        [promise, f = std::forward<T>(f)]()
        {
          try {
            promise->set_value(f());
          } catch (...) {
            promise->set_exception(std::current_exception());
          }
        },
        priority);
    return future;
  }
  virtual void cancelAll() = 0;
};
