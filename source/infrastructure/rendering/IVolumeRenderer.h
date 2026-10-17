
#pragma once
#include <vtkImageData.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

// 3D 体绘制渲染器接口（与 2D 的 IImageRenderer、MPR 的 IMprRenderer
// 并列的第三条渲染通道）。由 main.cpp 组合根构造具体实现并注入
// VolumePanel（面板不自建实现）。基类方法带默认空实现，便于测试假件
// 替换。契约：
// - setRenderTarget 幂等：同一 window 重复注入直接返回（保用户旋转）；
// - 无体/无窗时 renderer 侧 no-op 不崩溃（占位由面板层处理）；
// - updateWindowLevel 同值早退；WW/WC 单向（ViewModel 广播 → 面板 →
//   renderer 更新传输函数），3D 内无拖拽交互、无回灌回调；
// - reset 为 fit 语义（ResetCamera + 重绘），不重申相机朝向
//   （trackball 下用户旋转应被保留）。
class IVolumeRenderer
{
public:
  IVolumeRenderer();
  virtual ~IVolumeRenderer() = default;

  // 注入渲染目标（VolumePanel 持有自己的 QVTKOpenGLNativeWidget 的
  // vtkGenericOpenGLRenderWindow）。换窗时重挂 renderer 并装配
  // trackball 交互样式（旋转/平移/滚轮缩放）。
  virtual void setRenderTarget(vtkSmartPointer<vtkRenderWindow> window);

  // 设置整幅体（唯一像素源）。无体 no-op；同指针仅补绘；
  // 新体写入 mapper 并按当前 WW/WC（或体标量范围默认值）构建
  // 传输函数。
  virtual void setVolume(vtkSmartPointer<vtkImageData> volume);

  // 窗宽窗位（程序化写入，来自 ViewModel 广播）：按
  // [wc-ww/2, wc+ww/2] 重建颜色/不透明度传输函数后重绘。
  virtual void updateWindowLevel(double windowWidth, double windowCenter);

  // 重置相机并重绘（fit 语义，面板 fitToWindow 转发）。
  virtual void reset();

  // 重绘（GL 上下文重建/状态变更后调用，KTD8）。
  virtual void render();
};
