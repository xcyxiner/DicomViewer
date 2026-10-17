#include <QApplication>
#include <QByteArray>
#include <QElapsedTimer>
#include <QStringList>
#include <QThread>

#include "presentation/view/gui/VolumePanel.h"

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

// 3D 假渲染器（逻辑级断言，不建 GL 上下文）：记录渲染目标注入、
// setVolume、窗宽窗位与渲染调用。
class FakeVolumeRenderer : public IVolumeRenderer
{
public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window) override
  {
    ++setRenderTargetCount;
    bound = (window != nullptr);
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
  bool bound = false;
  vtkSmartPointer<vtkImageData> lastVolume;
  double lastWindowWidth = 0.0;
  double lastWindowCenter = 0.0;
};

// 组装一个带记录型 2D 渲染器的 ViewModel（仿 MprPanel_test 夹具）。
struct VolumeFixture
{
  std::shared_ptr<RecordingImageRenderer> imageRenderer =
      std::make_shared<RecordingImageRenderer>();
  std::shared_ptr<VTKDicomAdaptReader> reader =
      std::make_shared<VTKDicomAdaptReader>();
  std::shared_ptr<QtTaskQueue> taskQueue = std::make_shared<QtTaskQueue>();
  std::shared_ptr<MemoryFrameCache> cache =
      std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel;
  std::shared_ptr<FakeVolumeRenderer> volumeRenderer =
      std::make_shared<FakeVolumeRenderer>();

  VolumeFixture()
      : viewModel(imageRenderer, reader, taskQueue, cache)
  {
  }
};

// 计数 open() 调用的 reader —— 直接证明 loadFiles 未被触发。
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

// 计数 activate 的 VolumePanel 子类。
class CountingVolumePanel : public VolumePanel
{
public:
  using VolumePanel::VolumePanel;

  void activate() override
  {
    ++activateCount;
    VolumePanel::activate();
  }

  int activateCount = 0;
};

// 逻辑级 MPR 假面板：镜像 main.cpp 的注册集（2D + 轴/冠/矢 + 3D），
// 使 3D 按注册序落槽 3。
class FakeMprPanel : public IViewPanel
{
public:
  explicit FakeMprPanel(const QString& name)
      : m_name(name)
  {
  }

  QString viewName() const override { return m_name; }

  ViewRole viewRole() const override { return ViewRole::Mpr; }

private:
  QString m_name;
};

}  // namespace

// 注册名唯一 + 角色为 Volume（LayoutMenu/ViewMode 据此列出与分配）
TEST(VolumePanelRegistration, ViewRegisteredEmitsUniqueName)
{
  VolumeFixture fixture;
  GUIWindow window;
  VolumePanel volume(&fixture.viewModel, fixture.volumeRenderer);

  EXPECT_EQ(volume.viewName(), QStringLiteral("3D 体绘制"));
  EXPECT_EQ(volume.viewRole(), ViewRole::Volume);

  QStringList registered;
  QObject::connect(&window,
                   &GUIWindow::viewRegistered,
                   &window,
                   [&registered](const QString& name)
                   { registered.append(name); });
  window.registerView(&volume);

  EXPECT_TRUE(registered.contains(QStringLiteral("3D 体绘制")))
      << "实际注册: " << registered.join(",").toStdString();
}

// 空体占位：seriesLoaded 到达但取体为空 → 不推 renderer，激活补取后
// 仍占位、零推送。
TEST(VolumePanelBehavior, NullVolumeEntersPlaceholder)
{
  VolumeFixture fixture;
  VolumePanel volume(&fixture.viewModel, fixture.volumeRenderer);

  EXPECT_FALSE(volume.hasVolume());
  // 直接触发信号（Qt 信号为 public）：模拟"seriesLoaded 到达但取体为空"
  emit fixture.viewModel.seriesLoaded(QStringLiteral("nonexistent.series"));

  EXPECT_FALSE(volume.hasVolume());
  EXPECT_EQ(fixture.volumeRenderer->setVolumeCount, 0) << "空体不应推送";

  // 激活触发惰性补取：取体仍为空 → 保持占位
  volume.activate();
  EXPECT_FALSE(volume.hasVolume());
  EXPECT_EQ(fixture.volumeRenderer->setVolumeCount, 0);
}

// N=1 序列占位：单层体无法体绘制，经 isVolumeUsable 门控拦截
// （取体入口本身仍返回非空，2D 抽层渲染不受影响）。
TEST(VolumePanelBehavior, SingleFrameSeriesEntersPlaceholder)
{
  VolumeFixture fixture;
  VolumePanel volume(&fixture.viewModel, fixture.volumeRenderer);

  int seriesLoadedCount = 0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &volume,
                   [&seriesLoadedCount](const QString&)
                   { ++seriesLoadedCount; });

  fixture.viewModel.loadSeries("res/CT_small.dcm");
  ASSERT_TRUE(waitFor([&]() { return seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";

  // 惰性：未激活不取体不推送
  EXPECT_FALSE(volume.hasVolume());
  EXPECT_EQ(fixture.volumeRenderer->setVolumeCount, 0);

  // 激活后经 isVolumeUsable 门控：单层体 → 占位，不推给 renderer
  volume.activate();
  EXPECT_FALSE(volume.hasVolume()) << "单层体不应视为可用体";
  EXPECT_EQ(fixture.volumeRenderer->setVolumeCount, 0)
      << "单层体不应推给 renderer";
  ASSERT_NE(fixture.viewModel.getVolume(), nullptr)
      << "取体入口仍返回体（2D 路径共用）";
}

// loadFiles 为 no-op（KTD7：避免 openFile 槽位广播造成双加载）
TEST(VolumePanelBehavior, LoadFilesIsNoOp)
{
  auto imageRenderer = std::make_shared<RecordingImageRenderer>();
  auto reader = std::make_shared<CountingReader>();
  auto taskQueue = std::make_shared<QtTaskQueue>();
  auto cache = std::make_shared<MemoryFrameCache>();
  SeriesViewModel viewModel(imageRenderer, reader, taskQueue, cache);
  auto volumeRenderer = std::make_shared<FakeVolumeRenderer>();
  VolumePanel volume(&viewModel, volumeRenderer);

  volume.loadFiles({QStringLiteral("res/CT_small.dcm")});

  // 若 loadFiles 转发到 ViewModel::loadSeries，后台任务必调用 reader.open
  QElapsedTimer timer;
  timer.start();
  while (reader->openCount == 0 && timer.elapsed() < 500) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(5);
  }
  EXPECT_EQ(reader->openCount, 0) << "loadFiles 触发了加载（应为 no-op）";
  EXPECT_EQ(volumeRenderer->setVolumeCount, 0);
}

// 窗宽窗位广播链（KTD11）：加载后的初始广播即达面板并写 renderer；
// 菜单 setWindowLevel 走同一广播链；activate 补推含最新 WW/WC。
TEST(VolumePanelBehavior, WindowLevelBroadcastForwards)
{
  VolumeFixture fixture;
  VolumePanel volume(&fixture.viewModel, fixture.volumeRenderer);

  int seriesLoadedCount = 0;
  QObject::connect(&fixture.viewModel,
                   &SeriesViewModel::seriesLoaded,
                   &volume,
                   [&seriesLoadedCount](const QString&)
                   { ++seriesLoadedCount; });

  fixture.viewModel.loadSeries("res/CT_small.dcm");
  ASSERT_TRUE(waitFor([&]() { return seriesLoadedCount >= 1; }))
      << "seriesLoaded 未发出";
  // 加载成功后 ViewModel 立即广播初始 WW/WC（面板本地缓存 + 写
  // renderer，无论是否激活）
  EXPECT_GE(fixture.volumeRenderer->windowLevelCount, 1);

  // 激活补推：渲染目标注入 + 缓存的 WW/WC 重放（幂等，KTD8）
  volume.activate();
  EXPECT_EQ(fixture.volumeRenderer->setRenderTargetCount, 1);
  EXPECT_GE(fixture.volumeRenderer->windowLevelCount, 2)
      << "activate 应补推 WW/WC";

  // 菜单/交互路径：setWindowLevel → 广播链 → 面板写 renderer
  const double expectedWW = 100.0;
  const double expectedWC = 50.0;
  const int before = fixture.volumeRenderer->windowLevelCount;
  fixture.viewModel.setWindowLevel(expectedWW, expectedWC);
  EXPECT_GT(fixture.volumeRenderer->windowLevelCount, before);
  EXPECT_DOUBLE_EQ(fixture.volumeRenderer->lastWindowWidth, expectedWW);
  EXPECT_DOUBLE_EQ(fixture.volumeRenderer->lastWindowCenter, expectedWC);
}

// 仅 2D 下注册不激活；切 Mpr3d（强制 Grid → 槽 3 可见）后
// refreshLayout 恰补调 activate 一次（KTD8）。
TEST(VolumePanelLayout, ModeSwitchActivatesExactlyOnce)
{
  VolumeFixture fixture;
  GUIWindow window;
  GUICenter center(&fixture.viewModel);
  CountingVolumePanel volume(&fixture.viewModel, fixture.volumeRenderer);

  window.setViewMode(ViewMode::TwoDOnly);
  // 镜像 main.cpp 注册序：2D → 轴 → 冠 → 矢 → 3D
  FakeMprPanel axial(QStringLiteral("MPR 轴位"));
  FakeMprPanel coronal(QStringLiteral("MPR 冠状"));
  FakeMprPanel sagittal(QStringLiteral("MPR 矢状"));
  window.registerView(&center);
  window.registerView(&axial);
  window.registerView(&coronal);
  window.registerView(&sagittal);
  window.registerView(&volume);

  EXPECT_EQ(volume.activateCount, 0) << "仅 2D 下 3D 不占槽，不应激活";

  window.setViewMode(ViewMode::Mpr3d);

  EXPECT_EQ(window.layoutMode(), LayoutMode::Grid);
  EXPECT_EQ(window.panelInSlot(3), static_cast<IViewPanel*>(&volume))
      << "Mpr3d 下槽 0-3 = 轴/冠/矢/3D";
  EXPECT_EQ(volume.activateCount, 1)
      << "refreshLayout 应恰好补调 activate 一次";
  EXPECT_TRUE(fixture.volumeRenderer->bound) << "activate 应注入渲染目标";
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 控件测试无显示服务器：强制 offscreen 平台（须在 QApplication 构造前）
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
