# 1.ps1
# DicomViewer CMake 配置脚本

$HomeDir = "D:"
$VcpkgRoot = "$HomeDir\vcpkg"
$VcpkgCMake = "$VcpkgRoot\downloads\tools\cmake-4.3.3-windows\cmake-4.3.3-windows-x86_64\bin\cmake.exe"
$ProjectRoot = "$HomeDir\xcyxiner\DicomViewer"

$DicomDebugPath   = "$ProjectRoot\vcpkg_installed\x64-windows\debug"
$DicomReleasePath = "$ProjectRoot\vcpkg_installed\x64-windows"

& $VcpkgCMake -S . -B build `
    -D CMAKE_BUILD_TYPE=Debug `
    -D CMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -D "CMAKE_PREFIX_PATH=$DicomDebugPath;$DicomReleasePath" `
    -D VCPKG_TARGET_TRIPLET=x64-windows `
    -D VCPKG_OVERLAY_TRIPLETS="$ProjectRoot\triplets" -D DEPLOY_QT_PLUGINS=ON

# 如果需要同时构建，取消下面这行注释
& $VcpkgCMake --build build --config Debug 