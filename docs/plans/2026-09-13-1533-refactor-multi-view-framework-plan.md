---
title: 多视图框架重构 - 计划
type: refactor
date: 2026-09-13
artifact_contract: ce-unified-plan/v1
artifact_readiness: implementation-ready
product_contract_source: ce-brainstorm
execution: code
---

## 目标胶囊

**目标：** GUIWindow 支持多种布局模式（1x1、1x2、2x2），每个格子可独立选择视图，使未来新增视图（MPR、3D 等）可灵活组合显示。

**手段：** QGridLayout + IViewPanel 接口 + LayoutMenu（KTD1、KTD2、KTD3）。

## 产品合约

### 概述

将 GUIWindow 从单视图架构重构为多视图网格布局架构。引入 IViewPanel 作为视图抽象，QGridLayout 支持 1x1/1x2/2x2 三种布局模式，LayoutMenu 提供布局切换和视图分配。GUICenter 成为第一个具体的 IViewPanel 实现。文件打开由 GUIWindow 全局管理。

### 问题背景

GUIWindow 当前硬编码了单个 GUICenter 控件。菜单信号处理器使用 `dynamic_cast<GUICenter*>(m_childWidget)` 调用视图特定方法。新增视图需要修改 GUIWindow 的信号接线，违反开闭原则。本次重构建立一个框架，支持多种布局模式，每个格子可独立放置不同视图，为 MPR（多平面重建）等需要同时显示多个视图的场景提供基础。

### 需求

**布局管理：**
1. R1. GUIWindow 使用 QGridLayout 管理视图，支持三种布局模式：1x1（单视图全屏）、1x2（左右双视图）、2x2（四宫格）。
2. R2. 布局模式通过 LayoutMenu 切换，切换时保留已分配的视图。
3. R3. 每个格子可独立选择显示哪个已注册视图（通过格子右键菜单或下拉选择）。

**视图接口：**
4. R4. 所有中心视图实现 IViewPanel，定义 viewName()、activate()、deactivate()、fitToWindow()、setWindowLevel()、resetWindowLevel() 和 loadFiles()。
5. R5. IViewPanel 包含 setWindowLevel(windowWidth, windowCenter) 和 resetWindowLevel() 方法。不支持窗宽窗位的视图提供空实现。
6. R6. IViewPanel 包含 fitToWindow() 用于缩放适应窗口。

**菜单集成：**
7. R7. LayoutMenu 出现在菜单栏，提供布局模式选择（1x1/1x2/2x2）和视图分配。
8. R8. 选择布局模式切换网格布局，已分配的视图保持不变。

**文件操作：**
9. R9. GUIWindow 全局管理文件打开。打开文件时，GUIWindow 将文件路径传递给所有活动 IViewPanel 的 loadFiles(paths) 方法。
10. R10. GUICenter 的 onOpenFile/onOpenFolder 移至 GUIWindow 层。

**解耦：**
11. R11. WindowLevelMenu 信号通过当前焦点视图的 IViewPanel 通用方法路由。
12. R12. GUIWindow 不再使用 `dynamic_cast<GUICenter*>(m_childWidget)` 进行任何操作。

### 关键决策

- **QGridLayout 替代 QStackedWidget**（会话已定：用户定向 — 选择于 QStackedWidget：支持多视图同时显示，为 MPR 等场景提供基础）。管辖 R1、R2。
- **IViewPanel 使用通用窗宽窗位方法**（会话已定：用户批准 — 所有视图实现，非 DICOM 视图为空实现）。管辖 R5、R11。
- **GUIWindow 全局管理文件打开**（会话已定：用户定向 — GUIWindow 拥有文件对话框并将路径传递给所有活动视图）。管辖 R9、R10。

### 范围边界

**包含：**
- IViewPanel 接口定义
- GUICenter 适配为 IViewPanel
- GUIWindow 网格布局管理重构
- LayoutMenu 创建（布局切换 + 视图分配）
- WindowLevelMenu 信号解耦
- 文件打开迁移至 GUIWindow

**推迟后续工作：**
- 具体新视图（MPR、3D 等）— 独立计划
- 视图间数据共享机制
- 视图拖拽重排
- 动态布局（用户自定义行列数）

### 来源

- `source/presentation/view/window/GUIWindow.h/.cpp` — 当前单视图主窗口
- `source/presentation/view/gui/GUICenter.h/.cpp` — 需适配的现有视图
- `source/presentation/view/menu/WindowLevelMenu.h/.cpp` — 需解耦的 DICOM 特定菜单
- `source/presentation/view/menu/FileMenu.h/.cpp` — 文件菜单信号
- `source/main.cpp` — 组合根

## 规划合约

### 关键技术决策

**KTD1. QGridLayout 作为布局管理器。**
GUIWindow 使用 QGridLayout 替代直接的 `ui->layout->addWidget(t_widget)`。支持三种布局模式：1x1（1行1列）、1x2（1行2列）、2x2（2行2列）。`setGridLayout(mode)` 方法负责清空并重建网格，将已注册视图放入对应格子。
（会话已定：用户定向 — 选择于 QStackedWidget：支持多视图同时显示）

**KTD2. IViewPanel 作为纯虚接口。**
新的抽象类，包含：`virtual QString viewName() const = 0`（纯虚）、`virtual void activate()`、`virtual void deactivate()`、`virtual void fitToWindow()`、`virtual void setWindowLevel(double ww, double wc)`、`virtual void resetWindowLevel()`、`virtual void loadFiles(const QStringList& paths)`。除 viewName() 外均有空默认实现。
（会话已定：用户批准 — 通用方法加空默认实现）

**KTD3. LayoutMenu 提供布局切换和视图分配。**
LayoutMenu 遵循现有菜单模式（QMenu 子类 + .ui 文件）。包含布局模式选择（QActionGroup 单选）和视图分配子菜单。每个格子有一个子菜单，列出所有已注册视图供选择。
（会话已定：用户批准 — 独立菜单）

**KTD4. 文件打开迁移至 GUIWindow。**
GUICenter 的 `onOpenFile()`/`onOpenFolder()` 逻辑（QFileDialog）移至 GUIWindow。GUIWindow 将文件路径传递给所有活动视图的 `loadFiles(paths)` 方法。GUICenter 的 `addFiles` 信号和文件对话框代码被移除；`loadFiles()` 直接调用 `m_seriesViewModel->loadSeries()`。

**KTD5. WindowLevelMenu 通过焦点视图路由。**
GUIWindow 追踪当前焦点所在的格子索引（`m_focusedSlot`）。WindowLevelMenu 信号通过该格子中的 IViewPanel 路由。点击格子可切换焦点。

### 高层技术设计

```
GUIWindow (QMainWindow)
├── QMenuBar
│   ├── FileMenu
│   ├── WindowLevelMenu
│   └── LayoutMenu（新增）
├── QGridLayout（新增，替代直接布局）
│   ├── [0,0] IViewPanel*（格子0）
│   ├── [0,1] IViewPanel*（格子1，1x2/2x2模式）
│   ├── [1,0] IViewPanel*（格子2，2x2模式）
│   └── [1,1] IViewPanel*（格子3，2x2模式）
├── m_panels: QVector<IViewPanel*>（已注册视图列表）
├── m_slotAssignments: QVector<int>（格子→视图索引映射）
├── m_focusedSlot: int（当前焦点格子）
└── m_layoutMode: LayoutMode（当前布局模式）
```

布局模式：
```
1x1:          1x2:          2x2:
┌─────────┐   ┌──────┬──────┐   ┌──────┬──────┐
│  视图 A  │   │ 视图A │ 视图B │   │ 视图A │ 视图B │
│         │   │      │      │   ├──────┼──────┤
└─────────┘   └──────┴──────┘   │ 视图C │ 视图D │
                                 └──────┴──────┘
```

视图分配流程：
1. 用户点击格子 → 设置 `m_focusedSlot`
2. 用户从 LayoutMenu 的格子子菜单选择视图 → 更新 `m_slotAssignments`
3. 调用 `refreshLayout()` → 重建 QGridLayout 内容

文件打开流程：
1. GUIWindow::openFile() → QFileDialog → 获取路径
2. 遍历所有格子中的活动视图，调用 `panel->loadFiles(paths)`

### 假设

- GUICenter 的 VTK 渲染窗口管理（setRenderWindow、fitToWindow）保留在 GUICenter 的 IViewPanel 方法实现内部。
- 未来视图也将实现 IViewPanel。当视图需要超出 IViewPanel 的能力时，这些是视图内部的。
- 初始实现中，1x1 模式默认显示 GUICenter，与当前行为一致。
- 空格子显示占位符（如灰色背景 + "无视图" 文字）。

## 实施单元

### U1. 定义 IViewPanel 接口

**目标：** 创建所有中心视图必须实现的抽象基类。

**需求：** R4、R5、R6

**依赖：** 无

**文件：**
- 新建 `source/presentation/view/IViewPanel.h`
- 修改 `source_list.cmake`

**方案：**
1. 创建 IViewPanel 作为 QWidget 子类，包含纯虚方法和默认实现方法。
2. 纯虚方法：`viewName() const -> QString`。
3. 带空默认实现的虚方法：`activate()`、`deactivate()`、`fitToWindow()`、`setWindowLevel(double ww, double wc)`、`resetWindowLevel()`、`loadFiles(const QStringList& paths)`。
4. IViewPanel 继承 QWidget，以便可添加到 QGridLayout。

**遵循模式：** 项目中现有的接口模式（如 `IDicomReader.h`、`IImageRenderer.h`）。

**测试场景：**
- IViewPanel 可被子类化并实例化（编译时验证）。
- 默认方法实现可调用且不崩溃。

**验证：** 项目使用新接口编译通过。

### U2. 适配 GUICenter 为 IViewPanel

**目标：** 使 GUICenter 实现 IViewPanel，保留所有现有行为。

**需求：** R4、R5、R6、R10

**依赖：** U1

**文件：**
- 修改 `source/presentation/view/gui/GUICenter.h`
- 修改 `source/presentation/view/gui/GUICenter.cpp`

**方案：**
1. GUICenter 仅继承 IViewPanel（IViewPanel 继承 QWidget，QWidget 功能通过传递继承获得）。不要同时继承 QWidget 和 IViewPanel — 那会造成 QObject 的菱形继承。
2. 实现 `viewName()` 返回描述性名称（如 "2D Viewer"）。
3. 实现 `activate()` — 调用 `m_seriesViewModel->setRenderWindow(m_renderWindow)`（当前在 `showEvent()` 中完成）。
4. 实现 `deactivate()` — 空实现或根据需要停止渲染。
5. 移除 `onOpenFile()`/`onOpenFolder()` 文件对话框逻辑（移至 GUIWindow 的 U3）。GUICenter 的 `loadFiles(paths)` 直接调用 `m_seriesViewModel->loadSeries()`；`addFiles` 信号被移除。
6. 现有的 `fitToWindow()`、`setWindowLevel()`、`resetWindowLevel()` 已匹配 IViewPanel 签名 — 添加 `override`。
7. 移除公共 `showEvent()` 方法 — 其逻辑（setRenderWindow）现在在 `activate()` 中。

**遵循模式：** 当前 GUICenter 实现；Qt 虚方法模式。

**测试场景：**
- GUICenter 使用 IViewPanel 实现编译通过。
- `viewName()` 返回非空字符串。
- `activate()` 设置渲染窗口（通过手动冒烟测试验证）。
- `loadFiles(paths)` 触发序列加载（验证 DICOM 文件加载并渲染）。

**验证：** 构建成功。打开 DICOM 文件 — 2D 查看器正确渲染。

### U3. 重构 GUIWindow 支持网格布局管理

**目标：** GUIWindow 使用 QGridLayout 管理多个 IViewPanel，支持 1x1/1x2/2x2 布局模式。

**需求：** R1、R2、R3、R9、R10、R12

**依赖：** U1、U2

**文件：**
- 修改 `source/presentation/view/window/GUIWindow.h`
- 修改 `source/presentation/view/window/GUIWindow.cpp`
- 修改 `source/main.cpp`

**方案：**
1. 定义布局模式枚举 `enum class LayoutMode { Single, Horizontal, Grid }`（对应 1x1、1x2、2x2）。
2. 替换 `QWidget* m_childWidget` 为：
   - `QGridLayout* m_gridLayout`（在构造函数中以编程方式创建，添加到 `ui->layout`；不修改 GUIWindow.ui）
   - `QVector<IViewPanel*> m_panels`（已注册视图列表）
   - `QVector<int> m_slotAssignments`（格子→视图索引映射，-1 表示空格子）
   - `int m_focusedSlot = 0`（当前焦点格子）
   - `LayoutMode m_layoutMode = LayoutMode::Single`（当前布局模式）
3. 添加 `registerView(IViewPanel* panel)` 方法 — 将视图添加到 `m_panels`。
4. 添加 `setGridLayout(LayoutMode mode)` 方法 — 更新 `m_layoutMode`，调用 `refreshLayout()`。
5. 添加 `assignViewToSlot(int slot, int viewIndex)` 方法 — 更新 `m_slotAssignments`，调用 `refreshLayout()`。
6. 添加 `refreshLayout()` 方法 — 清空 QGridLayout 中的所有控件，按 `m_layoutMode` 和 `m_slotAssignments` 重新放置视图。空格子显示占位符 QWidget。
7. 添加 `setFocusedSlot(int slot)` 方法 — 点击格子时设置焦点。
8. 迁移文件 open：`openFile()`/`openFolder()` 直接使用 QFileDialog，然后遍历所有活动视图调用 `loadFiles(paths)`。
9. 替换所有 `dynamic_cast<GUICenter*>(m_childWidget)->xxx()` 为焦点视图的对应方法调用。
10. 在 `createMenu()` 中连接 `FileMenu::openFile` 信号到 `GUIWindow::openFile()`。
11. 从 GUIWindow.cpp 移除 `#include "presentation/view/gui/GUICenter.h"`；改为 `#include "presentation/view/IViewPanel.h"`。
12. 在 `main.cpp` 中：创建 GUICenter，调用 `window.registerView(&center)`。默认 1x1 布局，格子0 显示 GUICenter。
13. 移除 `setContent(QWidget*)` 方法。移除 `showEvent()` 首次显示逻辑。

**遵循模式：** Qt QGridLayout 模式；现有 GUIWindow 结构。

**测试场景：**
- GUIWindow 使用 QGridLayout 编译通过。
- `registerView()` 将视图添加到列表。
- `setGridLayout(LayoutMode::Single)` 显示单视图全屏。
- `setGridLayout(LayoutMode::Horizontal)` 显示左右双视图。
- `setGridLayout(LayoutMode::Grid)` 显示四宫格。
- `assignViewToSlot()` 将视图分配到指定格子。
- 空格子显示占位符。
- 点击格子设置焦点。
- 文件打开传递路径给所有活动视图。
- GUIWindow 代码中不存在 `dynamic_cast<GUICenter*>`。

**验证：** 构建成功。1x1 模式下打开 DICOM 文件正常渲染。切换到 1x2 模式显示两个格子。

### U4. 添加 LayoutMenu

**目标：** 创建 LayoutMenu，提供布局模式切换和格子视图分配。

**需求：** R7、R8

**依赖：** U3

**文件：**
- 新建 `source/presentation/view/menu/LayoutMenu.h`
- 新建 `source/presentation/view/menu/LayoutMenu.cpp`
- 新建 `source/presentation/view/menu/LayoutMenu.ui`
- 修改 `source/presentation/view/window/GUIWindow.cpp`（createMenu）
- 修改 `source_list.cmake`

**方案：**
1. LayoutMenu 遵循现有菜单模式（QMenu 子类 + .ui 文件）。
2. 布局模式部分：QActionGroup 单选，包含 "1×1"、"1×2"、"2×2" 三个选项。
3. 格子分配部分：每个格子一个子菜单（"格子 1"、"格子 2" 等），子菜单列出所有已注册视图名称供选择。
4. 信号：`layoutModeChanged(LayoutMode)`、`slotViewChanged(int slot, int viewIndex)`。
5. GUIWindow 的 `createMenu()` 创建 LayoutMenu 并连接信号。
6. `registerView()` 时更新 LayoutMenu 的格子子菜单选项。
7. `setGridLayout()` 时更新 LayoutMenu 的布局模式选中状态和格子子菜单可见性。

**遵循模式：** FileMenu.h/.cpp/.ui 和 WindowLevelMenu.h/.cpp/.ui 模式。

**测试场景：**
- LayoutMenu 出现在菜单栏。
- 布局模式选项可切换（单选行为）。
- 格子子菜单列出已注册视图。
- 选择格子中的视图触发 `assignViewToSlot()`。
- 切换布局模式时格子子菜单数量正确更新。

**验证：** 构建成功。菜单栏显示 Layout 菜单。切换布局模式网格正确更新。

### U5. 解耦 WindowLevelMenu 与 GUICenter

**目标：** WindowLevelMenu 信号通过焦点视图的 IViewPanel 通用方法路由。

**需求：** R11、R12

**依赖：** U3

**文件：**
- 修改 `source/presentation/view/window/GUIWindow.cpp`（createMenu 信号连接）

**方案：**
1. 添加辅助方法 `focusedPanel()` — 返回 `m_slotAssignments[m_focusedSlot]` 对应的 IViewPanel，或 nullptr（空格子时）。
2. 将 GUIWindow::createMenu() 中所有 WindowLevelMenu 信号连接从 `dynamic_cast<GUICenter*>(m_childWidget)->setWindowLevel(...)` 改为 `focusedPanel()->setWindowLevel(...)`（需空指针检查）。
3. `resetWindowLevel()` 和 `fitToWindow()` 同理。
4. WindowLevelMenu 本身无变化。

**遵循模式：** GUIWindow::createMenu() 中现有的信号/槽连接模式。

**测试场景：**
- WindowLevelMenu Brain 预设调用焦点视图的 `setWindowLevel(80, 40)`。
- WindowLevelMenu Lung 预设调用焦点视图的 `setWindowLevel(1500, -600)`。
- WindowLevelMenu Bone 预设调用焦点视图的 `setWindowLevel(2000, 300)`。
- FitToWindow 调用焦点视图的 `fitToWindow()`。
- 空格子聚焦时菜单操作安全忽略（nullptr 检查）。
- GUIWindow 中不存在 `dynamic_cast<GUICenter*>`。

**验证：** 构建成功。应用窗宽窗位预设 — DICOM 查看器正确响应。切换焦点格子后操作作用于新焦点视图。

## 验证合约

- **构建：** `cmake --preset wsl-debug && cmake --build build`
- **测试：** `ctest --test-dir build`
- **格式化：** `cmake --build build --target format`
- **静态分析：** `cmake --build build --target tidy`（如可用）
- **手动冒烟测试：**
  1. 1x1 模式：打开 DICOM 文件，验证 2D 查看器渲染。
  2. 切换 1x2 模式：验证左右两个格子，左侧显示 GUICenter。
  3. 切换 2x2 模式：验证四宫格，空格子显示占位符。
  4. 切换回 1x1 模式：验证视图恢复正常。
  5. 应用窗宽窗位预设，验证焦点视图响应。

## 完成定义

- 所有 5 个实施单元完成且构建成功。
- GUIWindow 代码中不存在 `dynamic_cast<GUICenter*>`。
- 支持 1x1、1x2、2x2 三种布局模式，可通过 LayoutMenu 切换。
- 每个格子可通过 LayoutMenu 子菜单分配视图。
- 空格子显示占位符。
- GUICenter 在 1x1 模式下与之前工作完全一致。
- 窗宽窗位预设通过焦点视图的 IViewPanel 路由正常工作。
- 文件打开将路径传递给所有活动视图。
- `source_list.cmake` 包含所有新文件。
