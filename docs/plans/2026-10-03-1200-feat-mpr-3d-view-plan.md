---
title: MPR + 3D 视图模式 - 计划
type: feat
date: 2026-10-03
artifact_contract: ce-unified-plan/v1
artifact_readiness: implementation-ready
product_contract_source: manual
execution: code
---

## Goal Capsule

- **目标：** View 菜单从三模式（仅 2D / 仅 MPR / 2D+MPR 分屏）收敛为两模式（仅 2D 默认 / MPR + 3D），MPR 模式下除三平面外新增 3D 体绘制面板；切模式自动联动布局。
- **手段：** 新增 `IVolumeRenderer`/`VtkVolumeRenderer`（vtkSmartVolumeMapper 光线投射 + WW/WC→传输函数）与 `VolumePanel`（生命周期仿 MprPanel）；`ViewMode` 改两值、`ViewRole` 增 `Volume`；`vtk_module_autoinit` 修复 VolumeOpenGL2 对象工厂注册（KTD13）。
- **权威层级：** 实现机制以本计划 KTD 为准；单元实现不得偏离其引用的 R/KTD。
- **停止条件：** 全部 R 满足、Verification Contract 全绿。
- **执行画像：** `execution: code`，6 个实施单元（U1 枚举菜单 → U2 GUIWindow / U3 渲染层并行 → U4 VolumePanel → U5 组合根 → U6 测试）。

---

## Product Contract

### Requirements

**视图模式收敛：**

- R10. View 菜单只有两项：「仅 2D」（默认）与「MPR + 3D」；旧「仅 MPR」「2D + MPR 分屏」及其槽位记忆逻辑移除。
- R11. 进入 MPR + 3D 自动切 2x2 Grid，四格 = 轴位 / 冠状 / 矢状 / 3D（按注册序落槽 0-3，2D 让位）；切回仅 2D 自动回 Single 且槽 0 = 2D。
- R12. 逐格「分配视图」在两种模式下均禁用；槽位完全由 ViewMode 决定，越权分配调用无效。

**3D 体绘制：**

- R13. 3D 面板显示体绘制（光线投射），支持鼠标旋转 / 平移 / 滚轮缩放；黑底，与 2D/MPR 视觉一致。
- R14. 窗宽窗位单向联动：2D 拖拽、MPR 拖拽、菜单预设任一处修改经 ViewModel 广播更新 3D 传输函数（最后操作为准）；3D 内不做 WW/WC 拖拽、无回灌。
- R15. 加载 / 占位 / 惰性激活与 MprPanel 一致：空体显示「无序列数据」，单层体（z < 2 层）显示「序列层数不足，无法 3D」；未激活只标脏，activate 补推（KTD8 顺序无关）；loadFiles no-op（KTD7）。

### Success Criteria

- 启动即仅 2D 单格；一键切「MPR + 3D」得四格自动 Grid，无「无视图」占位；切回恢复单格。
- 3D 格可旋转缩放；2D 改 WW/WC 时 3D 传输函数同步变化。
- 空数据 / 单层数据不崩溃、有占位提示；全量测试通过。

---

## Planning Contract (KTD)

- KTD13. VTK 对象工厂 autoinit：`vtk_module_autoinit(TARGETS DicomViewer_exe MODULES ${VTK_LIBRARIES})` 挂在 `target_link_libraries` 之后、仅最终可执行目标。缺它则 `vtkRenderingVolumeOpenGL2` override 未注册，`vtkSmartVolumeMapper` 构造内触发的 `vtkGPUVolumeRayCastMapper::New()` 返回 nullptr（组合根构造即崩溃）。测试目标不加（全用 Fake）。
- KTD14. 渲染接口契约与 MPR 同构：`IVolumeRenderer` 全 virtual + 空实现（测试假件替换），组合根构造注入 `VolumePanel`，面板不自建实现；`setRenderTarget` 同窗早退保用户相机；`updateWindowLevel` 同值早退（`FloatCompare::nearlyEqual`）；无体 / 无窗 renderer 侧 no-op。
- KTD15. 单层体可用性门槛单源：`isVolumeUsable` 抽到 `source/infrastructure/rendering/VolumeUsability.h`，`MprPanel` 与 `VolumePanel` 共用，防两处漂移。

---

## Implementation Units

- U1 枚举与菜单：`ViewMode { TwoDOnly, Mpr3d }`、`ViewRole::Volume`、`ViewMenu.ui/.cpp` 两 action。
- U2 GUIWindow：`isRoleActive` 角色矩阵、`setViewMode` 删分屏记忆 + 强制布局联动、`assignViewToSlot` 恒忽略、「分配视图」恒禁用。
- U3 渲染层：`IVolumeRenderer` + `VtkVolumeRenderer`（TF 映射、trackball、朝向、autoinit CMake 行）。
- U4 VolumePanel：仿 MprPanel 生命周期（惰性投递 / activate 补推 / 占位门槛 / KTD11 转发）。
- U5 组合根：ServiceContainer 显式实例化 + main.cpp 注册（registerView 序：2D → 轴 → 冠 → 矢 → 3D）。
- U6 测试：`ViewMode_test` 按两模式重写、`MprPanel_test` 三用例适配、新增 `VolumePanel_test`（六用例）+ CMake 目标。

## Verification Contract

- `cmake --preset wsl-vcpkg-debug && cmake --build build` 通过。
- `ctest --test-dir build` 全绿（41 用例，含 ViewMode 7、MprPanel 9、VolumePanel 6）。
- `ldd build/DicomViewer | grep RenderingVolume` 含 `libvtkRenderingVolumeOpenGL2`；offscreen 启动 5s 无 `no override found`、无崩溃。
- 手工（需多层序列数据）：四格布局、3D 旋转缩放、WW/WC 联动、占位文案、模式往返。
