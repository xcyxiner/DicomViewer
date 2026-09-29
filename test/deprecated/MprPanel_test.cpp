#include <QApplication>
#include <QByteArray>
#include <QElapsedTimer>
#include <QStringList>
#include <QThread>
#include <QtGlobal>
#include <fstream>
#include <functional>
#include <map>
#include <string>

#include "presentation/view/gui/MprPanel.h"

#include <gtest/gtest.h>

#include "TestCommon.h"
#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/IImageRenderer.h"
#include "infrastructure/task/QtTaskQueue.h"
#include "presentation/view/gui/GUICenter.h"
#include "presentation/view/window/GUIWindow.h"
#include "presentation/viewmodels/SeriesViewModel.h"

namespace
{

// 2D 侧记录型渲染器：本单元不关注 2D 行为，仅满足 ViewModel 构造。
class RecordingImageRenderer : public IImageRenderer
{
public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow>) override {}

  void render(const IFrameCache::FramePtr&, const DisplaySettings&) override {}

  void updateWindowLevel(double, double) override {}

  void reset() override {}

  void fitToWindow() override {}
};

// MPR 假渲染器（逻辑级断言，不建 GL 上下文）：记录三平面注入、
// setVolume、窗宽窗位与渲染调用。
class FakeMprRenderer : public IMprRenderer
{
public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window,
                       int plane) override
  {
    ++setRenderTargetCount;
    if (plane >= 0 && plane < 3) {
      boundPlanes[plane] = (window != nullptr);
    }
  }

  void setVolume(vtkSmartPointer<vtkImageData> volume) override
  {
    ++setVolumeCount;
    lastVolume = volume;
  }

  void updateWindowLevel(double windowWidth, double windowCenter) override
  {
    ++windowLevelCount;
    lastWindowWidth = windowWidth;
    lastWindowCenter = windowCenter;
  }

  void reset() override { ++resetCount; }

  void render() override { ++renderCount; }

  int setRenderTargetCount = 0;
  int setVolumeCount = 0;
  int windowLevelCount = 0;
  int resetCount = 0;
  int renderCount = 0;
  bool boundPlanes[3] = {false, false, false};
  vtkSmartPointer<vtkImageData> lastVolume;
  double lastWindowWidth = 0.0;
  double lastWindowCenter = 0.0;
};

// 组装一个带记录型 2D 渲染器的 ViewModel（仿 SeriesViewModel_test）。
struct MprFixture
{
  std::shared_ptr<RecordingImageRenderer> imageRenderer =
      std::make_shared<RecordingImageRenderer>();
  std::shared_ptr<VTKDicomAdaptReader> reader =
      std::make_shared<VTKDicomAdaptReader>();
  std::shared_ptr<QtTaskQueue> taskQueue = std::make_shared<QtTaskQueue>();
  std::shared_ptr<MemoryFrameCache> cache =
      std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel;
  std::shared_ptr<FakeMprRenderer> mprRenderer =
      std::make_shared<FakeMprRenderer>();

  MprFixture()
      : viewModel(imageRenderer, reader, taskQueue, cache)
  {
  }
};

// 场景 2 用：计数 open() 调用的 reader —— 直接证明 loadSeries 未被触发。
class CountingReader : public VTKDicomAdaptReader
{
public:
  void open(const std::string& filePath) override
  {
    ++openCount;
    VTKDicomAdaptReader::open(filePath);
  }

  int openCount = 0;
};

// 场景 8 用：计数 activate 的 MprPanel 子类。
class CountingMprPanel : public MprPanel
{
public:
  using MprPanel::MprPanel;

  void activate() override
  {
    ++activateCount;
    MprPanel::activate();
  }

  int activateCount = 0;
};

}  // namespace

// 场景 1：三个平面面板各自注册，viewRegistered 出现三个可区分名字
// （LayoutMenu 据此列出，供 2x2 下逐格分配）。
TEST(MprPanelRegistration, ViewRegisteredEmitsThreePlaneNames)
{
  MprFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  MprPanel axial(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  MprPanel coronal(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneCoronal);
  MprPanel sagittal(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneSagittal);

  QStringList registered;
  QObject::connect(&window,
                   &GUIWindow::viewRegistered,
                   &window,
                   [&registered](const QString& name)
                   { registered.append(name); });

  window.registerView(&center);
  window.registerView(&axial);
  window.registerView(&coronal);
  window.registerView(&sagittal);

  EXPECT_TRUE(registered.contains(QStringLiteral("2D Viewer")))
      << "实际注册: " << registered.join(",").toStdString();
  EXPECT_TRUE(registered.contains(QStringLiteral("MPR 轴位")))
      << "实际注册: " << registered.join(",").toStdString();
  EXPECT_TRUE(registered.contains(QStringLiteral("MPR 冠状")))
      << "实际注册: " << registered.join(",").toStdString();
  EXPECT_TRUE(registered.contains(QStringLiteral("MPR 矢状")))
      << "实际注册: " << registered.join(",").toStdString();
}

// 拆分：每个平面面板只注入自己平面的渲染目标，不越权注入其余平面
// （三个面板共享同一 renderer，各管各的子视口）。
TEST(MprPanelBehavior, EachPanelBindsOnlyItsOwnPlane)
{
  MprFixture fixture;
  MprPanel first(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  MprPanel second(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneCoronal);

  first.activate();

  EXPECT_TRUE(fixture.mprRenderer->boundPlanes[0]);
  EXPECT_FALSE(fixture.mprRenderer->boundPlanes[1])
      << "首个面板不应注入冠状面渲染目标";
  EXPECT_FALSE(fixture.mprRenderer->boundPlanes[2])
      << "首个面板不应注入矢状面渲染目标";

  second.activate();
  EXPECT_TRUE(fixture.mprRenderer->boundPlanes[1]);
}

// 分屏 + 2x2：三个平面面板占三个槽位、与 2D 组成四格（注册序即默认
// 槽序：2D → 轴 → 冠 → 矢）。
TEST(MprPanelLayout, SplitGridDistributesThreePlanesAcrossSlots)
{
  MprFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  MprPanel axial(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  MprPanel coronal(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneCoronal);
  MprPanel sagittal(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneSagittal);
  window.setViewMode(ViewMode::TwoDOnly);
  window.registerView(&center);
  window.registerView(&axial);
  window.registerView(&coronal);
  window.registerView(&sagittal);
  window.setViewMode(ViewMode::Split);
  window.setGridLayout(LayoutMode::Grid);

  EXPECT_EQ(window.panelInSlot(0), static_cast<IViewPanel*>(&center));
  EXPECT_EQ(window.panelInSlot(1), static_cast<IViewPanel*>(&axial));
  EXPECT_EQ(window.panelInSlot(2), static_cast<IViewPanel*>(&coronal));
  EXPECT_EQ(window.panelInSlot(3), static_cast<IViewPanel*>(&sagittal));
}

// 仅 MPR + 1x1：槽位由 ViewMode 决定，轴位面板（注册序首个 MPR）落
// 槽 0；1x1 只渲染槽 0，其余平面面板无槽可显示。
TEST(MprPanelLayout, MprOnlySingleShowsAxialInSlot0)
{
  MprFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  MprPanel axial(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  MprPanel coronal(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneCoronal);
  MprPanel sagittal(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneSagittal);
  window.setViewMode(ViewMode::TwoDOnly);
  window.registerView(&center);
  window.registerView(&axial);
  window.registerView(&coronal);
  window.registerView(&sagittal);
  window.setViewMode(ViewMode::MprOnly);

  EXPECT_EQ(window.panelInSlot(0), static_cast<IViewPanel*>(&axial));
}

// 场景 2：loadFiles 为 no-op（KTD7：避免 openFile 槽位广播造成双加载）。
TEST(MprPanelBehavior, LoadFilesIsNoOp)
{
  auto imageRenderer = std::make_shared<RecordingImageRenderer>();
  auto reader = std::make_shared<CountingReader>();
  auto taskQueue = std::make_shared<QtTaskQueue>();
  auto cache = std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel(imageRenderer, reader, taskQueue, cache);
  auto mprRenderer = std::make_shared<FakeMprRenderer>();
  MprPanel mpr(&viewModel, mprRenderer, IMprRenderer::kPlaneAxial);

  mpr.loadFiles({QStringLiteral("res/series")});

  // 若 loadFiles 转发到 ViewModel::loadSeries，后台任务必调用 reader.open
  QElapsedTimer timer;
  timer.start();
  while (reader->openCount == 0 && timer.elapsed() < 500) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(5);
  }
  EXPECT_EQ(reader->openCount, 0) << "loadFiles 触发了加载（应为 no-op）";
  EXPECT_EQ(mprRenderer->setVolumeCount, 0);
}

// 场景 3 + 4：惰性投递——未激活时 seriesLoaded 不取体不推送；
// activate 后补取，假 renderer 收到 setVolume 恰一次；初始 WW/WC
// 等于第 0 帧值（广播与激活无关）。
TEST(MprPanelBehavior, SeriesLoadedSetsVolumeOnceWithInitialLevel)
{
  auto exp = loadExpectation("res/series/expected.txt");
  MprFixture fixture;
  MprPanel mpr(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);

  int seriesLoadedCount = 0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &mpr,
                   [&seriesLoadedCount](const QString&)
                   { ++seriesLoadedCount; });

  fixture.viewModel.loadSeries("res/series");
  ASSERT_TRUE(waitFor([&]() { return seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";

  // 惰性：未被菜单切出过 → 不取体、不推送
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 0)
      << "未激活不应推送 setVolume";
  EXPECT_FALSE(mpr.hasVolume());

  // 初始窗宽窗位（KTD7：seriesLoaded 后立即广播，不依赖激活）
  EXPECT_GE(fixture.mprRenderer->windowLevelCount, 1);
  EXPECT_DOUBLE_EQ(fixture.mprRenderer->lastWindowWidth, exp.windowWidth);
  EXPECT_DOUBLE_EQ(fixture.mprRenderer->lastWindowCenter, exp.windowCenter);

  // 激活（菜单切出）→ 惰性补取：取体入口返回体槽位对象，
  // 假 renderer 收到的就是该对象，setVolume 恰一次
  mpr.activate();
  ASSERT_NE(fixture.viewModel.getVolume(), nullptr);
  EXPECT_EQ(fixture.mprRenderer->lastVolume, fixture.viewModel.getVolume());
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 1);
  EXPECT_TRUE(mpr.hasVolume());
}

// 场景 5：getVolume 返回空时进入占位态而非崩溃。
TEST(MprPanelBehavior, NullVolumeEntersPlaceholder)
{
  MprFixture fixture;
  MprPanel mpr(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);

  EXPECT_FALSE(mpr.hasVolume());
  // 直接触发信号（Qt 信号为 public）：模拟"seriesLoaded 到达但取体为空"
  emit fixture.viewModel.seriesLoaded(QStringLiteral("nonexistent.series"));

  EXPECT_FALSE(mpr.hasVolume());
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 0) << "空体不应推给 renderer";

  // 激活触发惰性补取：取体仍为空 → 保持占位
  mpr.activate();
  EXPECT_FALSE(mpr.hasVolume());
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 0);
}

// 场景 6：N=1 序列进入占位态——单层体冠/矢面退化为 1 像素条，
// 不应推给 renderer（取体入口本身仍返回非空，2D 抽层渲染不受影响）。
TEST(MprPanelBehavior, SingleFrameSeriesEntersPlaceholder)
{
  MprFixture fixture;
  MprPanel mpr(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);

  int seriesLoadedCount = 0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &mpr,
                   [&seriesLoadedCount](const QString&)
                   { ++seriesLoadedCount; });

  fixture.viewModel.loadSeries("res/CT_small.dcm");
  ASSERT_TRUE(waitFor([&]() { return seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";

  // 惰性：未激活不取体不推送
  EXPECT_FALSE(mpr.hasVolume());
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 0);

  // 激活后经 isMprUsable 门控：单层体 → 占位，不推给 renderer
  mpr.activate();
  EXPECT_FALSE(mpr.hasVolume()) << "单层体不应视为可用体";
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 0)
      << "单层体不应推给 renderer";
  ASSERT_NE(fixture.viewModel.getVolume(), nullptr)
      << "取体入口仍返回体（2D 路径共用）";
}

// 场景 7 顺序 A：先开序列（MPR 未激活）、后切 1x2 → 最终收到 setVolume。
TEST(MprPanelLayout, LoadThenLayoutReceivesVolume)
{
  auto exp = loadExpectation("res/series/expected.txt");
  MprFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  MprPanel mpr(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  // 显式仅 2D 注册（固化场景前提：MPR 注册时不占槽、不激活，不随
  // 默认视图模式漂移），再进分屏（MPR → 槽 1，Single 下不可见）
  window.setViewMode(ViewMode::TwoDOnly);
  window.registerView(&center);
  window.registerView(&mpr);
  window.setViewMode(ViewMode::Split);

  int seriesLoadedCount = 0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &mpr,
                   [&seriesLoadedCount](const QString&)
                   { ++seriesLoadedCount; });

  fixture.viewModel.loadSeries("res/series");
  ASSERT_TRUE(waitFor([&]() { return seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";
  EXPECT_EQ(fixture.mprRenderer->setRenderTargetCount, 0)
      << "Single 下不应激活";

  window.setGridLayout(LayoutMode::Horizontal);  // 切 1x2 → refreshLayout 激活

  // 单面板只注入自己平面的渲染目标（拆分后三平面由三面板各自注入，
  // 本场景只注册轴位面板）
  EXPECT_EQ(fixture.mprRenderer->setRenderTargetCount, 1)
      << "轴位平面渲染目标未注入";
  EXPECT_GE(fixture.mprRenderer->setVolumeCount, 1) << "未收到 setVolume";
  EXPECT_TRUE(mpr.hasVolume());
  EXPECT_DOUBLE_EQ(fixture.mprRenderer->lastWindowWidth, exp.windowWidth);
  EXPECT_DOUBLE_EQ(fixture.mprRenderer->lastWindowCenter, exp.windowCenter);
}

// 场景 7 顺序 B：先切 1x2（MPR 激活但无体）、后开序列 → 收到 setVolume。
TEST(MprPanelLayout, LayoutThenLoadReceivesVolume)
{
  MprFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  MprPanel mpr(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  // 同 7A：仅 2D 下注册 → 进分屏（MPR 槽 1，Single 下不可见）
  window.setViewMode(ViewMode::TwoDOnly);
  window.registerView(&center);
  window.registerView(&mpr);
  window.setViewMode(ViewMode::Split);

  window.setGridLayout(LayoutMode::Horizontal);  // 先布局：activate 无体
  EXPECT_EQ(fixture.mprRenderer->setRenderTargetCount, 1);
  EXPECT_EQ(fixture.mprRenderer->setVolumeCount, 0);

  int seriesLoadedCount = 0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &mpr,
                   [&seriesLoadedCount](const QString&)
                   { ++seriesLoadedCount; });

  fixture.viewModel.loadSeries("res/series");
  ASSERT_TRUE(waitFor([&]() { return seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";

  EXPECT_GE(fixture.mprRenderer->setVolumeCount, 1) << "未收到 setVolume";
  EXPECT_TRUE(mpr.hasVolume());
}

// 场景 8：分屏模式下已注册但 Single 布局不可见的 MprPanel，切 1x2 后
// refreshLayout 补调 activate 恰一次（KTD8）。
TEST(MprPanelLayout, LayoutSwitchActivatesMprExactlyOnce)
{
  MprFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  CountingMprPanel mpr(
      &fixture.viewModel, fixture.mprRenderer, IMprRenderer::kPlaneAxial);
  // 仅 2D 下注册 → 进分屏：MPR 得槽 1，Single 下仍不可见
  window.setViewMode(ViewMode::TwoDOnly);
  window.registerView(&center);
  window.registerView(&mpr);
  window.setViewMode(ViewMode::Split);

  EXPECT_EQ(mpr.activateCount, 0) << "Single 布局下 MPR 槽位不可见，不应激活";

  window.setGridLayout(LayoutMode::Horizontal);

  EXPECT_EQ(mpr.activateCount, 1) << "refreshLayout 应恰好补调 activate 一次";
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 控件测试无显示服务器：强制 offscreen 平台（须在 QApplication 构造前）
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
