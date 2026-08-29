cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    DicomViewer
    VERSION 0.1.0
    DESCRIPTION "qt6 vtk gdcm dcmtk"
    HOMEPAGE_URL "https://github.com/xcyxiner/DicomViewer"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- 查找依赖 ----
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

find_package(DCMTK REQUIRED)
find_package(VTK REQUIRED COMPONENTS
    GUISupportQt
    CommonCore
    CommonDataModel
    FiltersCore
    IOImage
    ImagingColor
    ImagingCore
    ImagingGeneral
    ImagingMath
    RenderingCore
    RenderingOpenGL2
    RenderingVolumeOpenGL2
    InteractionStyle
    RenderingQt
)
find_package(dicom CONFIG REQUIRED)
find_package(GDCM CONFIG REQUIRED)
qt_standard_project_setup()

# ---- 生成源文件列表 ----
execute_process(
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/gen_src.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}  # <-- 关键：指定工作目录为源码目录
)  
include(source_list.cmake)


# ---- 测试库（可选） ----
option(AUTO_TESTING "Enable automatic testing" OFF)
if(AUTO_TESTING)
    add_library(dicom_core STATIC ${SRC})
    target_include_directories(dicom_core PUBLIC ${CMAKE_SOURCE_DIR}/source)
endif()

add_executable(DicomViewer_exe WIN32 ${SRC})
add_executable(DicomViewer::exe ALIAS DicomViewer_exe)

set_property(TARGET DicomViewer_exe PROPERTY OUTPUT_NAME DicomViewer)
target_include_directories(DicomViewer_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

target_compile_features(DicomViewer_exe PRIVATE cxx_std_17)
# ---- 公共依赖 ----
set(COMMON_LIBS
    Qt6::Widgets
    Qt6::OpenGLWidgets
    DCMTK::dcmdata
    DCMTK::ofstd
    DCMTK::dcmfg
    VTK::GUISupportQt
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersCore
    VTK::IOImage
    VTK::ImagingColor
    VTK::ImagingCore
    VTK::ImagingGeneral
    VTK::RenderingCore
    VTK::RenderingOpenGL2
    VTK::RenderingVolumeOpenGL2
    VTK::InteractionStyle
    VTK::RenderingQt
    VTK::DICOM
    gdcmDICT
    gdcmIOD
    gdcmDSED
    gdcmCommon
    gdcmMSFF
)
if(AUTO_TESTING)
    target_link_libraries(dicom_core PUBLIC ${COMMON_LIBS})
    target_compile_features(dicom_core PUBLIC cxx_std_17)
endif()
target_link_libraries(DicomViewer_exe PRIVATE ${COMMON_LIBS})

# ---- DCMTK 字典路径 ----
if(UNIX)
    set(DCM_DICT_PATH "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/dcmtk/dicom.dic")
    if(AUTO_TESTING)
        target_compile_definitions(dicom_core PUBLIC DCM_DICT_DEFAULT_PATH="${DCM_DICT_PATH}")
    endif()
    target_compile_definitions(DicomViewer_exe PRIVATE DCM_DICT_DEFAULT_PATH="${DCM_DICT_PATH}")
endif()

# ---- Windows: 自动部署 Qt 插件 ----
option(DEPLOY_QT_PLUGINS "Auto-copy Qt platform plugins after build" OFF)
if(WIN32 AND MSVC AND DEPLOY_QT_PLUGINS)
    set(_triplet "${VCPKG_TARGET_TRIPLET}")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(_qt_plugins "${CMAKE_BINARY_DIR}/vcpkg_installed/${_triplet}/debug/Qt6/plugins")
        set(_plugin_suffix "d")
        set(_config_name "Debug")
    else()
        set(_qt_plugins "${CMAKE_BINARY_DIR}/vcpkg_installed/${_triplet}/Qt6/plugins")
        set(_plugin_suffix "")
        set(_config_name "Release")
    endif()

    add_custom_command(TARGET DicomViewer_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:DicomViewer_exe>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_plugins}/platforms/qwindows${_plugin_suffix}.dll"
            $<TARGET_FILE_DIR:DicomViewer_exe>/platforms/qwindows${_plugin_suffix}.dll
        COMMENT "Copying qwindows${_plugin_suffix}.dll (${_config_name})"
    )
endif()

# ---- Linux: 自动部署 Qt 字体 ----
option(AUTO_DEPLOY_FONTS "Auto copy system fonts for Qt (Linux/Debug only)" OFF)
if(AUTO_DEPLOY_FONTS AND UNIX)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(QT_FONT_DEST "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib/fonts")
    else()
        set(QT_FONT_DEST "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib/fonts")
    endif()

    if(NOT EXISTS "${QT_FONT_DEST}")
        file(MAKE_DIRECTORY "${QT_FONT_DEST}")

        set(FONT_DIRS
            "/usr/share/fonts/truetype/dejavu"
            "/usr/share/fonts/truetype/liberation"
            "/usr/share/fonts/truetype/ubuntu"
            "/usr/share/fonts/truetype/freefont"
        )

        set(FONT_COPIED FALSE)
        foreach(FONT_DIR ${FONT_DIRS})
            if(EXISTS "${FONT_DIR}")
                file(GLOB FONTS "${FONT_DIR}/*.ttf")
                foreach(FONT ${FONTS})
                    file(COPY "${FONT}" DESTINATION "${QT_FONT_DEST}")
                    message(STATUS "  Deployed font: ${FONT}")
                endforeach()
                message(STATUS "Deployed fonts from ${FONT_DIR} to ${QT_FONT_DEST}")
                set(FONT_COPIED TRUE)
                break()
            endif()
        endforeach()

        if(NOT FONT_COPIED)
            message(WARNING "No font directories found. Please install fonts with:")
            message(WARNING "  sudo apt install fonts-dejavu-core fonts-liberation")
        endif()
    else()
        message(STATUS "Qt fonts already deployed at ${QT_FONT_DEST}")
    endif()
endif()

# ---- 复制资源文件 ----
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res/)

# ---- 安装规则 ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- 测试 ----
if(AUTO_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

# ---- 开发者模式 ----
if(NOT DicomViewer_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(AUTHOR_WARNING "Developer mode is intended for developers of DicomViewer")
endif()

include(cmake/dev-mode.cmake)
