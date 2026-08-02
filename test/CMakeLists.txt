

find_package(GTest CONFIG REQUIRED)

# 3. 编译测试可执行文件
add_executable(DcmtkReader_test
    DcmtkReader_test.cpp
)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../res/
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res/)
# 4. 测试程序链接「测试专用库」
target_link_libraries(DcmtkReader_test PRIVATE
    GTest::gtest_main
    dicom_core
)

# 5. 注册测试
include(GoogleTest)
gtest_discover_tests(DcmtkReader_test)