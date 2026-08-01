#include "VtkAdaptRenderer.h"

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkImageActor.h>
#include <vtkImageFlip.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkMatrix3x3.h>
#include <vtkPolyDataMapper.h>

VtkAdaptRenderer::VtkAdaptRenderer()
{
  m_imageActor = vtkSmartPointer<vtkImageActor>::New();
  m_renderer = vtkSmartPointer<vtkRenderer>::New();
  m_renderer->AddActor(m_imageActor);
  imageProperty = vtkSmartPointer<vtkImageProperty>::New();
  style = vtkSmartPointer<vtkInteractorStyleImage>::New();
  m_imageActor->SetProperty(imageProperty);
  m_windowLevelColors = vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
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
        // m_windowLevelColors->SetInputData(imageData);
        // m_imageActor->GetMapper()->SetInputConnection(
        //     m_windowLevelColors->GetOutputPort());
        // m_windowLevelColors->SetWindow(settings.getWindowWidth());
        // m_windowLevelColors->SetLevel(settings.getWindowCenter());
        // m_windowLevelColors->Update();
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

void VtkAdaptRenderer::reset() {}

void VtkAdaptRenderer::CompareImageData(vtkImageData* img1, vtkImageData* img2)
{
  // 检查维度和数据类型
  int dims1[3], dims2[3];
  img1->GetDimensions(dims1);
  img2->GetDimensions(dims2);
  if (dims1[0] != dims2[0] || dims1[1] != dims2[1] || dims1[2] != dims2[2]) {
    std::cerr << "Dimensions do not match!" << std::endl;
    return;
  }
  int scalarType1 = img1->GetScalarType();
  int scalarType2 = img2->GetScalarType();
  if (scalarType1 != scalarType2) {
    std::cerr << "Scalar types differ!" << std::endl;
    return;
  }
  // 获取像素指针（假设均为 unsigned short）
  auto* ptr1 = static_cast<unsigned short*>(img1->GetScalarPointer());
  auto* ptr2 = static_cast<unsigned short*>(img2->GetScalarPointer());
  int totalPixels = dims1[0] * dims1[1] * dims1[2];
  // 统计差异
  double sumDiff = 0.0;
  double maxDiff = 0.0;
  int diffCount = 0;
  int firstDiffIdx = -1;
  unsigned short firstVal1 = 0, firstVal2 = 0;
  for (int i = 0; i < totalPixels; ++i) {
    unsigned short v1 = ptr1[i];
    unsigned short v2 = ptr2[i];
    double diff = std::abs(static_cast<double>(v1) - static_cast<double>(v2));
    if (diff >= 0.0) {
      sumDiff += diff;
      if (diff > maxDiff) {
        maxDiff = diff;
      }
      diffCount++;
      if (firstDiffIdx == -1) {
        firstDiffIdx = i;
        firstVal1 = v1;
        firstVal2 = v2;
      }
    }
  }
  double meanDiff = (diffCount > 0) ? (sumDiff / diffCount) : 0.0;
  std::cout << "=== Pixel Comparison ===" << std::endl;
  std::cout << "Total pixels: " << totalPixels << std::endl;
  std::cout << "Number of differing pixels: " << diffCount << std::endl;
  std::cout << "Mean absolute difference: " << meanDiff << std::endl;
  std::cout << "Max absolute difference: " << maxDiff << std::endl;
  if (firstDiffIdx != -1) {
    std::cout << "First diff at linear index " << firstDiffIdx << " (row "
              << firstDiffIdx / dims1[0] << ", col " << firstDiffIdx % dims1[0]
              << ")" << ": img1=" << firstVal1 << ", img2=" << firstVal2
              << std::endl;
  } else {
    std::cout << "All pixels are identical (within numeric precision)."
              << std::endl;
  }
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
