---
name: architecture-overview
description: DicomViewer 实际代码架构全貌（2026-09-25 快照），含与 CLAUDE.md 的差异点
metadata: 
  node_type: memory
  type: project
  originSessionId: d5ff9629-922d-4769-979a-44be7e88a721
  modified: 2026-09-25T07:48:22.399Z
---

# DicomViewer 整体架构（基于实际代码，2026-09-25）

四层 MVVM + DDD：`domain → application → infrastructure → presentation`，依赖方向由外向内，
领域层不依赖任何框架。以下是**实际接线**情况，与 CLAUDE.md 的差异见文末。

## 分层与职责

### domain/model（纯 C++ 实体）
- 聚合层级：`Patient → Study → Series → Image`，`CoreRepository` 管理 Patient 生命周期
- 值对象/聚合：`Frame`（像素 + 空间元数据 IPP/IOP）、`DisplaySettings`（WW/WC）、
  `StackDisplaySet`（frameUids + currentIndex + DisplaySettings）
- 注意：聚合层级主要供 `IDicomReader::readSeries()` 返回 `Series` 使用；
  加载主流程实际直接操作 `StackDisplaySet`，`CoreRepository`/`Patient`/`Study` 目前未被上层调用

### application
- `LoadSeriesUseCase`：构造注入 `IDicomReader&` + `ITaskQueue&` + `IFrameCache&`，
  暴露 `loadSeriesAsync(path) → std::future<shared_ptr<StackDisplaySet>>`

### infrastructure（接口 I 前缀 + 同目录实现）
- `dicom_io/`：`IDicomReader`（open / readSeries / readFrameInfo / readFrame）；
  实现有 `DcmtkReader`、`GdcmReader`、`VTKDicomAdaptReader`、`HybridReader`（Dcmtk+Gdcm 组合互备）
- `rendering/`：`IImageRenderer` → `VtkAdaptRenderer`（Frame → vtkImageData，应用 IPP/IOP 方位）
- `cache/`：`IFrameCache` → `MemoryFrameCache`（unordered_map，`FramePtr = variant<shared_ptr<Frame>, vtkSmartPointer<vtkImageData>, nullptr_t>`）
- `task/`：`ITaskQueue` → `QtTaskQueue`（QThreadPool）
- `container/`：`ServiceContainer` — 极简 DI 容器（type_index → shared_ptr<void>，
  registerInstance/resolve 模板）
- `utils/`：`ImageDataComparator`（测试辅助）

### presentation（MVVM）
- `viewmodels/SeriesViewModel`：QObject，构造注入 4 个基础设施接口 +
  `LoadSeriesUseCase`；暴露 `loadSeries/render/fitToWindow/setWindowLevel/resetWindowLevel`，
  信号 `imageChanged`
- `view/IViewPanel`：视图面板抽象基类（QWidget），定义 viewName / activate / deactivate /
  fitToWindow / 窗宽窗位 / loadFiles —— **多视图框架的核心接口**
- `view/LayoutMode.h`：`Single(1x1) / Horizontal(1x2) / Grid(2x2)`
- `view/window/GUIWindow`：QMainWindow，持有 `QVector<IViewPanel*> m_panels` +
  `m_slotAssignments`（格子→视图索引映射，-1 空）+ `m_focusedSlot`；
  提供 registerView / setGridLayout / assignViewToSlot / setFocusedSlot；
  信号 layoutModeChanged / viewRegistered / focusedSlotChanged
- `view/gui/GUICenter`：唯一 IViewPanel 实现（"2D Viewer"），内嵌 QVTKOpenGLNativeWidget
- `view/menu/`：`FileMenu`（Open）、`LayoutMenu`（布局切换 + 分配视图子菜单）、
  `WindowLevelMenu`（窗宽窗位预设）

## 启动与关键数据流

`main.cpp` 是 **Composition Root**：

```
main
  → ServiceContainer 注册: VtkAdaptRenderer / VTKDicomAdaptReader / QtTaskQueue / MemoryFrameCache
  → SeriesViewModel(4个 resolve 出的接口)          # 构造函数依赖注入
  → GUIWindow + GUICenter(&viewModel)
  → window.registerView(&center)                   # 目前只注册 1 个面板
```

加载流程：
```
FileMenu → GUIWindow::openFile() → GUICenter::loadFiles(paths)
  → SeriesViewModel::loadSeries(path)
    → LoadSeriesUseCase::loadSeriesAsync [QtTaskQueue 后台线程]
      → IDicomReader::open + readFrame → IFrameCache::put → StackDisplaySet
    → emit imageChanged() → render() → IFrameCache::get → IImageRenderer::render(frame, displaySettings)
```

## 与 CLAUDE.md 的差异（CLAUDE.md 尚未覆盖）

1. **ServiceContainer + Composition Root**：依赖注入不再散落在 main 里手写传递，有类型化容器
2. **多视图框架**（feature-multi-view 合入）：IViewPanel 抽象 + GUIWindow 网格槽位管理 +
   LayoutMenu，支持 1x1 / 1x2 / 2x2；当前只有 GUICenter 一个面板，空槽显示占位符
3. **运行时实际只用 `VTKDicomAdaptReader`**；DcmtkReader / GdcmReader / HybridReader
   存在但未接入 main（HybridReader 是二者的组合互备）
4. 新增 `WindowLevelMenu`、`LayoutMenu`、`ImageDataComparator`、`IViewPanel`、`LayoutMode`
5. GUIWindow 菜单为三个独立 QMenu 类，非单一 FileMenu

相关：[[chinese-docs]]（文档用中文编写）
