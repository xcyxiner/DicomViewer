
#pragma once
// 测试共享脚手架（header-only，各测试可执行文件独立编译，不改
// test/CMakeLists.txt 的目标结构）：
//   1. res/series/expected.txt 期望值解析 —— 全字段超集，消费方只
//      读自己断言的字段（原先四处子集解析复制在此合并）；
//   2. waitFor 事件泵 —— 跨线程信号回投主线程靠 QueuedConnection，
//      泵事件直到条件满足（原先三处逐字节相同的定义在此合并）。
// 不引入 QtTest 依赖（QTest::qWait 会新增链接要求）。

#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>

#include <gtest/gtest.h>

// res/series/expected.txt —— tools/gen_series_fixture.py 的期望值
// （夹具入库，键结构与生成脚本绑定）。
struct SeriesExpectation
{
  std::string seriesUid;
  int count = 0;
  std::string baseUid;
  int rows = 0;
  int cols = 0;
  double pixelSpacingX = 0.0;
  double pixelSpacingY = 0.0;
  double sliceThickness = 0.0;
  double windowWidth = 0.0;
  double windowCenter = 0.0;
  double ippX = 0.0;
  double ippY = 0.0;
  std::vector<double> ippZ;
  std::vector<std::string> uids;
};

inline SeriesExpectation loadExpectation(const std::string& path)
{
  std::ifstream file(path);
  EXPECT_TRUE(file.good()) << "缺少夹具期望文件: " << path;
  std::map<std::string, std::string> kv;
  std::string line;
  while (std::getline(file, line)) {
    auto pos = line.find('=');
    if (pos != std::string::npos) {
      kv[line.substr(0, pos)] = line.substr(pos + 1);
    }
  }
  SeriesExpectation exp;
  exp.seriesUid = kv["series_uid"];
  exp.count = std::stoi(kv["count"]);
  exp.baseUid = kv["base_uid"];
  exp.rows = std::stoi(kv["rows"]);
  exp.cols = std::stoi(kv["cols"]);
  exp.pixelSpacingX = std::stod(kv["pixel_spacing_x"]);
  exp.pixelSpacingY = std::stod(kv["pixel_spacing_y"]);
  exp.sliceThickness = std::stod(kv["slice_thickness"]);
  exp.windowWidth = std::stod(kv["window_width"]);
  exp.windowCenter = std::stod(kv["window_center"]);
  exp.ippX = std::stod(kv["ipp_x"]);
  exp.ippY = std::stod(kv["ipp_y"]);
  for (int i = 0; i < exp.count; ++i) {
    exp.ippZ.push_back(std::stod(kv["ipp_z_" + std::to_string(i)]));
    exp.uids.push_back(kv["uid_" + std::to_string(i)]);
  }
  return exp;
}

// 夹具层数（expected.txt 的 count=，与夹具生成脚本绑定）。
inline int fixtureFrameCount()
{
  return loadExpectation("res/series/expected.txt").count;
}

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
