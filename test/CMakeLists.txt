

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

# 序列读取 (U1): 失败路径
# （依赖 res/series 夹具的目录打开 / 层序 / 逐层 UID / 患者几何用例
#   挪到 test/deprecated/，夹具回归后恢复）
add_executable(SeriesRead_test
    SeriesRead_test.cpp
)
target_link_libraries(SeriesRead_test PRIVATE
    GTest::gtest_main
    dicom_core
)
gtest_discover_tests(SeriesRead_test)

# 加载用例与失败通道 (U3): 缺 WW/WC 取默认值 / 无效路径失败结果
# （依赖 res/series 夹具的完整序列 StackDisplaySet / 体入槽位 /
#   单槽替换 / 旧状态保留用例挪到 test/deprecated/）
add_executable(LoadSeriesUseCase_test
    LoadSeriesUseCase_test.cpp
)
target_link_libraries(LoadSeriesUseCase_test PRIVATE
    GTest::gtest_main
    dicom_core
)
gtest_discover_tests(LoadSeriesUseCase_test)

# 2D 逐层导航 (U4): 未加载安全空操作 / N=1 空操作
# （依赖 res/series 夹具的夹取 / sliceChanged / 抽层渲染 /
#   2D↔MPR 联动用例挪到 test/deprecated/）
add_executable(SeriesViewModel_test
    SeriesViewModel_test.cpp
)
target_link_libraries(SeriesViewModel_test PRIVATE
    GTest::gtest_main
    dicom_core
)
gtest_discover_tests(SeriesViewModel_test)

# MPR 三平面面板 (U5): viewRegistered / loadFiles no-op / 空体占位 /
# N=1 单层占位 / 平面绑定 / 槽位分配 / refreshLayout 补调 activate
# （依赖 res/series 夹具的惰性投递 / 初始 WW/WC / R6 两顺序用例
#   挪到 test/deprecated/）
add_executable(MprPanel_test
    MprPanel_test.cpp
)
target_link_libraries(MprPanel_test PRIVATE
    GTest::gtest_main
    dicom_core
)
gtest_discover_tests(MprPanel_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# 视图模式（View 菜单）: 默认仅 2D / 槽位分配 / 分屏指派记忆 /
# 越权分配忽略 / 注册时机 / 信号恰发一次
add_executable(ViewMode_test
    ViewMode_test.cpp
)
target_link_libraries(ViewMode_test PRIVATE
    GTest::gtest_main
    dicom_core
)
gtest_discover_tests(ViewMode_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)