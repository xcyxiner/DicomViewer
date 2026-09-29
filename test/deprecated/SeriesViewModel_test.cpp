#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <fstream>
#include <functional>
#include <memory>
#include <string>

#include "presentation/viewmodels/SeriesViewModel.h"

#include <gtest/gtest.h>

#include "TestCommon.h"
#include "domain/model/DisplaySettings.h"
#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/IImageRenderer.h"
#include "infrastructure/rendering/IMprRenderer.h"
#include "infrastructure/task/QtTaskQueue.h"
#include "presentation/view/gui/MprPanel.h"

namespace
{

// 不建 GL 上下文的记录型渲染器：断言收到的抽层层号与次数。
// 联动（U6）扩展：捕获 ViewModel 绑定的窗宽窗位回灌回调（模拟 2D
// 拖动）、记录程序化回写；m_echoWindowLevel 模拟"程序化回写又被
// 回灌同值"的最坏情况（KTD11 防回环验证）。
class RecordingRenderer : public IImageRenderer
{
public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow>) override {}

  void render(const IFrameCache::FramePtr& frame,
              const DisplaySettings&) override
  {
    ++renderCount;
    auto* image = std::get_if<vtkSmartPointer<vtkImageData>>(&frame);
    if (image && *image) {
      int extent[6];
      (*image)->GetExtent(extent);
      lastSliceZ = extent[4];
      lastSliceZMax = extent[5];
    } else {
      lastSliceZ = -1;
      lastSliceZMax = -1;
    }
  }

  void updateWindowLevel(double windowWidth, double windowCenter) override
  {
    ++windowLevelUpdateCount;
    lastWindowWidth = windowWidth;
    lastWindowCenter = windowCenter;
    // 最坏情况模拟：程序化回写立刻以同值回灌（真实实现由 renderer
    // 内部抑制 + ViewModel 值比较双保险拦截）
    if (m_echoWindowLevel && m_windowLevelCallback) {
      m_windowLevelCallback(windowWidth, windowCenter);
    }
  }

  void reset() override {}

  void fitToWindow() override { ++fitCount; }

  void setWindowLevelCallback(
      std::function<void(double, double)> callback) override
  {
    m_windowLevelCallback = std::move(callback);
  }

  int renderCount = 0;
  int fitCount = 0;
  int windowLevelUpdateCount = 0;
  int lastSliceZ = -1;
  int lastSliceZMax = -1;
  double lastWindowWidth = 0.0;
  double lastWindowCenter = 0.0;
  bool m_echoWindowLevel = false;
  std::function<void(double, double)> m_windowLevelCallback;
};

// 不建 GL 上下文的假 MPR 渲染器：记录 MprPanel 对 IMprRenderer 的
// 全部调用与面板挂接的两个回灌回调（联动用例的触发入口）。
class FakeMprRenderer : public IMprRenderer
{
public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow>, int) override {}

  void setVolume(vtkSmartPointer<vtkImageData> volume) override
  {
    ++setVolumeCount;
    lastVolume = volume;
  }

  void updateWindowLevel(double windowWidth, double windowCenter) override
  {
    ++updateCount;
    lastWindowWidth = windowWidth;
    lastWindowCenter = windowCenter;
  }

  void reset() override {}

  void render() override { ++renderCount; }

  void setSlicePosition(int plane, double position) override
  {
    ++slicePositionCount;
    lastPlane = plane;
    lastPosition = position;
  }

  void setWindowLevelCallback(
      std::function<void(double, double)> callback) override
  {
    windowLevelCallback = std::move(callback);
  }

  void setSliceIndexCallback(std::function<void(double)> callback) override
  {
    sliceIndexCallback = std::move(callback);
  }

  int setVolumeCount = 0;
  int updateCount = 0;
  int slicePositionCount = 0;
  int renderCount = 0;
  int lastPlane = -1;
  double lastPosition = -1.0;
  double lastWindowWidth = 0.0;
  double lastWindowCenter = 0.0;
  vtkSmartPointer<vtkImageData> lastVolume;
  std::function<void(double, double)> windowLevelCallback;
  std::function<void(double)> sliceIndexCallback;
};

// waitFor 事件泵与 fixtureFrameCount 见 TestCommon.h（共享脚手架）。

struct NavSignals
{
  int sliceChangedCount = 0;
  int imageChangedCount = 0;
  int lastSliceIndex = -1;
};

// 注意：信号记录器必须以引用方式传入（lambda 捕获 &recorded）。
// 按值返回 + 成员初始化的写法依赖 NRVO 是否对子对象目标生效，
// 编译器不保证，曾导致信号写入已析构临时对象、计数恒 0。
void observeNav(SeriesViewModel& viewModel, NavSignals& recorded)
{
  QObject::connect(&viewModel,
                   &SeriesViewModel::sliceChanged,
                   &viewModel,
                   [&recorded](int index)
                   {
                     ++recorded.sliceChangedCount;
                     recorded.lastSliceIndex = index;
                   });
  QObject::connect(&viewModel,
                   &SeriesViewModel::imageChanged,
                   &viewModel,
                   [&recorded]() { ++recorded.imageChangedCount; });
}

// 组装一个带记录型渲染器的 ViewModel。
struct NavFixture
{
  std::shared_ptr<RecordingRenderer> renderer =
      std::make_shared<RecordingRenderer>();
  std::shared_ptr<VTKDicomAdaptReader> reader =
      std::make_shared<VTKDicomAdaptReader>();
  std::shared_ptr<QtTaskQueue> taskQueue = std::make_shared<QtTaskQueue>();
  std::shared_ptr<MemoryFrameCache> cache =
      std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel;
  NavSignals recorded;

  NavFixture()
      : viewModel(renderer, reader, taskQueue, cache)
  {
    observeNav(viewModel, recorded);
  }

  // 模拟 GUICenter 的 imageChanged → render + fit 接线。
  void connectRenderOnReady()
  {
    QObject::connect(&viewModel,
                     &SeriesViewModel::imageChanged,
                     &viewModel,
                     [this]()
                     {
                       viewModel.render();
                       viewModel.fitToWindow();
                     });
  }

  bool loadAndWait(const std::string& path)
  {
    const int before = recorded.imageChangedCount;
    viewModel.loadSeries(path);
    return waitFor([&]() { return recorded.imageChangedCount > before; });
  }
};

}  // namespace

// 场景 4：未加载序列时翻层调用为安全空操作（不发信号、不崩溃）。
TEST(SeriesViewModelNavTest, NavigationBeforeLoadIsSafeNoop)
{
  NavFixture fixture;

  EXPECT_NO_THROW(fixture.viewModel.nextFrame());
  EXPECT_NO_THROW(fixture.viewModel.previousFrame());
  EXPECT_NO_THROW(fixture.viewModel.setCurrentIndex(3));

  EXPECT_EQ(fixture.recorded.sliceChangedCount, 0);
  EXPECT_EQ(fixture.recorded.imageChangedCount, 0);
  EXPECT_EQ(fixture.renderer->renderCount, 0);
}

// 场景 1：setCurrentIndex 越界夹取到 [0, N-1]；
// 边界处 next/prev 不改变索引、不再发信号。
TEST(SeriesViewModelNavTest, SetCurrentIndexClampsToBounds)
{
  NavFixture fixture;
  const int n = fixtureFrameCount();
  ASSERT_GT(n, 1);
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  // 负数 → 0：与初始 currentIndex 相同，按幂等规则不发信号
  fixture.viewModel.setCurrentIndex(-10);
  EXPECT_EQ(fixture.viewModel.currentIndex(), 0);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 0);

  // 越上界 → N-1，恰发一次 sliceChanged
  fixture.viewModel.setCurrentIndex(n + 5);
  EXPECT_EQ(fixture.viewModel.currentIndex(), n - 1);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 1);
  EXPECT_EQ(fixture.recorded.lastSliceIndex, n - 1);

  // 上边界 next：索引不变、无新信号
  fixture.viewModel.nextFrame();
  EXPECT_EQ(fixture.viewModel.currentIndex(), n - 1);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 1);

  // 离开上边界 -1，再落到 1、0
  fixture.viewModel.previousFrame();
  EXPECT_EQ(fixture.viewModel.currentIndex(), n - 2);
  fixture.viewModel.setCurrentIndex(1);
  EXPECT_EQ(fixture.viewModel.currentIndex(), 1);
  fixture.viewModel.previousFrame();
  EXPECT_EQ(fixture.viewModel.currentIndex(), 0);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 4);

  // 下边界 prev：不变、无新信号
  fixture.viewModel.previousFrame();
  EXPECT_EQ(fixture.viewModel.currentIndex(), 0);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 4);
}

// 场景 2：每次翻层恰好发出一次 sliceChanged，且携带新索引。
TEST(SeriesViewModelNavTest, SliceChangedEmitsExactlyOncePerChange)
{
  NavFixture fixture;
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  fixture.viewModel.setCurrentIndex(3);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 1);
  EXPECT_EQ(fixture.recorded.lastSliceIndex, 3);

  fixture.viewModel.setCurrentIndex(4);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 2);
  EXPECT_EQ(fixture.recorded.lastSliceIndex, 4);

  fixture.viewModel.nextFrame();
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 3);
  EXPECT_EQ(fixture.recorded.lastSliceIndex, 5);
}

// 场景 3：翻层不触发 imageChanged（相机与窗宽窗位保持语义由
// 信号分离保证）。
TEST(SeriesViewModelNavTest, SliceChangeDoesNotEmitImageChanged)
{
  NavFixture fixture;
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";
  const int afterLoad = fixture.recorded.imageChangedCount;
  ASSERT_EQ(afterLoad, 1);

  fixture.viewModel.setCurrentIndex(2);
  fixture.viewModel.nextFrame();
  fixture.viewModel.previousFrame();

  EXPECT_EQ(fixture.recorded.imageChangedCount, afterLoad);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 3);
}

// 场景 5（逻辑等价层）：重复 setCurrentIndex 到同值 —— 索引一致，
// 第二次为幂等空操作（不重复发 sliceChanged），滑条同步依赖此性质。
TEST(SeriesViewModelNavTest, RepeatedSetCurrentIndexIsIdempotent)
{
  NavFixture fixture;
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  fixture.viewModel.setCurrentIndex(5);
  EXPECT_EQ(fixture.viewModel.currentIndex(), 5);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 1);

  fixture.viewModel.setCurrentIndex(5);
  EXPECT_EQ(fixture.viewModel.currentIndex(), 5);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 1);
}

// 渲染接入：加载后 render 收到第 0 层体抽层；翻层后恰好多渲染一次
// 且抽层 z 与新索引一致（抽层实现唯一、几何来自体快照，KTD1/KTD6）。
TEST(SeriesViewModelNavTest, RenderReceivesVolumeSliceAtCurrentIndex)
{
  NavFixture fixture;
  fixture.connectRenderOnReady();
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  ASSERT_GE(fixture.renderer->renderCount, 1);
  EXPECT_EQ(fixture.renderer->lastSliceZ, 0);
  EXPECT_EQ(fixture.renderer->lastSliceZMax, 0);
  EXPECT_EQ(fixture.renderer->fitCount, 1);  // 仅加载时 fit 一次

  const int before = fixture.renderer->renderCount;
  fixture.viewModel.setCurrentIndex(4);
  EXPECT_EQ(fixture.renderer->renderCount, before + 1);
  EXPECT_EQ(fixture.renderer->lastSliceZ, 4);
  EXPECT_EQ(fixture.renderer->lastSliceZMax, 4);
  EXPECT_EQ(fixture.renderer->fitCount, 1);  // 翻层不 fit
}

// N=1 序列：翻层为无副作用空操作（索引保持 0、无 sliceChanged）。
TEST(SeriesViewModelNavTest, SingleFrameSeriesNavigationIsNoop)
{
  NavFixture fixture;
  ASSERT_TRUE(fixture.loadAndWait("res/CT_small.dcm")) << "单帧加载失败";

  fixture.viewModel.nextFrame();
  fixture.viewModel.previousFrame();
  fixture.viewModel.setCurrentIndex(7);

  EXPECT_EQ(fixture.viewModel.currentIndex(), 0);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, 0);
}

// ===== U6：2D 与 MPR 双向联动（R7/R8，KTD11） =====

// 场景 1：setWindowLevel → windowLevelChanged 携带相同 ww/wc 恰发出
// 一次；同值重复调用为幂等空操作（KTD11 值比较防环的第一道闸）。
TEST(SeriesViewModelLinkageTest, SetWindowLevelEmitsOnceWithSameValues)
{
  NavFixture fixture;
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  int emitted = 0;
  double gotWidth = 0.0;
  double gotCenter = 0.0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::windowLevelChanged,
                   &fixture.viewModel,
                   [&](double windowWidth, double windowCenter)
                   {
                     ++emitted;
                     gotWidth = windowWidth;
                     gotCenter = windowCenter;
                   });

  fixture.viewModel.setWindowLevel(80.0, 40.0);
  EXPECT_EQ(emitted, 1);
  EXPECT_DOUBLE_EQ(gotWidth, 80.0);
  EXPECT_DOUBLE_EQ(gotCenter, 40.0);
  // 2D renderer 程序化回写恰一次
  EXPECT_EQ(fixture.renderer->windowLevelUpdateCount, 1);

  // 同值重复调用 → 幂等，不重复广播、不重复回写
  fixture.viewModel.setWindowLevel(80.0, 40.0);
  EXPECT_EQ(emitted, 1);
  EXPECT_EQ(fixture.renderer->windowLevelUpdateCount, 1);
}

// 2D 拖动（renderer 回灌回调）→ 与 setWindowLevel 同一广播链 →
// MPR renderer 经面板收到一致值（R7 的 2D 侧链路）。
TEST(SeriesViewModelLinkageTest, TwoDInteractionBroadcastsToMpr)
{
  NavFixture fixture;
  auto mpr = std::make_shared<FakeMprRenderer>();
  MprPanel panel(&fixture.viewModel, mpr, IMprRenderer::kPlaneAxial);
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";
  ASSERT_TRUE(fixture.renderer->m_windowLevelCallback)
      << "ViewModel 未绑定 2D 回灌回调";
  const int mprUpdatesBefore = mpr->updateCount;

  int emitted = 0;
  double gotWidth = 0.0;
  double gotCenter = 0.0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::windowLevelChanged,
                   &fixture.viewModel,
                   [&](double windowWidth, double windowCenter)
                   {
                     ++emitted;
                     gotWidth = windowWidth;
                     gotCenter = windowCenter;
                   });

  fixture.renderer->m_windowLevelCallback(120.0, 60.0);  // 模拟 2D 拖动

  EXPECT_EQ(emitted, 1);
  EXPECT_DOUBLE_EQ(gotWidth, 120.0);
  EXPECT_DOUBLE_EQ(gotCenter, 60.0);
  EXPECT_EQ(mpr->updateCount, mprUpdatesBefore + 1);
  EXPECT_DOUBLE_EQ(mpr->lastWindowWidth, 120.0);
  EXPECT_DOUBLE_EQ(mpr->lastWindowCenter, 60.0);
}

// 场景 2：发出 sliceChanged 后 MprPanel 的切面状态随之更新
// （转发到 IMprRenderer::setSlicePosition，轴位 pane 直映射，M3）。
TEST(SeriesViewModelLinkageTest, SliceChangedUpdatesMprPlane)
{
  NavFixture fixture;
  auto mpr = std::make_shared<FakeMprRenderer>();
  MprPanel panel(&fixture.viewModel, mpr, IMprRenderer::kPlaneAxial);
  // 惰性投递：未激活的面板不收 seriesLoaded 推送，先激活再加载
  panel.activate();
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  // seriesLoaded 后面板把轴位切面补推到当前层（第 0 层）
  EXPECT_EQ(mpr->slicePositionCount, 1);
  EXPECT_EQ(mpr->lastPlane, 0);
  EXPECT_DOUBLE_EQ(mpr->lastPosition, 0.0);

  fixture.viewModel.setCurrentIndex(4);
  EXPECT_EQ(mpr->slicePositionCount, 2);
  EXPECT_EQ(mpr->lastPlane, 0);
  EXPECT_DOUBLE_EQ(mpr->lastPosition, 4.0);

  fixture.viewModel.nextFrame();
  EXPECT_EQ(mpr->slicePositionCount, 3);
  EXPECT_DOUBLE_EQ(mpr->lastPosition, 5.0);
}

// 场景 3 + 场景 6（反向链路）：MPR 切面回调携带越界坐标（含小数）
// → 2D 索引夹取到 [0, N-1]，sliceChanged 恰各发出一次。
TEST(SeriesViewModelLinkageTest, MprSliceCallbackClampsAndEmitsOnce)
{
  NavFixture fixture;
  auto mpr = std::make_shared<FakeMprRenderer>();
  MprPanel panel(&fixture.viewModel, mpr, IMprRenderer::kPlaneAxial);
  ASSERT_TRUE(mpr->sliceIndexCallback) << "面板未挂接切面回灌回调";
  const int n = fixtureFrameCount();
  ASSERT_GT(n, 1);
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  const int before = fixture.recorded.sliceChangedCount;

  mpr->sliceIndexCallback(999.0);  // 越上界 → 夹取 N-1
  EXPECT_EQ(fixture.viewModel.currentIndex(), n - 1);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, before + 1);

  mpr->sliceIndexCallback(-7.5);  // 越下界（小数取整后仍越界）→ 0
  EXPECT_EQ(fixture.viewModel.currentIndex(), 0);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, before + 2);

  // 反向越界值与 setCurrentIndex 直调等价：再次同值回调为幂等空操作
  mpr->sliceIndexCallback(-7.5);
  EXPECT_EQ(fixture.recorded.sliceChangedCount, before + 2);
}

// 场景 4：加载新序列后联动状态重置为新序列的第 0 层与初始 WW/WC。
TEST(SeriesViewModelLinkageTest, ReloadResetsLinkageState)
{
  NavFixture fixture;
  auto mpr = std::make_shared<FakeMprRenderer>();
  MprPanel panel(&fixture.viewModel, mpr, IMprRenderer::kPlaneAxial);
  // 惰性投递：未激活的面板不收 seriesLoaded 推送，先激活再加载
  panel.activate();
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";

  fixture.viewModel.setWindowLevel(999.0, 111.0);
  fixture.viewModel.setCurrentIndex(5);
  ASSERT_EQ(fixture.viewModel.currentIndex(), 5);

  int wlEmitted = 0;
  double gotWidth = 0.0;
  double gotCenter = 0.0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::windowLevelChanged,
                   &fixture.viewModel,
                   [&](double windowWidth, double windowCenter)
                   {
                     ++wlEmitted;
                     gotWidth = windowWidth;
                     gotCenter = windowCenter;
                   });

  ASSERT_TRUE(fixture.loadAndWait("res/CT_small.dcm")) << "单帧加载失败";

  // 新序列初始 WW/WC（CT_small 无 WW/WC 标签 → DisplaySettings 默认值）
  EXPECT_EQ(wlEmitted, 1);
  EXPECT_DOUBLE_EQ(gotWidth, 400.0);
  EXPECT_DOUBLE_EQ(gotCenter, 40.0);
  // 联动层位置重置到新序列第 0 层，MPR 轴位切面同步
  EXPECT_EQ(fixture.viewModel.currentIndex(), 0);
  EXPECT_EQ(mpr->lastPlane, 0);
  EXPECT_DOUBLE_EQ(mpr->lastPosition, 0.0);
}

// 场景 5：MPR 侧拖动改 WW/WC → windowLevelChanged 恰发出一次；
// ViewModel 回写两 renderer，2D 侧最坏情况（回写同值又回灌）被值
// 比较拦截，不再产生新广播（KTD11 防回环，集成）。
TEST(SeriesViewModelLinkageTest, MprInteractionBroadcastsExactlyOnce)
{
  NavFixture fixture;
  fixture.renderer->m_echoWindowLevel = true;  // 2D 回写同值回灌
  auto mpr = std::make_shared<FakeMprRenderer>();
  MprPanel panel(&fixture.viewModel, mpr, IMprRenderer::kPlaneAxial);
  ASSERT_TRUE(fixture.loadAndWait("res/series")) << "夹具加载失败";
  ASSERT_TRUE(mpr->windowLevelCallback) << "面板未挂接窗宽窗位回灌回调";

  const int dRendererUpdates = fixture.renderer->windowLevelUpdateCount;
  const int mprUpdates = mpr->updateCount;
  int emitted = 0;
  double gotWidth = 0.0;
  double gotCenter = 0.0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::windowLevelChanged,
                   &fixture.viewModel,
                   [&](double windowWidth, double windowCenter)
                   {
                     ++emitted;
                     gotWidth = windowWidth;
                     gotCenter = windowCenter;
                   });

  mpr->windowLevelCallback(1500.0, -600.0);  // 模拟 MPR 侧拖动

  EXPECT_EQ(emitted, 1);
  EXPECT_DOUBLE_EQ(gotWidth, 1500.0);
  EXPECT_DOUBLE_EQ(gotCenter, -600.0);
  // 2D renderer 回写恰一次（echo 回灌被幂等拦截 → 计数不再增长）
  EXPECT_EQ(fixture.renderer->windowLevelUpdateCount, dRendererUpdates + 1);
  // 面板把广播应用到 MPR renderer 恰一次
  EXPECT_EQ(mpr->updateCount, mprUpdates + 1);
  EXPECT_DOUBLE_EQ(mpr->lastWindowWidth, 1500.0);
  EXPECT_DOUBLE_EQ(mpr->lastWindowCenter, -600.0);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 联动用例构造 MprPanel（QWidget）：无显示环境时回落 offscreen，
  // 与 MprPanel_test 的 QT_QPA_PLATFORM=offscreen 门禁一致
  if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")
      && !qEnvironmentVariableIsSet("DISPLAY"))
  {
    qputenv("QT_QPA_PLATFORM", "offscreen");
  }
  // 跨线程信号回投主线程需要事件循环
  QApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
