#include <algorithm>
#include <cmath>

#include "VtkAdaptRenderer.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkImageActor.h>
#include <vtkImageFlip.h>
#include <vtkImageMapper3D.h>
#include <vtkMatrix3x3.h>

#include "infrastructure/utils/FloatCompare.h"
#include "infrastructure/utils/WindowLevelDrag.h"

VtkAdaptRenderer::VtkAdaptRenderer()
{
  m_imageActor = vtkSmartPointer<vtkImageActor>::New();
  m_renderer = vtkSmartPointer<vtkRenderer>::New();
  m_renderer->AddActor(m_imageActor);
  m_renderer->SetBackground(0.0, 0.0, 0.0);  // 黑色背景
  imageProperty = vtkSmartPointer<vtkImageProperty>::New();
  style = vtkSmartPointer<vtkInteractorStyleImage>::New();
  m_imageActor->SetProperty(imageProperty);

  // 窗宽窗位交互回灌（KTD11）：观察者存在时 style 把应用职责交给
  // 观察者（其 WindowLevel() 的 HasObserver 分支），由本对象计算并
  // 应用后回灌 ViewModel 统一广播。观察者随 style 成员同生命周期。
  style->AddObserver(vtkCommand::StartWindowLevelEvent,
                     this,
                     &VtkAdaptRenderer::handleStartWindowLevel);
  style->AddObserver(
      vtkCommand::WindowLevelEvent, this, &VtkAdaptRenderer::handleWindowLevel);
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
  // 同值早退：拖拽链路（本对象应用 → ViewModel 广播回灌）第二道
  // 写入值相同，跳过 Set + Render 消除每帧二次重绘（E1）。GL 重建
  // 后的重绘由调用方显式 render()/fit 保障。
  if (FloatCompare::nearlyEqual(imageProperty->GetColorWindow(), windowWidth)
      && FloatCompare::nearlyEqual(imageProperty->GetColorLevel(),
                                   windowCenter))
  {
    return;
  }
  // 程序化回写期间抑制交互回灌（KTD11 第一道闸；交互事件不会在本
  // 调用栈内触发，此标志为纵深防御，第二道闸在 ViewModel 值比较）
  m_suppressWindowLevelCallback = true;
  imageProperty->SetColorWindow(windowWidth);
  imageProperty->SetColorLevel(windowCenter);
  m_renderWindow->Render();
  m_suppressWindowLevelCallback = false;
}

void VtkAdaptRenderer::setWindowLevelCallback(
    std::function<void(double, double)> callback)
{
  // 观察者在构造时已挂接；此处只登记回灌目标（null = 不回灌）
  m_windowLevelCallback = std::move(callback);
}

void VtkAdaptRenderer::handleStartWindowLevel(vtkObject*, unsigned long, void*)
{
  // KTD11 计算基准：拖拽开始时刻当前 imageProperty 的窗宽窗位
  if (!imageProperty) {
    return;
  }
  m_initialWindowWidth = imageProperty->GetColorWindow();
  m_initialWindowCenter = imageProperty->GetColorLevel();
}

void VtkAdaptRenderer::handleWindowLevel(vtkObject*, unsigned long, void*)
{
  if (m_suppressWindowLevelCallback) {
    // 程序化回写期间的回灌直接丢弃（KTD11 第一道闸）
    return;
  }
  if (!imageProperty || !m_renderWindow) {
    return;
  }
  const int* size = m_renderWindow->GetSize();
  if (size[0] <= 0 || size[1] <= 0) {
    return;
  }

  // 计算公式在 WindowLevelDrag 中单源（复刻 vtkInteractorStyleImage::
  // WindowLevel，style 在有观察者时不再自行应用，见其源码）。
  // WindowLevelStartPosition/CurrentPosition 由 style 在按钮按下与
  // 移动时维护，观察者可公开读取。
  const int* start = style->GetWindowLevelStartPosition();
  const int* current = style->GetWindowLevelCurrentPosition();
  double newWindow = 0.0;
  double newLevel = 0.0;
  WindowLevelDrag::compute(start,
                           current,
                           size,
                           m_initialWindowWidth,
                           m_initialWindowCenter,
                           newWindow,
                           newLevel);

  // 应用到 imageProperty 并重绘（程序化路径，抑制标志覆盖）
  updateWindowLevel(newWindow, newLevel);
  if (m_windowLevelCallback) {
    m_windowLevelCallback(newWindow, newLevel);
  }
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
