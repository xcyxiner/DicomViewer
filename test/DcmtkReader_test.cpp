#include <gtest/gtest.h>
#include "infrastructure/dicom_io/DcmtkReader.h"  // 根据你的实际路径调整

// 测试1：打开有效的DICOM文件
TEST(DcmtkReaderTest, OpenValidFile) {
    DcmtkReader reader;
    std::string valid_dicom = "/home/ubuntu/xcyxiner/DicomViewer/res/CT_small.dcm";  // 准备一个真实的DICOM文件
    
    EXPECT_NO_THROW(reader.open(valid_dicom));
    // 如果有getter方法，可以进一步验证
    // EXPECT_FALSE(reader.getDataset().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}