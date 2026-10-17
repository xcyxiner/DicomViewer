#include <algorithm>

#include "VtkVolumeRenderer.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkMatrix3x3.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include "infrastructure/utils/FloatCompare.h"

namespace
{

// 窗口上沿不透明度（实测可调）：0=全透明 1=不透明。体绘制不同于
// 2D 的查表显示，峰值过高会让近端结构遮挡深部，0.5 作可用基线。
constexpr double kMaxOpacity = 0.5;

}  // namespace

VtkVolumeRenderer::VtkVolumeRenderer()
{
  m_renderer = vtkSmartPointer<vtkRenderer>::New();
  m_renderer->SetBackground(0.0, 0.0, 0.0);

  m_mapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
  m_colorTF = vtkSmartPointer<vtkColorTransferFunction>::New();
  m_opacityTF = vtkSmartPointer<vtkPiecewiseFunction>::New();

  m_volumeActor = vtkSmartPointer<vtkVolume>::New();
  m_volumeActor->SetMapper(m_mapper);
  auto* property = m_volumeActor->GetProperty();
  property->SetInterpolationTypeToLinear();
  property->SetColor(m_colorTF);
  property->SetScalarOpacity(m_opacityTF);
  m_renderer->AddVolume(m_volumeActor);

  // 标准 trackball：左键旋转 / 中键平移 / 滚轮 dolly 缩放
  m_style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
}

VtkVolumeRenderer::~VtkVolumeRenderer() = default;

void VtkVolumeRenderer::setRenderTarget(vtkSmartPointer<vtkRenderWindow> window)
{
  if (!window) {
    return;
  }
  if (m_window != window) {
    // 换窗（或首次注入）：重挂 renderer（RenderWindow 随之重链）
    if (m_window) {
      m_window->RemoveRenderer(m_renderer);
    }
    window->AddRenderer(m_renderer);
    m_window = window;
    // 绑定变更才恢复相机且已有体 → 朝向 + fit；同窗重入跳过，
    // 保住用户旋转（仿 VtkMprRenderer::setRenderTarget）
    if (m_volume) {
      updateCameraOrientation();
      m_renderer->ResetCamera();
    }
  }
  if (auto* interactor = window->GetInteractor()) {
    // 装配交互样式（幂等：同样式重复 SetInteractorStyle 由 vtk
    // 内部早退；换样式时旧样式的观察者自动卸除）
    interactor->SetInteractorStyle(m_style);
  }
}

void VtkVolumeRenderer::setVolume(vtkSmartPointer<vtkImageData> volume)
{
  if (!volume) {
    // 无体 no-op：占位由面板层处理，不崩溃（契约同 IMprRenderer）
    return;
  }
  if (volume == m_volume) {
    // 同一体重复推送（activate 补推）：保留 WW/WC 与相机，仅补绘
    render();
    return;
  }
  m_volume = volume;
  m_mapper->SetInputData(volume);

  // 默认 WW/WC 不写缓存（标量范围只作本次 TF 的取值来源，缓存仍只
  // 由 updateWindowLevel 写入——仿 VtkMprRenderer::setVolume 语义）
  applyTransferFunction();

  if (m_window) {
    // 新体新几何：已绑定视口重置朝向 + fit（换序列时居中新体）
    updateCameraOrientation();
    m_renderer->ResetCamera();
  }
  render();
}

void VtkVolumeRenderer::updateWindowLevel(double windowWidth,
                                          double windowCenter)
{
  // 同值早退：广播链路的重复写入跳过 TF 重建 + 重绘（E1，仿
  // VtkMprRenderer::updateWindowLevel）。无回灌回调（3D 无拖拽），
  // 无需 suppress 标志。
  if (m_hasWindowLevel && FloatCompare::nearlyEqual(m_windowWidth, windowWidth)
      && FloatCompare::nearlyEqual(m_windowCenter, windowCenter))
  {
    return;
  }
  m_windowWidth = windowWidth;
  m_windowCenter = windowCenter;
  m_hasWindowLevel = true;
  applyTransferFunction();
  render();
}

void VtkVolumeRenderer::applyTransferFunction()
{
  if (!m_volume) {
    return;
  }
  double range[2];
  m_volume->GetScalarRange(range);

  double windowWidth = m_windowWidth;
  double windowCenter = m_windowCenter;
  if (!m_hasWindowLevel) {
    // 无广播值：体标量范围作默认窗口（全范围显示）
    windowWidth = range[1] - range[0];
    windowCenter = 0.5 * (range[0] + range[1]);
  }
  if (!(windowWidth > 0.0)) {
    // 防呆：零/负窗宽（常数体或异常广播）退回最小正窗，保证 lo<hi
    windowWidth = 1.0;
  }
  const double lo = windowCenter - 0.5 * windowWidth;
  const double hi = windowCenter + 0.5 * windowWidth;

  // 颜色：窗外低值 → 黑，窗外高值 → 白（TF 端点钳制，标量范围外
  // 无样本，端点值即窗外表现——不做范围夹取，否则窗口整体落在体外
  // 会退化为全范围显示，偏离窗宽窗位语义）
  m_colorTF->RemoveAllPoints();
  m_colorTF->AddRGBPoint(lo, 0.0, 0.0, 0.0);
  m_colorTF->AddRGBPoint(hi, 1.0, 1.0, 1.0);
  // 不透明度：窗下沿透明 → 窗上沿 kMaxOpacity（窗外钳制同上）
  m_opacityTF->RemoveAllPoints();
  m_opacityTF->AddPoint(lo, 0.0);
  m_opacityTF->AddPoint(hi, kMaxOpacity);
}

void VtkVolumeRenderer::updateCameraOrientation()
{
  if (!m_volume) {
    return;
  }
  auto* camera = m_renderer->GetActiveCamera();
  auto* direction = m_volume->GetDirectionMatrix();
  double d0[3];
  double d1[3];
  double d2[3];
  for (int row = 0; row < 3; ++row) {
    d0[row] = direction->GetElement(row, 0);
    d1[row] = direction->GetElement(row, 1);
    d2[row] = direction->GetElement(row, 2);
  }
  // 前-左-上斜视角：position ∝ d0 − d1 + d2，viewUp = d2。focal 先置
  // 原点，随后 ResetCamera 移到包围盒中心（手法仿
  // VtkMprRenderer::updateCameraOrientation）
  double position[3];
  for (int row = 0; row < 3; ++row) {
    position[row] = d0[row] - d1[row] + d2[row];
  }
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetPosition(position);
  camera->SetViewUp(d2[0], d2[1], d2[2]);
}

void VtkVolumeRenderer::reset()
{
  if (!m_volume) {
    return;
  }
  // fit 语义：只 ResetCamera（保用户旋转朝向），不重申默认朝向
  m_renderer->ResetCamera();
  render();
}

void VtkVolumeRenderer::render()
{
  // 只绘已绑定且有体的视口：占位态（面板切到占位、widget 隐藏）
  // 不触碰 GL（守卫同 VtkMprRenderer::renderPlane）
  if (m_window && m_volume) {
    m_window->Render();
  }
}
