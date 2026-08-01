#include "QtTaskQueue.h"

QtTaskQueue::QtTaskQueue(QThreadPool* threadPool)
    : m_threadPool(threadPool ? threadPool : QThreadPool::globalInstance())
{
}

std::future<void> QtTaskQueue::submit(std::function<void()> task, int priority)
{
  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();
  auto runnable = [promise, task = std::move(task)]()
  {
    try {
      task();
      promise->set_value();
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
  };

  struct TaskRunable : public QRunnable
  {
    std::function<void()> m_task;

    void run() override { m_task(); }
  };

  auto qrunnable = new TaskRunable();
  qrunnable->m_task = std::move(runnable);
  qrunnable->setAutoDelete(true);
  m_threadPool->start(qrunnable, priority);
  return future;
}

void QtTaskQueue::cancelAll()
{
  m_threadPool->clear();
}
