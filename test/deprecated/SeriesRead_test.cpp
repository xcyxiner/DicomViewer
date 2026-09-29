#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkStringArray.h>

#include "TestCommon.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"

namespace
{

// 夹具文件名与 IPP 层序被打乱（固定种子），层序断言因此等价于
// "打乱枚举顺序结果不变"。期望值解析见 TestCommon.h。

// 从 readFrame 输出中取出该层的 SOP Instance UID（FieldData 约定）。
std::string sliceSopUid(const IFrameCache::FramePtr& frame)
{
  auto* image = std::get_if<vtkSmartPointer<vtkImageData>>(&frame);
  if (!image || !*image) {
    return {};
  }
  auto* arr = vtkStringArray::SafeDownCast(
      (*image)->GetFieldData()->GetAbstractArray("SOPInstanceUIDs"));
  if (!arr || arr->GetNumberOfValues() == 0) {
    return {};
  }
  return arr->GetValue(0);
}

constexpr double kTolerance = 1e-3;

}  // namespace

TEST(SeriesReadTest, SingleFileOpenGetsOneFrame)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  EXPECT_NO_THROW(reader.open("res/CT_small.dcm"));
  EXPECT_EQ(reader.getFrameCount(), 1);
  EXPECT_EQ(sliceSopUid(reader.readFrame(0)), exp.baseUid);
}

TEST(SeriesReadTest, DirectoryOpenGetsAllFrames)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  EXPECT_NO_THROW(reader.open("res/series"));
  EXPECT_EQ(reader.getFrameCount(), exp.count);
}

TEST(SeriesReadTest, SlicesSortedByImagePositionPatient)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  reader.open("res/series");
  for (int i = 0; i < exp.count; ++i) {
    auto info = reader.readFrameInfo(i);
    ASSERT_NE(info, nullptr) << "第 " << i << " 层元数据缺失";
    EXPECT_EQ(info->getSopInstanceUid(), exp.uids[i]);
    EXPECT_NEAR(info->getImagePositionPatient()[2], exp.ippZ[i], kTolerance)
        << "第 " << i << " 层 IPP z 与期望层序不符";
  }
}

TEST(SeriesReadTest, ReadFrameReturnsDistinctPerSliceUids)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  reader.open("res/series");
  std::vector<std::string> seen;
  for (int i = 0; i < exp.count; ++i) {
    auto uid = sliceSopUid(reader.readFrame(i));
    EXPECT_EQ(uid, exp.uids[i]) << "第 " << i << " 层 SOP UID 不匹配";
    seen.push_back(uid);
  }
  std::sort(seen.begin(), seen.end());
  EXPECT_EQ(std::unique(seen.begin(), seen.end()) - seen.begin(), exp.count)
      << "逐层 SOP UID 应互不相同";
}

TEST(SeriesReadTest, SeriesInstanceUidMatchesFixture)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  reader.open("res/series");
  EXPECT_EQ(reader.getSeriesInstanceUid(), exp.seriesUid);
}

TEST(SeriesReadTest, VolumeCarriesPatientGeometry)
{
  auto exp = loadExpectation("res/series/expected.txt");
  VTKDicomAdaptReader reader;
  reader.open("res/series");
  auto volume = reader.readVolume();
  ASSERT_NE(volume, nullptr);

  int extent[6];
  volume->GetExtent(extent);
  EXPECT_EQ(extent[0], 0);
  EXPECT_EQ(extent[1], exp.cols - 1);
  EXPECT_EQ(extent[2], 0);
  EXPECT_EQ(extent[3], exp.rows - 1);
  EXPECT_EQ(extent[4], 0);
  EXPECT_EQ(extent[5], exp.count - 1);

  double spacing[3];
  volume->GetSpacing(spacing);
  EXPECT_NEAR(spacing[0], exp.pixelSpacingX, 1e-4);
  EXPECT_NEAR(spacing[1], exp.pixelSpacingY, 1e-4);
  EXPECT_NEAR(spacing[2], exp.sliceThickness, kTolerance);

  // KTD6 断言：患者几何已烘入体 —— 体素 (0,0,k) 的患者坐标
  // 应等于第 k 层的 ImagePositionPatient。
  for (int k = 0; k < exp.count; ++k) {
    double point[3];
    volume->TransformIndexToPhysicalPoint(0, 0, k, point);
    EXPECT_NEAR(point[0], exp.ippX, kTolerance) << "层 " << k << " x";
    EXPECT_NEAR(point[1], exp.ippY, kTolerance) << "层 " << k << " y";
    EXPECT_NEAR(point[2], exp.ippZ[k], kTolerance) << "层 " << k << " z";
  }
}

TEST(SeriesReadTest, InvalidPathsThrowCatchable)
{
  {
    VTKDicomAdaptReader reader;
    EXPECT_THROW(reader.open("res/does_not_exist.dcm"), std::exception);
  }
  {
    auto emptyDir =
        std::filesystem::temp_directory_path() / "dicomviewer_empty_dir";
    std::filesystem::remove_all(emptyDir);
    std::filesystem::create_directories(emptyDir);
    VTKDicomAdaptReader reader;
    EXPECT_THROW(reader.open(emptyDir.string()), std::exception);
    std::filesystem::remove_all(emptyDir);
  }
  {
    auto nonDicomDir =
        std::filesystem::temp_directory_path() / "dicomviewer_non_dicom_dir";
    std::filesystem::remove_all(nonDicomDir);
    std::filesystem::create_directories(nonDicomDir);
    std::ofstream(nonDicomDir / "a.txt") << "not a dicom file";
    VTKDicomAdaptReader reader;
    EXPECT_THROW(reader.open(nonDicomDir.string()), std::exception);
    std::filesystem::remove_all(nonDicomDir);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
