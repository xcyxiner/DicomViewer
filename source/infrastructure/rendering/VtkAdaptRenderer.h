
#pragma once
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

  vtkSmartPointer<vtkImageData> convertFrameToImageData(
    const std::shared_ptr<Frame>& frame);
private:
   vtkSmartPointer<vtkImageActor> m_imageActor;
   vtkSmartPointer<vtkRenderer> m_renderer;
   vtkSmartPointer<vtkRenderWindow> m_renderWindow;
   vtkSmartPointer<vtkImageProperty> imageProperty;
   vtkSmartPointer<vtkInteractorStyleImage> style;
   vtkSmartPointer<vtkRenderWindowInteractor> interactor;
};
