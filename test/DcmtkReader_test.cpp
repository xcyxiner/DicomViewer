#include "infrastructure/dicom_io/DcmtkReader.h"  // 根据你的实际路径调整

#include <gtest/gtest.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include "infrastructure/cache/MemoryFrameCache.h"  // 根据你的实际路径调整
#include "infrastructure/dicom_io/GdcmReader.h"  // 根据你的实际路径调整
#include "infrastructure/dicom_io/HybridReader.h"  // 根据你的实际路径调整
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"  // 根据你的实际路径调整
#include "infrastructure/rendering/VtkAdaptRenderer.h"  // 根据你的实际路径调整
std::string single_dicom =
    "/home/ubuntu/xcyxiner/DicomViewer/res/CT_small.dcm";  // 准备一个真实的DICOM文件
std::string multiframe_dicom =
    "/home/ubuntu/xcyxiner/DicomViewer/res/enhanced_sample.dcm";  // 准备一个多帧的DICOM文件

TEST(DcmtkReaderTest, OpenValidFile)
{
  DcmtkReader reader;
  EXPECT_NO_THROW(reader.open(single_dicom));
}

TEST(DcmtkReaderTest, readFrameInfo)
{
  DcmtkReader reader;
  EXPECT_NO_THROW(reader.open(single_dicom));
  EXPECT_NO_THROW(reader.readFrameInfo(0));  // 确保会抛出异常
  auto frameInfo = reader.readFrameInfo(0);
  EXPECT_NE(frameInfo, nullptr);  // 确保返回的frameInfo不为空
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

TEST(VtkAdaptRendererTest, CompareImageData)
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

          EXPECT_NE(imageData, nullptr);  // 确保返回的imageData不为空

          std::visit(
              [&renderer, &imageData](auto&& vtkArg)
              {
                using VTK_T = std::decay_t<decltype(vtkArg)>;
                if constexpr (std::is_same_v<VTK_T,
                                             vtkSmartPointer<vtkImageData>>) {
                  EXPECT_NE(vtkArg, nullptr);  // 确保返回的vtkImageData不为空
                  renderer.CompareImageData(imageData, vtkArg);
                } else if constexpr (std::is_same_v<VTK_T, std::nullptr_t>) {
                  FAIL() << "VTK Frame is nullptr";  // 如果是nullptr，测试失败
                }
              },
              vtkFrame);

        } else if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          EXPECT_NE(arg, nullptr);  // 确保返回的imageData不为空
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          FAIL() << "Frame is nullptr";  // 如果是nullptr，测试失败
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

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
