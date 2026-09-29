
#pragma once
#include <array>
#include <functional>

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>

#include "IMprRenderer.h"

class vtkImageResliceMapper;
class vtkImageSlice;
class vtkPlane;
class vtkRenderer;

// MPR 三平面渲染实现（KTD12 修订：每子视口一条
// vtkImageSlice → vtkImageResliceMapper 管线，显式 vtkPlane 切面、
// 方向矩阵感知，取代原 vtkResliceImageViewer/FourPaneViewer 范式）。
// 子视口顺序：0=轴位 1=冠状 2=矢状（R5）。
// 交互通道：滚轮 → 翻层（KTD10，样式子类覆写滚轮，修饰键让行为相机
// 推进；仅轴位 pane 回灌 2D 层索引）；左键拖拽 → 窗宽窗位
// （KTD11，回灌 ViewModel）。
// 与旧实现一致：无可见十字线（轴对齐模式 cursor 本就禁用）、无跨平面
// 切面位置联动。
class VtkMprRenderer : public IMprRenderer
{
public:
  explicit VtkMprRenderer();
  ~VtkMprRenderer() override;

public:
  void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window,
                       int plane) override;
  void setVolume(vtkSmartPointer<vtkImageData> volume) override;
  void updateWindowLevel(double windowWidth, double windowCenter) override;
  void reset() override;
  void render() override;
  void setSlicePosition(int plane, double position) override;
  void setWindowLevelCallback(
      std::function<void(double, double)> callback) override;
  void setSliceIndexCallback(std::function<void(double)> callback) override;

private:
  // 把权威切片状态 m_sliceIndex[plane] 写入该平面的 vtkPlane
  // （origin = 该体素的物理点，normal = 方向矩阵对应列并归一化），
  // 已绑定时同步重置相机裁剪面。plane 的 MTime 变更即触发 mapper
  // 重切，是切面状态的唯一写入口。
  void applySlice(int plane);

  // 把缓存的窗宽窗位应用到指定子视口的 vtkImageProperty
  // （三平面同值幂等；无输入亦安全）。
  void applyWindowLevel(int plane);

  // 只重绘已绑定的子视口（未绑定 no-op）。
  void renderPlane(int plane);

  // 方向矩阵列 → 相机 Position/ViewUp（复刻 vtkImageViewer2::
  // UpdateOrientation 语义，但按数据轴映射到患者空间）。
  void updateCameraOrientation(int plane);

  // ---- 交互事件处理（观察者回调，成员方法经 vtk 模板 AddObserver
  //      挂接在 style 上；观察者随本对象成员（style）同生命周期销毁）----
  // 拖拽开始：从该子视口的 vtkImageProperty 捕获初始窗宽窗位
  // （KTD11 基准值）。
  void handleStartWindowLevel(vtkObject* caller,
                              unsigned long eventId,
                              void* callData);
  // 拖拽移动：style 在有观察者时把应用职责交给观察者（见
  // vtkInteractorStyleImage::WindowLevel）——此处复刻其计算公式，
  // 经 updateWindowLevel 应用到三联后回灌 ViewModel（KTD11）。
  void handleWindowLevel(vtkObject* caller,
                         unsigned long eventId,
                         void* callData);
  // 滚轮翻层（MprImageStyle 覆写滚轮后经 wheelHandler 转入）：
  // 边界夹取、同值早退；仅轴位 pane 携带体素 z 回灌 → 2D 跳层，
  // 冠状/矢状的切面是 y/x，不影响 2D（M3）。
  void handleSliceWheel(int plane, int sign);

  // 由触发事件的 style 反查子视口编号（-1 = 未找到）。
  int planeOfStyle(const vtkObject* caller) const;

private:
  static constexpr int kPlaneCount = 3;
  // 每子视口的渲染管线：renderer（黑底 + 平行投影）持有 imageSlice，
  // imageSlice 的 mapper 为 resliceMapper（SliceFacesCamera/
  // SliceAtFocalPoint 默认关闭 → 走显式切面 m_planes[plane]）
  std::array<vtkSmartPointer<vtkRenderer>, kPlaneCount> m_renderers;
  std::array<vtkSmartPointer<vtkImageSlice>, kPlaneCount> m_slices;
  std::array<vtkSmartPointer<vtkImageResliceMapper>, kPlaneCount> m_mappers;
  // mapper 引用的切面对象（world 坐标，随 m_sliceIndex 更新）
  std::array<vtkSmartPointer<vtkPlane>, kPlaneCount> m_planes;
  // 各子视口的渲染目标（nullptr = 未绑定；m_windows[plane] 即绑定
  // 判定，不另设标志）
  std::array<vtkSmartPointer<vtkRenderWindow>, kPlaneCount> m_windows;
  // 每子视口一个交互样式（MprImageStyle 子类：滚轮=翻层、左键=
  // 窗宽窗位回灌、中键平移/右键缩放；换窗经 SetInteractorStyle
  // 自动装卸，无挂接簿记）
  std::array<vtkSmartPointer<vtkInteractorStyleImage>, kPlaneCount> m_styles;
  // 各子视口当前切片索引（整数权威状态，取值限定在该平面数据轴的
  // extent 范围内；applySlice 是唯一写入口）
  std::array<int, kPlaneCount> m_sliceIndex {0, 0, 0};

  vtkSmartPointer<vtkImageData> m_volume;
  double m_windowWidth = 0.0;
  double m_windowCenter = 0.0;
  bool m_hasWindowLevel = false;

  // ---- U6 联动状态 ----
  std::function<void(double, double)> m_windowLevelCallback;
  std::function<void(double)> m_sliceIndexCallback;
  // 拖拽开始时该子视口的初始窗宽窗位（KTD11 计算基准）
  double m_initialWindowWidth = 0.0;
  double m_initialWindowCenter = 0.0;
  // 程序化回写期间抑制交互回灌（KTD11 防环的第一道闸；
  // 第二道闸为 ViewModel 的值比较幂等）
  bool m_suppressWindowLevelCallback = false;
};
