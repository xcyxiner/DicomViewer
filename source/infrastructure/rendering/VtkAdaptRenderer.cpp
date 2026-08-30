#include "VtkAdaptRenderer.h"

#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageFlip.h>
#include <vtkImageMapper3D.h>
#include <vtkMatrix3x3.h>

VtkAdaptRenderer::VtkAdaptRenderer()
{
  m_imageActor = vtkSmartPointer<vtkImageActor>::New();
  m_renderer = vtkSmartPointer<vtkRenderer>::New();
  m_renderer->AddActor(m_imageActor);
  m_renderer->SetBackground(0.0, 0.0, 0.0);  // 黑色背景
  imageProperty = vtkSmartPointer<vtkImageProperty>::New();
  style = vtkSmartPointer<vtkInteractorStyleImage>::New();
  m_imageActor->SetProperty(imageProperty);
}

void VtkAdaptRenderer::setRenderTarget(vtkSmartPointer<vtkRenderWindow> window)
{
  m_renderWindow = window;
  m_renderWindow->AddRenderer(m_renderer);
}

void VtkAdaptRenderer::render(const IFrameCache::FramePtr& frame,
                              const DisplaySettings& settings)
{
  std::visit(
      [this, &settings](auto&& arg)
      {
        using T = std::decay_t<decltype(arg)>;
        vtkSmartPointer<vtkImageData> imageData;
        if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
          imageData = convertFrameToImageData(arg);
        }
        if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
          imageData = arg;
        }
        if (!imageData) {
          std::cerr << "Error: imageData is null!" << std::endl;
          return;
        }
        m_imageActor->GetMapper()->SetInputData(imageData);
        imageProperty->SetColorWindow(settings.getWindowWidth());
        imageProperty->SetColorLevel(settings.getWindowCenter());
        interactor = m_renderWindow->GetInteractor();
        interactor->SetInteractorStyle(style);
        interactor->Initialize();
        m_renderWindow->Render();
      },
      frame);
}

void VtkAdaptRenderer::updateWindowLevel(double windowWidth,
                                         double windowCenter)
{
  if (!imageProperty || !m_renderWindow) {
    return;
  }
  imageProperty->SetColorWindow(windowWidth);
  imageProperty->SetColorLevel(windowCenter);
  m_renderWindow->Render();
}

void VtkAdaptRenderer::reset() {}

void VtkAdaptRenderer::fitToWindow()
{
  if (!m_renderer || !m_imageActor || !m_imageActor->GetInput()) {
    return;
  }

  m_renderer->ResetCamera();
  m_renderWindow->Render();
}

vtkSmartPointer<vtkImageData> VtkAdaptRenderer::convertFrameToImageData(
    const std::shared_ptr<Frame>& frame)
{
  auto imageData = vtkSmartPointer<vtkImageData>::New();
  imageData->SetDimensions(frame->getCols(), frame->getRows(), 1);
  imageData->SetSpacing(
      frame->getPixelSpacingX(), frame->getPixelSpacingY(), 1.0);
  imageData->SetOrigin(frame->getImagePositionPatient().data());
  vtkSmartPointer<vtkMatrix3x3> directionMatrix =
      vtkSmartPointer<vtkMatrix3x3>::New();
  double R[3][3];
  double RX = frame->getImageOrientationPatient()[0];
  double RY = frame->getImageOrientationPatient()[1];
  double RZ = frame->getImageOrientationPatient()[2];
  double CX = frame->getImageOrientationPatient()[3];
  double CY = frame->getImageOrientationPatient()[4];
  double CZ = frame->getImageOrientationPatient()[5];
  R[0][0] = RX;
  R[1][0] = RY;
  R[2][0] = RZ;
  R[0][1] = CX;
  R[1][1] = CY;
  R[2][1] = CZ;
  R[0][2] = RY * CZ - RZ * CY;
  R[1][2] = RZ * CX - RX * CZ;
  R[2][2] = RX * CY - RY * CX;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      directionMatrix->SetElement(i, j, R[i][j]);
    }
  }
  imageData->SetDirectionMatrix(directionMatrix);
  imageData->SetExtent(0, frame->getCols() - 1, 0, frame->getRows() - 1, 0, 0);
  imageData->AllocateScalars(VTK_SHORT, 1);
  int numPixels = frame->getRows() * frame->getCols();
  std::vector<int16_t> huData(numPixels);
  for (int i = 0; i < numPixels; ++i) {
    huData[i] = static_cast<int16_t>(frame->getPixels()[i] * frame->getSlope()
                                     + frame->getIntercept());
  }
  memcpy(imageData->GetScalarPointer(),
         huData.data(),
         huData.size() * sizeof(int16_t));
  imageData->Modified();
  vtkSmartPointer<vtkImageFlip> flipper = vtkSmartPointer<vtkImageFlip>::New();
  flipper->SetInputData(imageData);
  flipper->SetFilteredAxis(1);  // 1 代表 Y 轴
  flipper->Update();
  vtkSmartPointer<vtkImageData> flippedImageData = flipper->GetOutput();
  return flippedImageData;
}
