#include <thread>
#include <vector>

#include "infrastructure/dicom_io/DcmtkReader.h"

#include <gtest/gtest.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/dicom_io/GdcmReader.h"
#include "infrastructure/dicom_io/HybridReader.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/VtkAdaptRenderer.h"
#include "infrastructure/utils/ImageDataComparator.h"
std::string single_dicom = "res/CT_small.dcm";  // 准备一个真实的DICOM文件
std::string mr_dicom = "res/MR_small.dcm";  // 包含窗宽窗位的DICOM文件
std::string multiframe_dicom =
    "res/enhanced_sample.dcm";  // 准备一个多帧的DICOM文件

TEST(DcmtkReaderTest, OpenValidFile)
{
  DcmtkReader reader;
  EXPECT_NO_THROW(reader.open(single_dicom));
}

TEST(DcmtkReaderTest, readFrameInfo)
{
  DcmtkReader reader;
  EXPECT_NO_THROW(reader.open(single_dicom));
  EXPECT_NO_THROW(reader.readFrameInfo(0));  // 确保不会抛出异常
  auto frameInfo = reader.readFrameInfo(0);
  EXPECT_NE(frameInfo, nullptr);  // 确保返回的frameInfo不为空
}

TEST(DcmtkReaderTest, readWindowWidthCenter)
{
  DcmtkReader reader;
  reader.open(mr_dicom);
  auto frameInfo = reader.readFrameInfo(0);
  ASSERT_NE(frameInfo, nullptr);
  double ww = frameInfo->getWindowWidth();
  double wc = frameInfo->getWindowCenter();
  EXPECT_GT(ww, 0.0) << "DcmtkReader: WindowWidth should be positive";
  EXPECT_NE(wc, 0.0) << "DcmtkReader: WindowCenter should be non-zero";
}

TEST(GdcmReaderTest, OpenValidFile)
{
  GdcmReader reader;
  EXPECT_NO_THROW(reader.open(single_dicom));
}

TEST(GdcmReaderTest, readFrame)
{
  GdcmReader reader;
  reader.open(single_dicom);
  EXPECT_NO_THROW(reader.readFrame(0));  // 确保不会抛出异常
  auto frame = reader.readFrame(0);
  std::visit(
      [](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的frame不为空
        } else if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的frame不为空
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          FAIL() << "Frame is nullptr";  // 如果是nullptr，测试失败
        }
      },
      frame);
}

TEST(HybridReaderTest, OpenValidFile)
{
  HybridReader reader;
  EXPECT_NO_THROW(reader.open(single_dicom));
}

TEST(HybridReaderTest, readFrame)
{
  HybridReader reader;
  reader.open(single_dicom);
  EXPECT_NO_THROW(reader.readFrame(0));  // 确保不会抛出异常
  auto frame = reader.readFrame(0);
  std::visit(
      [](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的frame不为空
        } else if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的frame不为空
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          FAIL() << "Frame is nullptr";  // 如果是nullptr，测试失败
        }
      },
      frame);
}

TEST(VTKDicomAdaptReaderTest, readWindowWidthCenter)
{
  VTKDicomAdaptReader vtkReader;
  vtkReader.open(mr_dicom);
  auto vtkFrame = vtkReader.readFrame(0);
  ASSERT_EQ(std::get_if<std::nullptr_t>(&vtkFrame), nullptr)
      << "VTKDicomAdaptReader returned nullptr";

  auto imageData = std::get<vtkSmartPointer<vtkImageData>>(vtkFrame);
  ASSERT_NE(imageData, nullptr);

  vtkDoubleArray* wcArray = vtkDoubleArray::SafeDownCast(
      imageData->GetFieldData()->GetAbstractArray("WindowCenter"));
  vtkDoubleArray* wwArray = vtkDoubleArray::SafeDownCast(
      imageData->GetFieldData()->GetAbstractArray("WindowWidth"));
  ASSERT_NE(wcArray, nullptr) << "WindowCenter array missing from FieldData";
  ASSERT_NE(wwArray, nullptr) << "WindowWidth array missing from FieldData";
  EXPECT_GT(wwArray->GetValue(0), 0.0)
      << "VTKDicomAdaptReader: WindowWidth should be positive";
  EXPECT_NE(wcArray->GetValue(0), 0.0)
      << "VTKDicomAdaptReader: WindowCenter should be non-zero";
}

TEST(WindowWidthCenterConsistencyTest, DcmtkVsVTK)
{
  // DcmtkReader 读取
  DcmtkReader dcmtkReader;
  dcmtkReader.open(mr_dicom);
  auto frameInfo = dcmtkReader.readFrameInfo(0);
  ASSERT_NE(frameInfo, nullptr);
  double dcmtkWW = frameInfo->getWindowWidth();
  double dcmtkWC = frameInfo->getWindowCenter();

  // VTKDicomAdaptReader 读取
  VTKDicomAdaptReader vtkReader;
  vtkReader.open(mr_dicom);
  auto vtkFrame = vtkReader.readFrame(0);
  auto imageData = std::get<vtkSmartPointer<vtkImageData>>(vtkFrame);
  ASSERT_NE(imageData, nullptr);

  vtkDoubleArray* wcArray = vtkDoubleArray::SafeDownCast(
      imageData->GetFieldData()->GetAbstractArray("WindowCenter"));
  vtkDoubleArray* wwArray = vtkDoubleArray::SafeDownCast(
      imageData->GetFieldData()->GetAbstractArray("WindowWidth"));
  ASSERT_NE(wcArray, nullptr);
  ASSERT_NE(wwArray, nullptr);

  double vtkWW = wwArray->GetValue(0);
  double vtkWC = wcArray->GetValue(0);

  // 对比两者结果一致
  EXPECT_DOUBLE_EQ(dcmtkWW, vtkWW)
      << "WindowWidth mismatch: DcmtkReader=" << dcmtkWW
      << " VTKDicomAdaptReader=" << vtkWW;
  EXPECT_DOUBLE_EQ(dcmtkWC, vtkWC)
      << "WindowCenter mismatch: DcmtkReader=" << dcmtkWC
      << " VTKDicomAdaptReader=" << vtkWC;
}

TEST(VtkAdaptRendererTest, convertFrameToImageData)
{
  VtkAdaptRenderer renderer;
  HybridReader reader;
  reader.open(single_dicom);
  auto frame = reader.readFrame(0);
  std::visit(
      [&renderer](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
          auto imageData = renderer.convertFrameToImageData(arg);
          EXPECT_NE(imageData, nullptr);  // 确保返回的imageData不为空
        } else if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的imageData不为空
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          FAIL() << "Frame is nullptr";  // 如果是nullptr，测试失败
        }
      },
      frame);
}

TEST(ImageDataComparatorTest, CompareImageData)
{
  VtkAdaptRenderer renderer;
  HybridReader reader;
  VTKDicomAdaptReader vtkReader;
  reader.open(single_dicom);
  auto frame = reader.readFrame(0);
  vtkReader.open(single_dicom);
  auto vtkFrame = vtkReader.readFrame(0);
  std::visit(
      [&renderer, &vtkFrame](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
          auto imageData = renderer.convertFrameToImageData(arg);

          EXPECT_NE(imageData, nullptr);

          std::visit(
              [&imageData](auto&& vtkArg)
              {
                using VTK_T = std::decay_t<decltype(vtkArg)>;
                if constexpr (std::is_same_v<VTK_T,
                                             vtkSmartPointer<vtkImageData>>) {
                  EXPECT_NE(vtkArg, nullptr);
                  auto result = ImageDataComparator::compare(imageData, vtkArg);
                  EXPECT_TRUE(result.dimensionsMatch);
                  EXPECT_TRUE(result.scalarTypesMatch);
                } else if constexpr (std::is_same_v<VTK_T, std::nullptr_t>) {
                  FAIL() << "VTK Frame is nullptr";
                }
              },
              vtkFrame);

        } else if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          EXPECT_NE(arg, nullptr);
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          FAIL() << "Frame is nullptr";
        }
      },
      frame);
}

TEST(MemoryFrameCacheTest, PutAndGetFrame)
{
  std::shared_ptr<MemoryFrameCache> cache =
      std::make_shared<MemoryFrameCache>();
  std::string sopUid = "testSopUid";
  int frameIndex = 0;
  auto frame = std::make_shared<Frame>();
  cache->put(sopUid, frameIndex, frame);
  auto retrievedFrame = cache->get(sopUid, frameIndex);
  std::visit(
      [](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的frame不为空
        } else if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的frame不为空
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          FAIL() << "Frame is nullptr";  // 如果是nullptr，测试失败
        }
      },
      retrievedFrame);
}

namespace
{

vtkSmartPointer<vtkImageData> makeSmallVolume()
{
  auto volume = vtkSmartPointer<vtkImageData>::New();
  volume->SetDimensions(4, 4, 2);
  volume->AllocateScalars(VTK_SHORT, 1);
  return volume;
}

}  // namespace

// ---- 体槽位 (U2): 单槽语义 + 并发保护 ----

TEST(MemoryFrameCacheTest, PutAndGetVolumeRoundTrip)
{
  MemoryFrameCache cache;
  auto volume = makeSmallVolume();
  cache.putVolume("series-A", volume);
  auto retrieved = cache.getVolume("series-A");
  ASSERT_NE(retrieved, nullptr);
  // 指针相等：取回同一对象，未复制像素
  EXPECT_EQ(retrieved.GetPointer(), volume.GetPointer());
}

TEST(MemoryFrameCacheTest, VolumeSingleSlotReplacement)
{
  MemoryFrameCache cache;
  auto volumeA = makeSmallVolume();
  auto volumeB = makeSmallVolume();
  cache.putVolume("series-A", volumeA);
  cache.putVolume("series-B", volumeB);
  // 单槽：新体写入后旧体不可再取回
  EXPECT_EQ(cache.getVolume("series-A"), nullptr);
  auto retrieved = cache.getVolume("series-B");
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved.GetPointer(), volumeB.GetPointer());
}

TEST(MemoryFrameCacheTest, GetUnknownVolumeReturnsNull)
{
  MemoryFrameCache cache;
  EXPECT_EQ(cache.getVolume("no-such-series"), nullptr);
}

TEST(MemoryFrameCacheTest, ConcurrentPutGetSmoke)
{
  auto cache = std::make_shared<MemoryFrameCache>();
  auto volume = makeSmallVolume();
  constexpr int kRounds = 500;
  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back(
        [cache, volume, t]()
        {
          for (int i = 0; i < kRounds; ++i) {
            auto frame = std::make_shared<Frame>();
            cache->put("sop-uid", i % 8, frame);
            cache->putVolume("series-concurrent", volume);
            (void)cache->get("sop-uid", i % 8);
            (void)cache->getVolume("series-concurrent");
          }
        });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  EXPECT_NE(cache->getVolume("series-concurrent"), nullptr);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
