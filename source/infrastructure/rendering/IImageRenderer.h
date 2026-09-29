
#pragma once
#include <functional>
#include <memory>
#include "infrastructure/cache/IFrameCache.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "domain/model/DisplaySettings.h"
class IImageRenderer
{
public:
    explicit IImageRenderer();
    virtual ~IImageRenderer()=default;
    virtual void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window) = 0; // 设置渲染目标
    virtual void render(const IFrameCache::FramePtr& frame,
                        const DisplaySettings& settings) = 0;
    virtual void updateWindowLevel(double windowWidth, double windowCenter) = 0;
    virtual void reset() = 0;
    virtual void fitToWindow() = 0;

    // 注册窗宽窗位变更回调（联动用）。
    // 处理流程：实现内部观察 vtkCommand::WindowLevelEvent（用户在视图内
    // 拖动改窗宽窗位）→ 调用该回调回灌 ViewModel → ViewModel 广播给所有面板；
    // 回调统一由 ViewModel 持有，面板不直接挂 renderer 回调。
    // 程序化回写（updateWindowLevel）期间须抑制回调并做值比较幂等，防止成环。
    // 默认空实现，VTK 渲染器按需覆写。
    virtual void setWindowLevelCallback(
        std::function<void(double windowWidth, double windowCenter)> callback)
    {
        (void)callback;
    }
};
