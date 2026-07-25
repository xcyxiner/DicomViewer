
#pragma once
#include "ITaskQueue.h"
#include <QThreadPool>
class QtTaskQueue : public ITaskQueue
{
public:
    explicit QtTaskQueue( QThreadPool* threadPool = nullptr);
    ~QtTaskQueue()=default;
private:
    QThreadPool* m_threadPool;
public:
    std::future<void> submit(std::function<void()> task, int priority = 0) override;
    void cancelAll() override;
};
