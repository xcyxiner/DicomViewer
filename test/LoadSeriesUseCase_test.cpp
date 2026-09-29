#include <QCoreApplication>

#include "application/LoadSeriesUseCase.h"

#include <gtest/gtest.h>

#include "domain/model/DisplaySettings.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/task/QtTaskQueue.h"

// 依赖 res/series 夹具的用例（完整序列 StackDisplaySet / 初始 WW/WC /
// 体入槽位 / 单槽替换 / 旧状态保留）已挪到
// test/deprecated/LoadSeriesUseCase_test.cpp，待夹具回归后一并恢复。

// ---- 用例层（loadSeriesAsync 直接 .get()）----

// 第 0 帧无 WW/WC（CT_small）时使用 DisplaySettings 默认值。
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

// 不存在路径以失败结果完成，不产生未捕获异常。
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

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 跨线程信号回投主线程需要事件循环
  QCoreApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
