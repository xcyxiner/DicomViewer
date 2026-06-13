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

add_library(
    DicomViewer_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    DicomViewer_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(DicomViewer_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(DicomViewer_exe source/main.cpp)
add_executable(DicomViewer::exe ALIAS DicomViewer_exe)

set_property(TARGET DicomViewer_exe PROPERTY OUTPUT_NAME DicomViewer)

target_compile_features(DicomViewer_exe PRIVATE cxx_std_17)

target_link_libraries(DicomViewer_exe PRIVATE DicomViewer_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT DicomViewer_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of DicomViewer"
  )
endif()

include(cmake/dev-mode.cmake)
