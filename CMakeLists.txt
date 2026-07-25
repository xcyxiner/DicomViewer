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

# ---- Declare library ----

# ---- Declare executable ----
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
if(DEFINED ENV{DCMDICTPATH})
    set(DCM_DICT_PATH "$ENV{DCMDICTPATH}")
    message(STATUS "Using DCMDICTPATH: ${DCM_DICT_PATH}")
else()
    # 可选：如果环境变量未设置，尝试查找默认路径（避免硬编码）
    find_file(DCM_DICT_PATH
        NAMES dicom.dic
        PATHS /usr/share/dcmtk /usr/local/share/dcmtk /opt/dcmtk/share/dcmtk
    )
    if(NOT DCM_DICT_PATH)
        message(FATAL_ERROR "DCMDICTPATH not set and dicom.dic not found in standard paths.")
    endif()
endif()
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

execute_process(
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/gen_src.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}  # <-- 关键：指定工作目录为源码目录
)  
include(source_list.cmake)

add_library(dicom_core STATIC
    ${SRC}
)

target_include_directories(dicom_core PUBLIC
    ${CMAKE_SOURCE_DIR}/source
)



add_executable(DicomViewer_exe ${SRC})
add_executable(DicomViewer::exe ALIAS DicomViewer_exe)

set_property(TARGET DicomViewer_exe PROPERTY OUTPUT_NAME DicomViewer)
target_include_directories(DicomViewer_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

target_compile_features(DicomViewer_exe PRIVATE cxx_std_17)

target_link_libraries(dicom_core  
   PUBLIC  
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

target_link_libraries(DicomViewer_exe  
   PRIVATE  
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
# 为需要 DCMTK 的目标添加宏定义
target_compile_definitions(dicom_core PUBLIC
    DCM_DICT_DEFAULT_PATH="${DCM_DICT_PATH}"
)
# 如果还有其他目标（如 DicomViewer_exe）也编译该文件，同样添加
target_compile_definitions(DicomViewer_exe PRIVATE
    DCM_DICT_DEFAULT_PATH="${DCM_DICT_PATH}"
)
# ---------- Windows: 自动 windeployqt ----------
option(DEPLOY_QT_PLUGINS "Auto-copy Qt platform plugins after build" OFF)
if(WIN32 AND MSVC AND DEPLOY_QT_PLUGINS)
    set(_qt_debug_bin "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/bin")
    set(_qt_debug_plugins "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins")

    add_custom_command(TARGET DicomViewer_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:DicomViewer_exe>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_debug_plugins}/platforms/qwindowsd.dll"
            $<TARGET_FILE_DIR:DicomViewer_exe>/platforms/qwindowsd.dll
        COMMENT "Copying qwindowsd.dll (Debug)"
    )
endif()

# ============================================================
# 自动部署 Qt 字体（仅 Linux + Debug）
# ============================================================
option(AUTO_DEPLOY_FONTS "Auto copy system fonts for Qt (Linux/Debug only)" OFF)
if(AUTO_DEPLOY_FONTS AND UNIX)
    # Qt 会找这个路径
    set(QT_FONT_DEST "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/debug/lib/fonts")
    if(NOT EXISTS "${QT_FONT_DEST}")
        file(MAKE_DIRECTORY "${QT_FONT_DEST}")
        # 系统字体来源（Ubuntu 默认路径）
        set(SYSTEM_FONT_DIR "/usr/share/fonts/truetype/dejavu")
        if(EXISTS "${SYSTEM_FONT_DIR}")
            file(GLOB DEJAVU_FONTS "${SYSTEM_FONT_DIR}/*.ttf")
            foreach(FONT ${DEJAVU_FONTS})
                file(COPY "${FONT}" DESTINATION "${QT_FONT_DEST}")
                message(STATUS "  Deployed font: ${FONT}")
            endforeach()
            message(STATUS "✅ Deployed DejaVu fonts to ${QT_FONT_DEST}")
        else()
            message(WARNING "⚠️ DejaVu fonts not found at ${SYSTEM_FONT_DIR}")
            message(WARNING "  Install with: sudo apt install fonts-dejavu-core")
        endif()
    else()
        message(STATUS "Qt fonts already deployed at ${QT_FONT_DEST}")
    endif()
endif()


# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----
option(AUTO_TESTING "Enable automatic testing" OFF)
if(AUTO_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()


if(NOT DicomViewer_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of DicomViewer"
  )
endif()

include(cmake/dev-mode.cmake)
