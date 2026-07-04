#cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(pwd)/vcpkg_installed/x64-linux/debug;$(pwd)/vcpkg_installed/x64-linux" -G Ninja  -DAUTO_DEPLOY_FONTS=ON
#cmake --build build -j10

cmake --preset wsl-debug
cmake --build --preset wsl-debug
