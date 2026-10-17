<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/github/license/xcyxiner/DicomViewer" alt="License"></a>
  <a href="https://github.com/xcyxiner/DicomViewer/actions"><img src="https://img.shields.io/github/actions/workflow/status/xcyxiner/DicomViewer/ci.yml?style=flat-square" alt="CI"></a>
   <a href="https://codecov.io/gh/xcyxiner/DicomViewer"><img src="https://img.shields.io/codecov/c/github/xcyxiner/DicomViewer?style=flat-square" alt="Coverage"></a>
  <a href="https://github.com/xcyxiner/DicomViewer/commits/master"><img src="https://img.shields.io/github/last-commit/xcyxiner/DicomViewer?style=flat-square" alt="Last Commit"></a>
  <a href="https://github.com/xcyxiner/DicomViewer/stargazers"><img src="https://img.shields.io/github/stars/xcyxiner/DicomViewer?style=social" alt="Stars"></a>
</p>

# DicomViewer

基于 Qt + VTK 的 DICOM 医学影像查看器：2D 阅片、MPR 多平面重建、
3D 体绘制。C++17 / MVVM + DDD 分层架构。

## 目标

- 2D 阅片（已完成：序列加载、窗宽窗位、多布局）
- MPR 多平面重建（已完成）
- 3D 可视化（已完成）

## 参考资料

### 测试数据

- 单文件 dcm: [dicom-test-files.git](https://github.com/robyoung/dicom-test-files.git)
- 序列夹具 `res/series/` 已从仓库删除，依赖它的测试暂移到
  [test/deprecated/](test/deprecated/README.md)；用
  `python3 tools/gen_series_fixture.py --n 12` 可重新生成。

## 依赖

通过 vcpkg (manifest 模式, `vcpkg.json`) 统一管理，版本由 `overrides` 锁定：

| 依赖 | 版本 | 说明 |
|------|------|------|
| Qt | 6.8.3 | qtbase (widgets/gui/opengl) + qtwayland (Linux) |
| DCMTK | 3.6.7 | DICOM 协议与 tag 解析 |
| GDCM | 3.0.24 | 像素数据读取 |
| VTK | 9.3.0-pv5.12.1 | ParaView 5.12.1 fork，渲染引擎 |
| vtk-dicom | 0.8.17 | VTK 的 DICOM 支持 (gdcm 后端) |
| GoogleTest | - | 测试框架 |

```
系统: ubuntu 24.04 (WSL2) / Windows 11
标准: C++17
构建: CMake 3.14+ (Ninja on Linux, VS2022 on Windows)
包管理: vcpkg (manifest 模式)
```

## IDE以及开发环境
```
win11
wsl2 (ubuntu 24.04)
nvim
vscode (连接 wsl, clangd + gdb 调试)
clang-format / clang-tidy (代码格式化与静态检查)
```

## 文档记录
- [qt以及opengl 专栏](https://juejin.cn/column/7645237921838972978)
- [DicomViewer 1](https://juejin.cn/post/7652620835596976171)
- [DicomViewer (目录调整) 2](https://juejin.cn/post/7652286706327699498)
- [DicomViewer (添加模型类)3](https://juejin.cn/post/7652563235685531667)
- [DicomViewer (后台线程处理文件)4](https://juejin.cn/post/7652719984766320694)
- [DicomViewer (dcmtk读取dcm文件)5](https://juejin.cn/post/7652943828446806058)
- [DicomViewer (dcm模型类赋值)6](https://juejin.cn/post/7653651196972433427)
- [DicomViewer (vcpkg Windows和ubuntu编译)7](https://juejin.cn/post/7655901733764694079)
- [DicomViewer 8(编译以及优化)](https://juejin.cn/post/7658236946784747526)
- [DicomViewer 9(ui编辑以及引用)](https://juejin.cn/post/7658478348117704747)
- [DicomViewer 10(使用xxd查看dcm文件)](https://juejin.cn/post/7660788416040943656)
- [DicomViewer 11(vscode 连接wsl gdb断点)](https://juejin.cn/post/7660773919465373711)
- [DicomViewer12(架构更新)](https://juejin.cn/post/7661160061000269864)
- [DicomViewer13(vtk测试)](https://juejin.cn/post/7663505563164327936)
- [DicomViewer14(读取图像按固定窗宽窗位显示)](https://juejin.cn/post/7666250786229239842)
- [DicomViewer15(act验证ci)](https://juejin.cn/post/7668906037449736198)
- [DicomViewer 16 使用容器服务加载](https://juejin.cn/post/7678969657585156102)
- [DicomViewer 17 窗口自适应](https://juejin.cn/post/7679264879620407336)
- [DicomViewer 18 菜单调整窗位窗宽](https://juejin.cn/post/7679331643697561619)
- [DicomViewer19 更换测试文件](https://juejin.cn/post/7681688916479017006)
- [DicomViewer 20 后续计划](https://juejin.cn/post/7683353819854422052)
- [DicomViewer21 页面布局](https://juejin.cn/post/7684573646439202826)
- [DicomViewer22 读取序列](https://juejin.cn/post/7689314158995800110)
- [DicomViewer23 支持mpr](https://juejin.cn/post/7690781115092959259)
- [DicomViewer24 修复编译bug(window test 失败)](https://juejin.cn/post/7691585964052938798)
- [DicomViewer25 加载3d](https://juejin.cn/post/7697180584537112618)

## 目录结构

只列出 git 跟踪的文件

```
├── .claude/
│   └── commands/
│       └── compound-engineering.md
├── .githooks/
│   └── pre-commit
├── .github/
│   └── workflows/
│       ├── build-ci-image.yml
│       └── ci.yml
├── .vscode/
│   ├── c_cpp_properties.json
│   └── launch.json
├── cmake/
│   ├── coverage.cmake
│   ├── dev-mode.cmake
│   ├── docs-ci.cmake
│   ├── docs.cmake
│   ├── folders.cmake
│   ├── install-rules.cmake
│   ├── lint-targets.cmake
│   ├── lint.cmake
│   ├── prelude.cmake
│   ├── project-is-top-level.cmake
│   ├── spell-targets.cmake
│   ├── spell.cmake
│   └── variables.cmake
├── docs/
│   ├── pages/
│   │   └── about.dox
│   ├── plans/
│   │   ├── 2026-09-13-1533-refactor-multi-view-framework-plan.md
│   │   ├── 2026-09-25-1630-feat-series-reading-mpr-plan.md
│   │   └── 2026-10-03-1200-feat-mpr-3d-view-plan.md
│   ├── conf.py.in
│   └── Doxyfile.in
├── memory/
│   └── architecture-overview.md
├── res/
│   ├── CT_small.dcm
│   ├── ExplVR_LitEndNoMeta.dcm
│   ├── JPEG-LL.dcm
│   ├── JPEG2000.dcm
│   └── MR_small.dcm
├── source/
│   ├── application/
│   │   ├── LoadSeriesUseCase.cpp
│   │   └── LoadSeriesUseCase.h
│   ├── domain/
│   │   └── model/
│   │       ├── CoreRepository.cpp
│   │       ├── CoreRepository.h
│   │       ├── DisplaySettings.cpp
│   │       ├── DisplaySettings.h
│   │       ├── Frame.cpp
│   │       ├── Frame.h
│   │       ├── Image.cpp
│   │       ├── Image.h
│   │       ├── Patient.cpp
│   │       ├── Patient.h
│   │       ├── Series.cpp
│   │       ├── Series.h
│   │       ├── StackDisplaySet.cpp
│   │       ├── StackDisplaySet.h
│   │       ├── Study.cpp
│   │       └── Study.h
│   ├── infrastructure/
│   │   ├── cache/
│   │   │   ├── IFrameCache.cpp
│   │   │   ├── IFrameCache.h
│   │   │   ├── MemoryFrameCache.cpp
│   │   │   └── MemoryFrameCache.h
│   │   ├── container/
│   │   │   ├── ServiceContainer.cpp
│   │   │   └── ServiceContainer.h
│   │   ├── dicom_io/
│   │   │   ├── DcmtkReader.cpp
│   │   │   ├── DcmtkReader.h
│   │   │   ├── GdcmReader.cpp
│   │   │   ├── GdcmReader.h
│   │   │   ├── HybridReader.cpp
│   │   │   ├── HybridReader.h
│   │   │   ├── IDicomReader.cpp
│   │   │   ├── IDicomReader.h
│   │   │   ├── VTKDicomAdaptReader.cpp
│   │   │   └── VTKDicomAdaptReader.h
│   │   ├── rendering/
│   │   │   ├── IImageRenderer.cpp
│   │   │   ├── IImageRenderer.h
│   │   │   ├── IMprRenderer.cpp
│   │   │   ├── IMprRenderer.h
│   │   │   ├── IVolumeRenderer.cpp
│   │   │   ├── IVolumeRenderer.h
│   │   │   ├── VolumeUsability.h
│   │   │   ├── VtkAdaptRenderer.cpp
│   │   │   ├── VtkAdaptRenderer.h
│   │   │   ├── VtkMprRenderer.cpp
│   │   │   ├── VtkMprRenderer.h
│   │   │   ├── VtkVolumeRenderer.cpp
│   │   │   └── VtkVolumeRenderer.h
│   │   ├── task/
│   │   │   ├── ITaskQueue.cpp
│   │   │   ├── ITaskQueue.h
│   │   │   ├── QtTaskQueue.cpp
│   │   │   └── QtTaskQueue.h
│   │   └── utils/
│   │       ├── FloatCompare.h
│   │       ├── ImageDataComparator.cpp
│   │       ├── ImageDataComparator.h
│   │       ├── ImageSliceExtractor.cpp
│   │       ├── ImageSliceExtractor.h
│   │       └── WindowLevelDrag.h
│   ├── presentation/
│   │   ├── view/
│   │   │   ├── gui/
│   │   │   │   ├── GUICenter.cpp
│   │   │   │   ├── GUICenter.h
│   │   │   │   ├── GUICenter.ui
│   │   │   │   ├── MprPanel.cpp
│   │   │   │   ├── MprPanel.h
│   │   │   │   ├── VolumePanel.cpp
│   │   │   │   └── VolumePanel.h
│   │   │   ├── menu/
│   │   │   │   ├── FileMenu.cpp
│   │   │   │   ├── FileMenu.h
│   │   │   │   ├── FileMenu.ui
│   │   │   │   ├── LayoutMenu.cpp
│   │   │   │   ├── LayoutMenu.h
│   │   │   │   ├── LayoutMenu.ui
│   │   │   │   ├── ViewMenu.cpp
│   │   │   │   ├── ViewMenu.h
│   │   │   │   ├── ViewMenu.ui
│   │   │   │   ├── WindowLevelMenu.cpp
│   │   │   │   ├── WindowLevelMenu.h
│   │   │   │   └── WindowLevelMenu.ui
│   │   │   ├── window/
│   │   │   │   ├── GUIWindow.cpp
│   │   │   │   ├── GUIWindow.h
│   │   │   │   └── GUIWindow.ui
│   │   │   ├── IViewPanel.h
│   │   │   ├── LayoutMode.h
│   │   │   └── ViewMode.h
│   │   └── viewmodels/
│   │       ├── SeriesViewModel.cpp
│   │       └── SeriesViewModel.h
│   └── main.cpp
├── test/
│   ├── deprecated/
│   │   ├── LoadSeriesUseCase_test.cpp
│   │   ├── MprPanel_test.cpp
│   │   ├── README.md
│   │   ├── SeriesRead_test.cpp
│   │   ├── SeriesViewModel_test.cpp
│   │   └── TestCommon.h
│   ├── CMakeLists.txt
│   ├── DcmtkReader_test.cpp
│   ├── LoadSeriesUseCase_test.cpp
│   ├── MprPanel_test.cpp
│   ├── SeriesRead_test.cpp
│   ├── SeriesViewModel_test.cpp
│   ├── TestCommon.h
│   ├── ViewMode_test.cpp
│   └── VolumePanel_test.cpp
├── tools/
│   ├── linux/
│   │   ├── ci-cmake.sh
│   │   ├── cmake-build.sh
│   │   ├── docker-build-ci.sh
│   │   ├── lint-check.sh
│   │   └── vcpkg-install.sh
│   ├── windows/
│   │   └── cmake-build.ps1
│   ├── gen_series_fixture.py
│   ├── new_c_class.py
│   └── new_qt_object.py
├── triplets/
│   ├── x64-linux.cmake
│   ├── x64-windows-release.cmake
│   └── x64-windows.cmake
├── vcpkg-cache/
│   └── downloads/
│       ├── gl2ps-1.4.2.tgz
│       ├── k202009-libaec-v1.1.6.tar.gz
│       └── libuuid-1.0.3.tar.gz
├── .clang-format
├── .clang-tidy
├── .clangd
├── .codespellrc
├── .gitignore
├── BUILDING.md
├── CLAUDE.md
├── CMakeLists.txt
├── CMakePresets.json
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── Dockerfile
├── gen_src.py
├── HACKING.md
├── LICENSE
├── README.md
├── source_list.cmake
└── vcpkg.json
```

## Building and installing
See the [BUILDING](BUILDING.md) document.

### Linux (WSL2)

```bash
# 配置 (Ninja + vcpkg, x64-linux)
# 前提: 环境变量 CMAKE_TOOLCHAIN_FILE 指向 vcpkg 的
# scripts/buildsystems/vcpkg.cmake (预设本身不设置 toolchain)
cmake --preset wsl-vcpkg-debug

# 编译
cmake --build build

# 运行测试
ctest --test-dir build

# 运行程序
./build/DicomViewer
```

### Windows (VS2022)

```powershell
# 配置 (需先安装 vcpkg 到 D:/vcpkg, 或修改 CMakePresets.json 中的路径)
cmake --preset win-vcpkg-debug

# 编译
cmake --build build --config Debug

# 运行测试
ctest --test-dir build -C Debug

# 运行程序 (VS 多配置生成器; 若用 Ninja 单配置则在 build\DicomViewer.exe)
.\build\Debug\DicomViewer.exe
```

### 常用目标

以下 lint/spell 目标定义在开发者模式中，需在配置时加
`-DDicomViewer_DEVELOPER_MODE=ON`（或使用 `CMakeUserPresets.json` 中的
`dev` 预设）才会生成：

```bash
cmake --preset wsl-vcpkg-debug -DDicomViewer_DEVELOPER_MODE=ON
cmake --build build --target format-check   # 检查格式 (clang-format --dry-run)
cmake --build build --target format-fix     # 自动修复格式
cmake --build build --target spell-check    # 拼写检查 (codespell)
```

clang-tidy 没有独立的构建目标，它通过 `CMAKE_CXX_CLANG_TIDY` 在编译时生效
（`ci-ubuntu`、`dev` 等预设已启用），正常编译即会运行。

## Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

## Licensing

[MIT](LICENSE)
