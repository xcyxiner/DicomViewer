
#pragma once
#include <memory>
#include "infrastructure/cache/IFrameCache.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "domain/model/DisplaySettings.h"
class IImageRenderer 
{
public:
    explicit IImageRenderer();
    ~IImageRenderer()=default;
    virtual void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window) = 0; // 设置渲染目标
    virtual void render(const IFrameCache::FramePtr& frame,
                        const DisplaySettings& settings) = 0;
    virtual void reset() = 0;
};
