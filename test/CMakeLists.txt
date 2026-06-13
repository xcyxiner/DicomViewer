# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(DicomViewerTests LANGUAGES CXX)

# ---- Tests ----

add_executable(DicomViewer_test source/DicomViewer_test.cpp)
target_link_libraries(DicomViewer_test PRIVATE DicomViewer_lib)
target_compile_features(DicomViewer_test PRIVATE cxx_std_17)

add_test(NAME DicomViewer_test COMMAND DicomViewer_test)

# ---- End-of-file commands ----

add_folders(Test)
