
#pragma once
#include "IImageRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkImageActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkImageProperty.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkInteractorStyleImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImageMapper3D.h"
class VtkRenderer : public IImageRenderer
{
public:
  explicit VtkRenderer();
  ~VtkRenderer() = default;

public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window) override;  // 设置渲染目标
  void render(const IFrameCache::FramePtr& frame,
              const DisplaySettings& settings) override;
  void reset() override;

  void CompareImageData(vtkImageData* img1, vtkImageData* img2);
  vtkSmartPointer<vtkImageData> convertFrameToImageData(
    const std::shared_ptr<Frame>& frame);
private:
   vtkSmartPointer<vtkImageActor> m_imageActor;
   vtkSmartPointer<vtkRenderer> m_renderer;
   vtkSmartPointer<vtkRenderWindow> m_renderWindow;
   vtkSmartPointer<vtkImageProperty> imageProperty;
   vtkSmartPointer<vtkImageMapToWindowLevelColors> m_windowLevelColors;
   vtkSmartPointer<vtkInteractorStyleImage> style;
   vtkSmartPointer<vtkRenderWindowInteractor> interactor;
};
