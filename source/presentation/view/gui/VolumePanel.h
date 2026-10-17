
#pragma once
#include <QStackedLayout>
#include <QString>
#include <memory>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>

#include "infrastructure/rendering/IVolumeRenderer.h"
#include "presentation/view/IViewPanel.h"
#include "presentation/viewmodels/SeriesViewModel.h"

class QLabel;

// 3D 体绘制面板：占网格一个槽位（Mpr3d 模式下默认槽 3）。组合根注入
// IVolumeRenderer，面板不自建实现。生命周期逐项对齐 MprPanel：
// 惰性投递（seriesLoaded 未激活只标脏）、activate 补推（KTD8）、
// loadFiles no-op（KTD7）、WW/WC 走 ViewModel 广播链（KTD11）。
// 与 MPR 的差异：无切面联动（不订 sliceChanged、无回灌回调）、
// 交互为 trackball 旋转缩放（renderer 内置）。
class VolumePanel : public IViewPanel
{
public:
  VolumePanel(SeriesViewModel* viewModel,
              std::shared_ptr<IVolumeRenderer> renderer,
              QWidget* parent = nullptr);
  ~VolumePanel() override;

  // IViewPanel 接口
  QString viewName() const override { return m_viewName; }

  ViewRole viewRole() const override { return ViewRole::Volume; }

  // KTD8：refreshLayout 对可见面板补调 —— 注入渲染目标并补推缓存的
  // volume/WW-WC 状态（幂等），保证加载与布局切换两顺序结果一致。
  void activate() override;
  void fitToWindow() override;
  void setWindowLevel(double windowWidth, double windowCenter) override;
  void resetWindowLevel() override;

  // loadFiles 不覆写：基类 IViewPanel 默认即 no-op（KTD7）

  // 测试观察点：是否已取得可用体（false = 占位态；单层体 z 向不足
  // 2 层无法体绘制，同样视为占位）
  bool hasVolume() const { return m_cachedVolume != nullptr; }

private:
  // seriesLoaded → 惰性投递：未激活只标脏不取体；激活后经取体入口
  // 按 seriesKey 取体（view 不直读 IFrameCache）→ applyVolume。
  void onSeriesLoaded();
  // 取体 + isVolumeUsable 门控 + 推送/占位的共用主体
  void applyVolume();

private:
  SeriesViewModel* m_viewModel;
  std::shared_ptr<IVolumeRenderer> m_renderer;
  QString m_viewName;  // 显示名（分配视图菜单列出，须与其余面板互异）

  QStackedLayout* m_stack = nullptr;  // 0=3D 视口 1=占位
  QVTKOpenGLNativeWidget* m_volumeWidget = nullptr;
  // 本视口的 render window（KTD8：面板持有，一 widget 对应一 window）
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  QLabel* m_placeholder = nullptr;

  vtkSmartPointer<vtkImageData> m_cachedVolume;  // activate 补推用
  // 惰性投递状态（语义同 MprPanel）
  bool m_activated = false;
  bool m_dirty = false;
  double m_windowWidth = 0.0;
  double m_windowCenter = 0.0;
  bool m_hasWindowLevel = false;
};
