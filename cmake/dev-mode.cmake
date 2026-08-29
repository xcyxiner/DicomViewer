include(cmake/folders.cmake)

include(CTest)
# if(BUILD_TESTING)
#   add_subdirectory(test)
# endif()

add_custom_target(
    run-exe
    COMMAND DicomViewer_exe
    VERBATIM
)
add_dependencies(run-exe DicomViewer_exe)

option(BUILD_MCSS_DOCS "Build documentation using Doxygen and m.css" OFF)
if(BUILD_MCSS_DOCS)
  include(cmake/docs.cmake)
endif()

option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if(ENABLE_COVERAGE)
  include(cmake/coverage.cmake)
endif()

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

# ---- Git hooks setup ----
add_custom_target(
    setup-hooks
    COMMAND git config core.hooksPath .githooks
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Setting git hooks path to .githooks"
    VERBATIM
)

add_folders(Project)
