
#pragma once
#include <QStackedLayout>
#include <QString>
#include <memory>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>

#include "infrastructure/rendering/IMprRenderer.h"
#include "presentation/view/IViewPanel.h"
#include "presentation/viewmodels/SeriesViewModel.h"

class QLabel;

// MPR 单平面面板：一个实例只占网格一个槽位、只显示一个平面
// （轴/冠/矢由构造参数 plane 决定）。main.cpp 构造三个实例注册进网格
// —— 2x2 布局下三格分占（拆分自原三联单面板，R5/KTD12 的三平面
// 显示改为"三面板 + 三槽位"表达）。三面板共享同一 IMprRenderer
// （组合根注入），十字线、切面同步与窗宽窗位联动仍由 renderer 单源
// 维护；无体时显示占位。
// 惰性投递（seriesLoaded 未激活只标脏）、activate 补推（KTD8）与
// loadFiles no-op（KTD7）语义与原三联实现一致。
class MprPanel : public IViewPanel
{
public:
  MprPanel(SeriesViewModel* viewModel,
           std::shared_ptr<IMprRenderer> renderer,
           int plane,
           QWidget* parent = nullptr);
  ~MprPanel() override;

  // IViewPanel 接口
  QString viewName() const override { return m_viewName; }

  ViewRole viewRole() const override { return ViewRole::Mpr; }

  // KTD8：refreshLayout 对可见面板补调 —— 注入本平面的渲染目标并补推
  // 缓存的 volume/WW-WC 状态（幂等），保证 R6 两种顺序结果一致。
  void activate() override;
  void fitToWindow() override;
  void setWindowLevel(double windowWidth, double windowCenter) override;
  void resetWindowLevel() override;

  // loadFiles 不覆写：基类 IViewPanel 默认即 no-op（KTD7，见类注释）

  // 测试观察点：是否已取得可用体（false = 占位态；单层体 z 向不足
  // 2 层、冠/矢面退化，同样视为占位）
  bool hasVolume() const { return m_cachedVolume != nullptr; }

private:
  // seriesLoaded → 惰性投递：未激活（从未被菜单切出）只标脏不取体；
  // 激活后经取体入口按 seriesKey 取体（view 不直读 IFrameCache）
  // → applyVolume。
  void onSeriesLoaded();
  // 取体 + isMprUsable 门控 + 推送/占位的共用主体（onSeriesLoaded
  // 实时路径与 activate 惰性补取路径均走这里）
  void applyVolume();

private:
  SeriesViewModel* m_viewModel;
  std::shared_ptr<IMprRenderer> m_renderer;
  int m_plane;  // 本面板负责的平面（IMprRenderer::kPlane*）
  QString m_viewName;  // 平面显示名（分配视图菜单列出）

  QStackedLayout* m_stack = nullptr;  // 0=平面视口 1=占位
  QVTKOpenGLNativeWidget* m_planeWidget = nullptr;
  // 本平面视口的 render window（KTD8：面板持有，一 widget 对应一
  // window，勿复用）
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  QLabel* m_placeholder = nullptr;

  vtkSmartPointer<vtkImageData> m_cachedVolume;  // activate 补推用
  // 惰性投递状态：m_activated = activate 已被调用过（面板切出过，
  // 此后 seriesLoaded 实时推送）；m_dirty = 未激活期间到达过
  // seriesLoaded，activate 时需补取体
  bool m_activated = false;
  bool m_dirty = false;
  double m_windowWidth = 0.0;
  double m_windowCenter = 0.0;
  bool m_hasWindowLevel = false;
};
