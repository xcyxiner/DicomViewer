#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"

// 依赖 res/series 夹具（目录帧数 / IPP 层序 / 逐层 UID / 患者几何 /
// SeriesInstanceUID）的用例已挪到 test/deprecated/SeriesRead_test.cpp，
// 待夹具回归后一并恢复。

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
