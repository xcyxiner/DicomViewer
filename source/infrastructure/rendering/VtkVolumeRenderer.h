
#pragma once
#include <functional>

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include "IVolumeRenderer.h"

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkRenderer;
class vtkSmartVolumeMapper;
class vtkVolume;

// 3D 体绘制渲染实现：vtkSmartVolumeMapper 光线投射 + 传输函数
// （颜色/不透明度）表达窗宽窗位。交互为标准 trackball（左键旋转、
// 中键平移、滚轮缩放），无 WW/WC 拖拽（KTD11 回灌链不涉及 3D）。
// WW/WC → TF 映射单源在 applyTransferFunction；黑底，与 2D/MPR 一致。
class VtkVolumeRenderer : public IVolumeRenderer
{
public:
  explicit VtkVolumeRenderer();
  ~VtkVolumeRenderer() override;

public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window) override;
  void setVolume(vtkSmartPointer<vtkImageData> volume) override;
  void updateWindowLevel(double windowWidth, double windowCenter) override;
  void reset() override;
  void render() override;

private:
  // WW/WC（或无缓存时体标量范围默认值）→ 颜色/不透明度传输函数。
  // 有体才生效；点序由 ww>0 守卫保证，窗外靠 TF 端点钳制（数据不
  // 超出标量范围，端点外无样本）。
  void applyTransferFunction();

  // 方向矩阵列 → 相机朝向（前-左-上斜视角，viewUp = 第三列；focal
  // 先置原点，随后 ResetCamera 移到包围盒中心——手法仿
  // VtkMprRenderer::updateCameraOrientation）。
  void updateCameraOrientation();

private:
  vtkSmartPointer<vtkRenderer> m_renderer;
  vtkSmartPointer<vtkVolume> m_volumeActor;
  vtkSmartPointer<vtkSmartVolumeMapper> m_mapper;
  vtkSmartPointer<vtkColorTransferFunction> m_colorTF;
  vtkSmartPointer<vtkPiecewiseFunction> m_opacityTF;
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_style;

  vtkSmartPointer<vtkRenderWindow> m_window;  // nullptr = 未绑定
  vtkSmartPointer<vtkImageData> m_volume;

  double m_windowWidth = 0.0;
  double m_windowCenter = 0.0;
  bool m_hasWindowLevel = false;
};
