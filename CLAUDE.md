# CLAUDE.md — DicomViewer 项目指南

## 项目概述

DICOM 医学影像查看器，支持 2D 图像浏览、MPR 多平面重建、3D 可视化。

- **语言/标准**: C++17
- **构建系统**: CMake 3.14+ (Ninja on Linux, VS2022 on Windows)
- **包管理**: vcpkg (manifest 模式)
- **GUI 框架**: Qt 6.8.3 (Widgets + OpenGLWidgets)
- **渲染引擎**: VTK 9.3 (ParaView 5.12.1 fork)
- **DICOM 解析**: DCMTK 3.6.7 / GDCM 3.0.24 / vtk-dicom 0.8.17
- **测试框架**: Google Test

## 架构: MVVM + DDD

四层分层架构:

```
source/
├── domain/model/          # 领域层 — 纯 C++ 实体，无框架依赖
│   ├── Patient            # 顶层聚合根，拥有 Study 集合
│   ├── Study              # 属于 Patient，拥有 Series 集合
│   ├── Series             # 属于 Study，拥有 Image 集合
│   ├── Image              # 属于 Series，持有文件路径和 SOP 信息
│   ├── Frame              # 值对象 — 像素数据 + 空间元数据
│   ├── DisplaySettings    # 值对象 — WW/WC 显示参数
│   ├── StackDisplaySet    # 聚合 — frameUids + currentIndex + DisplaySettings
│   └── CoreRepository     # 仓储模式 — 管理所有 Patient 的生命周期
│
├── application/           # 应用层 — 用例编排
│   └── LoadSeriesUseCase  # 注入 IDicomReader + ITaskQueue + IFrameCache
│
├── infrastructure/        # 基础设施层 — 接口实现
│   ├── dicom_io/
│   │   ├── IDicomReader       # 抽象接口: open/readSeries/readFrame/close
│   │   ├── DcmtkReader        # DCMTK 实现
│   │   ├── GdcmReader         # GDCM 实现
│   │   ├── VTKDicomAdaptReader # VTK 实现
│   │   └── HybridReader       # 组合模式 — DcmtkReader + GdcmReader 互备
│   ├── rendering/
│   │   ├── IImageRenderer     # 抽象接口: setRenderTarget/render/reset
│   │   └── VtkAdaptRenderer   # VTK 渲染实现
│   ├── cache/
│   │   ├── IFrameCache        # 抽象缓存接口
│   │   └── MemoryFrameCache   # 内存缓存 (unordered_map)
│   └── task/
│       ├── ITaskQueue         # 抽象异步任务接口
│       └── QtTaskQueue        # QThreadPool 实现
│
└── presentation/          # 表现层 — MVVM
    ├── viewmodels/
    │   └── SeriesViewModel    # QObject，持有所有基础设施实例，暴露 loadSeries/render
    └── view/
        ├── window/GUIWindow   # QMainWindow，菜单栏 + 中心区域
        ├── gui/GUICenter      # QWidget，绑定 SeriesViewModel，连接信号/槽
        └── menu/FileMenu      # File > Open (Ctrl+F)
```

### 关键数据流

```
用户点击 Open
  → FileMenu → GUIWindow::openFile()
    → GUICenter::onOpenFile() [QFileDialog]
      → emit addFiles(paths)
        → SeriesViewModel::loadSeries(path)
          → LoadSeriesUseCase::loadSeriesAsync(path)
            → ITaskQueue::submitWithResult() [后台线程]
              → IDicomReader::open() + readFrame()
                → IFrameCache::put()
                  → 返回 StackDisplaySet
              → emit imageChanged()
                → SeriesViewModel::render()
                  → IFrameCache::get()
                    → IImageRenderer::render(frame, displaySettings)
```

### 关键类型

```cpp
// 帧数据的多态表示 — 避免继承，用 variant
using FramePtr = std::variant<
    std::shared_ptr<Frame>,           // 原始像素数据
    vtkSmartPointer<vtkImageData>,    // VTK 预处理数据
    std::nullptr_t
>;
```

## 代码规范

### 命名约定 (由 .clang-tidy 强制)

| 类型 | 风格 | 示例 |
|------|------|------|
| 类/结构体 | PascalCase | `SeriesViewModel` |
| 函数/方法 | camelCase | `loadSeries()` |
| 变量 | camelCase | `currentIndex` |
| 私有/保护成员 | m_ 前缀 + camelCase | `m_reader`, `m_frameCache` |
| 宏 | UPPER_CASE | `CMAKE_CXX_STANDARD` |
| 模板参数 | CamelCase | `FramePtr` |

### 格式 (.clang-format)

- 基于 Google 风格
- 80 列限制
- 2 空格缩进
- 类/函数/枚举使用 Allman 风格大括号 (左括号换行)
- #include 按组重排

### 编码要求

- 优先使用智能指针 (`unique_ptr`, `shared_ptr`)，避免裸 new/delete
- 领域层不依赖任何框架 (Qt, VTK, DCMTK)
- 接口 (I 前缀) 定义在 infrastructure 层，实现类在同一目录
- 使用 `std::future` 处理异步结果
- 构造函数注入依赖，便于测试和替换

## 构建与开发

### 常用命令

```bash
# 配置 (使用 wsl-debug 预设)
cmake --preset wsl-debug

# 编译
cmake --build build

# 运行测试
ctest --test-dir build

# 代码格式化
cmake --build build --target format

# 拼写检查
cmake --build build --target spell

# 静态分析 (通过 clang-tidy)
cmake --build build --target tidy
```

### CMake 预设

| 预设 | 用途 |
|------|------|
| `wsl-debug` | 本地开发 (WSL2) |
| `ci-ubuntu` | CI Linux 构建 |
| `ci-windows` | CI Windows 构建 |
| `ci-coverage` | 代码覆盖率 (lcov) |
| `ci-sanitize` | ASan + UBSan |

### 测试

- 测试文件: `test/DcmtkReader_test.cpp`
- 测试数据: `res/*.dcm`
- 新增测试在 `test/CMakeLists.txt` 中添加

## 添加新组件

### 新增 C++ 类
```bash
python3 tools/new_c_class.py
```

### 新增 Qt 对象
```bash
python3 tools/new_qt_object.py
```

### 新增源文件后
```bash
python3 gen_src.py   # 重新生成 source_list.cmake
```

## CI/CD

- GitHub Actions (`.github/workflows/ci.yml`)
- 矩阵: Ubuntu 24.04 + Windows 2022
- 流水线: lint → coverage → sanitize → test → package → release
- Docker 镜像: `.github/workflows/build-ci-image.yml` 推送到 GHCR
