#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "application/LoadSeriesUseCase.h"

#include <gtest/gtest.h>

#include "TestCommon.h"
#include "domain/model/DisplaySettings.h"
#include "domain/model/StackDisplaySet.h"
#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/IImageRenderer.h"
#include "infrastructure/task/QtTaskQueue.h"
#include "presentation/viewmodels/SeriesViewModel.h"

namespace
{

// 不建 GL 上下文的假渲染器：只记录调用（逻辑级断言）。
class FakeRenderer : public IImageRenderer
{
public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow>) override {}

  void render(const IFrameCache::FramePtr&, const DisplaySettings&) override
  {
    ++renderCount;
  }

  void updateWindowLevel(double windowWidth, double windowCenter) override
  {
    ++windowLevelCount;
    lastWindowWidth = windowWidth;
    lastWindowCenter = windowCenter;
  }

  void reset() override {}

  void fitToWindow() override { ++fitCount; }

  int renderCount = 0;
  int windowLevelCount = 0;
  int fitCount = 0;
  double lastWindowWidth = 0.0;
  double lastWindowCenter = 0.0;
};

// ViewModel 信号记录器（connect + lambda 计数，不引入 QtTest 依赖）。
struct ViewModelSignals
{
  int seriesLoadedCount = 0;
  int loadFailedCount = 0;
  int imageChangedCount = 0;
  int windowLevelChangedCount = 0;
  std::string loadedSeriesKey;
  std::string failedPath;
  std::string failedReason;
  double lastWindowWidth = 0.0;
  double lastWindowCenter = 0.0;
};

// 注意：记录器必须以引用方式出参（lambda 捕获 &recorded）。按值返回
// 依赖 NRVO 对子对象目标生效，标准不保证——曾导致信号写入已析构
// 临时对象、计数恒 0（同 observeNav 的教训）。
void observe(SeriesViewModel& viewModel, ViewModelSignals& recorded)
{
  QObject::connect(&viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &viewModel,
                   [&recorded](const QString& key)
                   {
                     ++recorded.seriesLoadedCount;
                     recorded.loadedSeriesKey = key.toStdString();
                   });
  QObject::connect(&viewModel,
                   &SeriesViewModel::loadFailed,
                   &viewModel,
                   [&recorded](const QString& path, const QString& reason)
                   {
                     ++recorded.loadFailedCount;
                     recorded.failedPath = path.toStdString();
                     recorded.failedReason = reason.toStdString();
                   });
  QObject::connect(&viewModel,
                   &SeriesViewModel::imageChanged,
                   &viewModel,
                   [&recorded]() { ++recorded.imageChangedCount; });
  QObject::connect(&viewModel,
                   &SeriesViewModel::windowLevelChanged,
                   &viewModel,
                   [&recorded](double ww, double wc)
                   {
                     ++recorded.windowLevelChangedCount;
                     recorded.lastWindowWidth = ww;
                     recorded.lastWindowCenter = wc;
                   });
}

}  // namespace

// ---- 用例层（loadSeriesAsync 直接 .get()，6 场景）----

// 场景 1：夹具目录帧数 = N、UID 逐层对应、seriesKey 非空。
TEST(LoadSeriesUseCaseTest, DirectoryLoadProducesFullDisplaySet)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  QtTaskQueue taskQueue;
  LoadSeriesUseCase useCase(reader, taskQueue);

  auto result = useCase.loadSeriesAsync("res/series").get();

  EXPECT_TRUE(result.error.empty()) << result.error;
  ASSERT_NE(result.displaySet, nullptr);
  EXPECT_EQ(result.displaySet->getFrameUids(), exp.uids);
  EXPECT_EQ(result.displaySet->getSeriesKey(), exp.seriesUid);
  EXPECT_EQ(result.displaySet->getCurrentIndex(), 0);
  ASSERT_NE(result.volume, nullptr);
  int extent[6];
  result.volume->GetExtent(extent);
  EXPECT_EQ(extent[5] - extent[4] + 1, exp.count);
}

// 场景 2a：初始 DisplaySettings = 第 0 帧 WW/WC（夹具 400/40）。
TEST(LoadSeriesUseCaseTest, InitialDisplaySettingsFromFirstFrame)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  QtTaskQueue taskQueue;
  LoadSeriesUseCase useCase(reader, taskQueue);

  auto result = useCase.loadSeriesAsync("res/series").get();

  ASSERT_NE(result.displaySet, nullptr);
  const auto& settings = result.displaySet->getDisplaySettings();
  EXPECT_DOUBLE_EQ(settings.getWindowWidth(), exp.windowWidth);
  EXPECT_DOUBLE_EQ(settings.getWindowCenter(), exp.windowCenter);
}

// 场景 2b：第 0 帧无 WW/WC（CT_small）时使用 DisplaySettings 默认值。
TEST(LoadSeriesUseCaseTest, MissingWindowLevelUsesDefaults)
{
  VTKDicomAdaptReader reader;
  QtTaskQueue taskQueue;
  LoadSeriesUseCase useCase(reader, taskQueue);

  auto result = useCase.loadSeriesAsync("res/CT_small.dcm").get();

  ASSERT_NE(result.displaySet, nullptr);
  const DisplaySettings defaults;
  EXPECT_DOUBLE_EQ(result.displaySet->getDisplaySettings().getWindowWidth(),
                   defaults.getWindowWidth());
  EXPECT_DOUBLE_EQ(result.displaySet->getDisplaySettings().getWindowCenter(),
                   defaults.getWindowCenter());
}

// 场景 4（用例层）：不存在路径以失败结果完成，不产生未捕获异常。
TEST(LoadSeriesUseCaseTest, InvalidPathFailsWithResultError)
{
  VTKDicomAdaptReader reader;
  QtTaskQueue taskQueue;
  LoadSeriesUseCase useCase(reader, taskQueue);

  LoadSeriesResult result;
  EXPECT_NO_THROW(result =
                      useCase.loadSeriesAsync("res/does_not_exist.dcm").get());
  EXPECT_FALSE(result.error.empty());
  EXPECT_EQ(result.displaySet, nullptr);
  EXPECT_EQ(result.volume, nullptr);
}

// ---- ViewModel 集成（信号通道 + 同一替换点，场景 3/4/5/6）----

// 场景 3：成功加载后体已入缓存，getVolume 取回体；
// 且 seriesLoaded 后立即广播一次初始 windowLevelChanged（KTD7）。
TEST(SeriesViewModelTest, SuccessCommitsVolumeAndSignals)
{
  auto exp = loadExpectation("res/series/expected.txt");
  auto renderer = std::make_shared<FakeRenderer>();
  auto reader = std::make_shared<VTKDicomAdaptReader>();
  auto taskQueue = std::make_shared<QtTaskQueue>();
  auto cache = std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel(renderer, reader, taskQueue, cache);
  ViewModelSignals recorded;
  observe(viewModel, recorded);

  viewModel.loadSeries("res/series");
  ASSERT_TRUE(waitFor([&]() { return recorded.seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";

  EXPECT_EQ(recorded.loadedSeriesKey, exp.seriesUid);
  EXPECT_EQ(recorded.loadFailedCount, 0);
  // 体已按 seriesKey 入槽位，取体入口返回同一对象
  ASSERT_NE(viewModel.getVolume(), nullptr);
  EXPECT_EQ(viewModel.getVolume(), cache->getVolume(exp.seriesUid));
  // 初始窗宽窗位通道：紧随 seriesLoaded 广播一次
  EXPECT_EQ(recorded.windowLevelChangedCount, 1);
  EXPECT_DOUBLE_EQ(recorded.lastWindowWidth, exp.windowWidth);
  EXPECT_DOUBLE_EQ(recorded.lastWindowCenter, exp.windowCenter);
  // 序列就绪渲染信号
  EXPECT_EQ(recorded.imageChangedCount, 1);
}

// 场景 4（ViewModel 层）：打开不存在路径 → loadFailed，
// seriesLoaded 不发出，不产生未捕获异常。
TEST(SeriesViewModelTest, LoadFailedEmitsSignalWithoutCrash)
{
  auto renderer = std::make_shared<FakeRenderer>();
  auto reader = std::make_shared<VTKDicomAdaptReader>();
  auto taskQueue = std::make_shared<QtTaskQueue>();
  auto cache = std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel(renderer, reader, taskQueue, cache);
  ViewModelSignals recorded;
  observe(viewModel, recorded);

  viewModel.loadSeries("res/does_not_exist.dcm");
  ASSERT_TRUE(waitFor([&]() { return recorded.loadFailedCount >= 1; }))
      << "loadFailed 未发出";
  EXPECT_EQ(recorded.seriesLoadedCount, 0);
  EXPECT_FALSE(recorded.failedPath.empty());
  EXPECT_FALSE(recorded.failedReason.empty());
  EXPECT_EQ(viewModel.getVolume(), nullptr);
}

// 场景 5：连续打开两个序列，第二次成功后体槽位只保留新体
// （单槽替换：旧 seriesKey 不可再取回）。
TEST(SeriesViewModelTest, SequentialLoadsReplaceVolumeSlot)
{
  auto exp = loadExpectation("res/series/expected.txt");
  auto renderer = std::make_shared<FakeRenderer>();
  auto reader = std::make_shared<VTKDicomAdaptReader>();
  auto taskQueue = std::make_shared<QtTaskQueue>();
  auto cache = std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel(renderer, reader, taskQueue, cache);
  ViewModelSignals recorded;
  observe(viewModel, recorded);

  viewModel.loadSeries("res/series");
  ASSERT_TRUE(waitFor([&]() { return recorded.seriesLoadedCount >= 1; }))
      << "第一次加载失败";

  viewModel.loadSeries("res/CT_small.dcm");
  ASSERT_TRUE(waitFor([&]() { return recorded.seriesLoadedCount >= 2; }))
      << "第二次加载失败";

  const std::string secondKey = recorded.loadedSeriesKey;
  EXPECT_NE(secondKey, exp.seriesUid);
  ASSERT_NE(cache->getVolume(secondKey), nullptr);
  // 单槽语义：旧体不可再取回
  EXPECT_EQ(cache->getVolume(exp.seriesUid), nullptr);
}

// 场景 6：先成功再失败 —— loadFailed 发出，StackDisplaySet 与
// 体槽位仍指向旧序列（已显示图像保留，R9 后半句）。
TEST(SeriesViewModelTest, FailedLoadKeepsPreviousState)
{
  auto exp = loadExpectation("res/series/expected.txt");
  auto renderer = std::make_shared<FakeRenderer>();
  auto reader = std::make_shared<VTKDicomAdaptReader>();
  auto taskQueue = std::make_shared<QtTaskQueue>();
  auto cache = std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel(renderer, reader, taskQueue, cache);
  ViewModelSignals recorded;
  observe(viewModel, recorded);

  viewModel.loadSeries("res/series");
  ASSERT_TRUE(waitFor([&]() { return recorded.seriesLoadedCount >= 1; }))
      << "第一次加载失败";
  ASSERT_NE(viewModel.getVolume(), nullptr);

  viewModel.loadSeries("res/does_not_exist.dcm");
  ASSERT_TRUE(waitFor([&]() { return recorded.loadFailedCount >= 1; }))
      << "loadFailed 未发出";

  // 旧状态原样保留：显示集序列键、体槽位、就绪信号均不因失败改变
  EXPECT_EQ(recorded.seriesLoadedCount, 1);
  EXPECT_EQ(recorded.imageChangedCount, 1);
  EXPECT_EQ(viewModel.getVolume(), cache->getVolume(exp.seriesUid));
  ASSERT_NE(viewModel.getVolume(), nullptr);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 跨线程信号回投主线程需要事件循环
  QCoreApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
