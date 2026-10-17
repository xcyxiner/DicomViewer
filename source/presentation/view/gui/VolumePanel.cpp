
#include <QHBoxLayout>
#include <QLabel>

#include "VolumePanel.h"

#include "infrastructure/rendering/VolumeUsability.h"

namespace
{

// 显示名（分配视图菜单据此逐格列出，须与 2D/MPR 三平面互异）
QString nameForVolume()
{
  return QStringLiteral("3D 体绘制");
}

}  // namespace

VolumePanel::VolumePanel(SeriesViewModel* viewModel,
                         std::shared_ptr<IVolumeRenderer> renderer,
                         QWidget* parent)
    : IViewPanel(parent)
    , m_viewModel(viewModel)
    , m_renderer(std::move(renderer))
    , m_viewName(nameForVolume())
{
  // 单视口。面板持自己的 vtkGenericOpenGLRenderWindow（KTD8：
  // 一 widget 对应一 window，勿复用）。
  auto* panes = new QWidget(this);
  auto* row = new QHBoxLayout(panes);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(2);
  m_volumeWidget = new QVTKOpenGLNativeWidget(panes);
  m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  m_volumeWidget->setRenderWindow(m_renderWindow);
  row->addWidget(m_volumeWidget);

  // 占位（无体/加载失败态，不崩溃）
  m_placeholder = new QLabel(QStringLiteral("无序列数据"), this);
  m_placeholder->setObjectName(QStringLiteral("volumePlaceholder"));
  m_placeholder->setAlignment(Qt::AlignCenter);
  m_placeholder->setStyleSheet(QStringLiteral(
      "background-color: #2b2b2b; color: #888888; " "font-size: 14px;"));

  m_stack = new QStackedLayout(this);
  m_stack->setObjectName(QStringLiteral("volumeStack"));
  m_stack->addWidget(panes);  // index 0: 3D 视口
  m_stack->addWidget(m_placeholder);  // index 1: 占位
  m_stack->setCurrentIndex(1);  // 初始占位态（KTD7：有体后切换）

  // seriesLoaded → 惰性投递（默认 2D、菜单手动切出）：未激活只标脏，
  // 激活后经取体入口按 seriesKey 取体（KTD8：view 不直读
  // IFrameCache），可用体（z 向 ≥2 层）推给 renderer 并切实视口；
  // 空体或单层体保持占位。（语义同 MprPanel）
  connect(m_viewModel,
          &SeriesViewModel::seriesLoaded,
          this,
          [this](const QString&) { onSeriesLoaded(); });

  // 初始/联动窗宽窗位广播（KTD11：seriesLoaded 后 ViewModel 立即广播
  // 一次初始 WW/WC；菜单/交互修改也走此通道，最后操作为准）→ 更新
  // 传输函数。3D 内无拖拽交互，无回灌回调。
  connect(m_viewModel,
          &SeriesViewModel::windowLevelChanged,
          this,
          [this](double windowWidth, double windowCenter)
          {
            m_windowWidth = windowWidth;
            m_windowCenter = windowCenter;
            m_hasWindowLevel = true;
            m_renderer->updateWindowLevel(windowWidth, windowCenter);
          });
}

VolumePanel::~VolumePanel() = default;

void VolumePanel::onSeriesLoaded()
{
  // 惰性投递：从未被菜单切出过的面板只标脏——隐藏态不取体、不跑
  // renderer 流水线、不持有旧体，activate 时再补取（applyVolume）
  if (!m_activated) {
    m_cachedVolume = nullptr;  // 旧体不得跨序列驻留
    m_dirty = true;
    m_stack->setCurrentIndex(1);  // 占位（不可见，防御性归位）
    return;
  }
  applyVolume();
}

void VolumePanel::applyVolume()
{
  auto volume = m_viewModel->getVolume();
  const bool usable = isVolumeUsable(volume);
  if (usable) {
    m_cachedVolume = volume;
    m_renderer->setVolume(volume);
    m_stack->setCurrentIndex(0);  // 3D 视口
  } else {
    m_cachedVolume = nullptr;  // 换序列后旧体不得残留在 activate 补推路径
    // 单层体不进视口（体绘制退化为单张切片），提示区分两种占位因
    m_placeholder->setText(volume ? QStringLiteral("序列层数不足，无法 3D")
                                  : QStringLiteral("无序列数据"));
    m_stack->setCurrentIndex(1);  // 占位（取体为空或不足两层）
  }
}

void VolumePanel::activate()
{
  // KTD8：refreshLayout 对可见/上下文重建的面板补调 —— 注入渲染目标
  // （幂等），并补推 volume/WW-WC 状态，保证"先开序列后切布局"与
  // "先切布局后开序列"两顺序结果一致。
  m_activated = true;  // 惰性投递终点：此后 seriesLoaded 走实时推送
  m_renderer->setRenderTarget(m_renderWindow);
  if (m_dirty) {
    // 惰性补取：未激活期间到达过 seriesLoaded，渲染目标已注入，
    // 此处取体并应用（可用→推送+切实视口；不可用→占位）
    m_dirty = false;
    applyVolume();
  } else if (hasVolume()) {
    m_renderer->setVolume(m_cachedVolume);
  }
  if (m_hasWindowLevel) {
    m_renderer->updateWindowLevel(m_windowWidth, m_windowCenter);
  }
  m_renderer->render();
}

void VolumePanel::fitToWindow()
{
  if (hasVolume()) {
    m_renderer->reset();
  }
}

void VolumePanel::setWindowLevel(double windowWidth, double windowCenter)
{
  // 菜单路径（focusedPanel → 本面板）：转发 ViewModel 统一广播链
  // （KTD11，最后操作为准）——广播回来时经 windowLevelChanged 槽
  // 更新本地缓存并写 renderer，此处不再直写
  m_viewModel->setWindowLevel(windowWidth, windowCenter);
}

void VolumePanel::resetWindowLevel()
{
  // 同上：初始值恢复也走 ViewModel 广播链（初始值由其加载时捕获）
  m_viewModel->resetWindowLevel();
}
