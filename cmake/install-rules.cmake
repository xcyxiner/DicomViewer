install(
    TARGETS DicomViewer_exe
    RUNTIME COMPONENT DicomViewer_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
