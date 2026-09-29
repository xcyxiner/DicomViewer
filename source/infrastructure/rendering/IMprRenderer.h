
#pragma once
#include <functional>
#include "vtkImageData.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"

// MPR 三平面渲染器接口（KTD3：与 2D 的 IImageRenderer 分离，两视图
// 渲染职责不同）。由 main.cpp 组合根构造具体实现并注入 MprPanel
// （KTD8，面板不自建实现）。基类方法带默认空实现，便于测试假件替换。
class IMprRenderer
{
public:
    // 子视口平面编号（调用方与实现共用的具名常量，避免魔法数）
    static constexpr int kPlaneAxial = 0;  // 轴位
    static constexpr int kPlaneCoronal = 1;  // 冠状
    static constexpr int kPlaneSagittal = 2;  // 矢状

    explicit IMprRenderer();
    virtual ~IMprRenderer()=default;

    // 注入子视口的渲染目标。plane: 0=轴位 1=冠状 2=矢状
    // （MprPanel 持有自己的三个 QVTKOpenGLNativeWidget 各自的
    //  vtkGenericOpenGLRenderWindow）。幂等：同一 window/plane
    // 重复注入直接返回；上下文重建后由 render() 补绘。
    virtual void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window,
                                 int plane);

    // 设置整幅体（唯一像素源，KTD1/KTD6：体已含患者几何）。
    // 三平面共享同一 cursor 与 LUT（KTD12）；无体时三联显示占位，
    // 由面板层处理，renderer 侧 no-op 不崩溃。
    virtual void setVolume(vtkSmartPointer<vtkImageData> volume);

    // 窗宽窗位（程序化写入，三平面同步；KTD11 的联动广播在
    // ViewModel 层，本接口只做本地应用）。写入后主动重绘。
    virtual void updateWindowLevel(double windowWidth, double windowCenter);

    // 重置三平面相机并重绘（fit 语义，面板 fitToWindow 转发）。
    virtual void reset();

    // 重绘三个子视口（GL 上下文重建/状态变更后调用，KTD8）。
    virtual void render();

    // 切面定位入口（正向联动 R8：把 plane 子视口的切面定位到体素
    // 索引 position；轴对齐模式下切面即 viewer 的 Slice，内部夹取到
    // 该平面的 SliceRange）。渲染目标未注入（activate 前）时跳过，
    // 由 activate 后面板补推（KTD8）。基类默认空实现。
    virtual void setSlicePosition(int plane, double position);

    // 注册窗宽窗位交互回灌回调（KTD11）。
    // 处理流程：实现内部观察三联子视口的窗宽窗位交互
    // （vtkCommand::WindowLevelEvent）→ 调用该回调回灌 ViewModel →
    // ViewModel 广播给所有面板；程序化 updateWindowLevel 期间抑制
    // 回调并做值比较幂等（防广播-回灌成环）。回调统一由 ViewModel
    // 经 MprPanel 挂接一行转发，面板不实现变换/抑制逻辑。
    // 基类默认空实现。
    virtual void setWindowLevelCallback(
        std::function<void(double windowWidth, double windowCenter)>
            callback);

    // 注册轴位切面变更回灌回调（R8 反向链路）。
    // 处理流程：子视口切面（轴位 pane 的体素 z）变化时携带新索引调用
    // 该回调 → 面板转发 ViewModel::setCurrentIndex（内部夹取
    // [0, N-1]、同值幂等防环）。基类默认空实现。
    virtual void setSliceIndexCallback(
        std::function<void(double voxelIndex)> callback);
};
