#include <QApplication>
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
#include "infrastructure/task/QtTaskQueue.h"

// 依赖 res/series 夹具的用例（逐层导航夹取 / sliceChanged 恰一次 /
// 体抽层渲染 / 2D↔MPR 双向联动等）已挪到
// test/deprecated/SeriesViewModel_test.cpp，待夹具回归后一并恢复。

namespace
{

// 不建 GL 上下文的记录型渲染器：断言收到的抽层层号与次数。
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

  void updateWindowLevel(double, double) override {}

  void reset() override {}

  void fitToWindow() override { ++fitCount; }

  int renderCount = 0;
  int fitCount = 0;
  int lastSliceZ = -1;
  int lastSliceZMax = -1;
};

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

  bool loadAndWait(const std::string& path)
  {
    const int before = recorded.imageChangedCount;
    viewModel.loadSeries(path);
    return waitFor([&]() { return recorded.imageChangedCount > before; });
  }
};

}  // namespace

// 未加载序列时翻层调用为安全空操作（不发信号、不崩溃）。
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

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 无显示环境时回落 offscreen，与 MprPanel_test 的门禁一致
  if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")
      && !qEnvironmentVariableIsSet("DISPLAY"))
  {
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  }
  // 跨线程信号回投主线程需要事件循环
  QApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
