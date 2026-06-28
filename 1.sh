cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug -D CMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake -D CMAKE_PREFIX_PATH="$HOME/xcyxiner/DicomViewer/vcpkg_installed/x64-linux/debug;$HOME/xcyxiner/DicomViewer/vcpkg_installed/x64-linux"  -DVCPKG_TARGET_TRIPLET=x64-linux -DVCPKG_OVERLAY_TRIPLETS=$HOME/xcyxiner/DicomViewer/triplets
#cmake --build build
