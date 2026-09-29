
#pragma once
#include <functional>
#include "IImageRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkImageActor.h"
#include <vtkRenderer.h>
#include "vtkRenderWindow.h"
#include "vtkImageProperty.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindowInteractor.h"
class VtkAdaptRenderer : public IImageRenderer
{
public:
  explicit VtkAdaptRenderer();
  ~VtkAdaptRenderer() = default;

public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window) override;  // 设置渲染目标
  void render(const IFrameCache::FramePtr& frame,
              const DisplaySettings& settings) override;
  void updateWindowLevel(double windowWidth, double windowCenter) override;
  void reset() override;
  void fitToWindow() override;
  void setWindowLevelCallback(
      std::function<void(double, double)> callback) override;

  vtkSmartPointer<vtkImageData> convertFrameToImageData(
    const std::shared_ptr<Frame>& frame);
private:
  // ---- 窗宽窗位交互回灌（KTD11，观察者回调）----
  // 拖拽开始：从 imageProperty 捕获初始窗宽窗位（计算基准）。
  void handleStartWindowLevel(vtkObject* caller,
                              unsigned long eventId,
                              void* callData);
  // 拖拽移动：style 在有观察者时把应用职责交给观察者（见
  // vtkInteractorStyleImage::WindowLevel 源码）——此处复刻其计算
  // 公式应用到 imageProperty，再回灌 ViewModel 统一广播。
  void handleWindowLevel(vtkObject* caller,
                         unsigned long eventId,
                         void* callData);

private:
   vtkSmartPointer<vtkImageActor> m_imageActor;
   vtkSmartPointer<vtkRenderer> m_renderer;
   vtkSmartPointer<vtkRenderWindow> m_renderWindow;
   vtkSmartPointer<vtkImageProperty> imageProperty;
   vtkSmartPointer<vtkInteractorStyleImage> style;
   vtkSmartPointer<vtkRenderWindowInteractor> interactor;

   // 回灌目标（由 ViewModel 构造时绑定）；null = 不回灌
   std::function<void(double, double)> m_windowLevelCallback;
   // 拖拽开始时的初始窗宽窗位（KTD11 计算基准）
   double m_initialWindowWidth = 0.0;
   double m_initialWindowCenter = 0.0;
   // 程序化回写（updateWindowLevel）期间抑制回灌（KTD11 防环第一道闸；
   // 第二道闸为 ViewModel 的值比较幂等）
   bool m_suppressWindowLevelCallback = false;
};
