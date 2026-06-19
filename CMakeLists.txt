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
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(DCMTK_DIR "/opt/dcmtk/lib/cmake/dcmtk")  
set(VTK_DIR "/usr/local/lib/cmake/vtk-9.6")  
find_package(DCMTK REQUIRED)
find_package(VTK REQUIRED COMPONENTS  
  CommonCore  
  CommonDataModel  
  IOImage  
  ImagingCore  
  RenderingCore  
  RenderingOpenGL2  
  RenderingVolumeOpenGL2  
  InteractionStyle  
  RenderingQt  
)

qt_standard_project_setup()

execute_process(COMMAND python gen_src.py)  
include(source_list.cmake)

add_executable(DicomViewer_exe ${SRC})
add_executable(DicomViewer::exe ALIAS DicomViewer_exe)

set_property(TARGET DicomViewer_exe PROPERTY OUTPUT_NAME DicomViewer)
target_include_directories(DicomViewer_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

target_compile_features(DicomViewer_exe PRIVATE cxx_std_17)

target_link_libraries(DicomViewer_exe  
   PRIVATE  
     Qt6::Widgets  
     DCMTK::DCMTK  
     VTK::CommonCore  
     VTK::CommonDataModel  
     VTK::IOImage  
     VTK::ImagingCore  
     VTK::RenderingCore  
     VTK::RenderingOpenGL2  
     VTK::RenderingVolumeOpenGL2  
     VTK::InteractionStyle  
     VTK::RenderingQt  
)

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
