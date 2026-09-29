#pragma once
// 测试共享脚手架（header-only，各测试可执行文件独立编译，不改
// test/CMakeLists.txt 的目标结构）：
//   1. waitFor 事件泵 —— 跨线程信号回投主线程靠 QueuedConnection，
//      泵事件直到条件满足。
// 不引入 QtTest 依赖（QTest::qWait 会新增链接要求）。
//
// 原先的第 2 项 res/series/expected.txt 期望值解析（SeriesExpectation /
// loadExpectation / fixtureFrameCount）已随夹具移除而挪到
// test/deprecated/TestCommon.h，待夹具回归后恢复。

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>
#include <functional>

#include <gtest/gtest.h>

// 泵事件循环直到条件满足（跨线程信号回投主线程靠 QueuedConnection）。
inline bool waitFor(const std::function<bool()>& predicate,
                    int timeoutMs = 10000)
{
  QElapsedTimer timer;
  timer.start();
  while (!predicate() && timer.elapsed() < timeoutMs) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(5);
  }
  return predicate();
}
