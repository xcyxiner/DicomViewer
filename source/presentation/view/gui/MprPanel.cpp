
#include <QHBoxLayout>
#include <QLabel>
#include <cmath>

#include "MprPanel.h"

namespace
{

// MPR 可用性：体在 z 向至少 2 层才是真正的三维体。单层体（N=1 的
// 单文件序列）轴位尚可，但冠/矢面退化为 1 像素条，无诊断意义——
// 此时进占位比进三联更诚实。多帧单文件（enhanced）z 深度 >1 不受影响。
bool isMprUsable(const vtkSmartPointer<vtkImageData>& volume)
{
  if (!volume) {
    return false;
  }
  int extent[6];
  volume->GetExtent(extent);
  return extent[5] > extent[4];
}

// 平面编号 → 面板显示名（分配视图菜单据此逐格列出，须互不相同）
QString nameForPlane(int plane)
{
  switch (plane) {
    case IMprRenderer::kPlaneAxial:
      return QStringLiteral("MPR 轴位");
    case IMprRenderer::kPlaneCoronal:
      return QStringLiteral("MPR 冠状");
    case IMprRenderer::kPlaneSagittal:
      return QStringLiteral("MPR 矢状");
    default:
      return QStringLiteral("MPR");
  }
}

}  // namespace

MprPanel::MprPanel(SeriesViewModel* viewModel,
                   std::shared_ptr<IMprRenderer> renderer,
                   int plane,
                   QWidget* parent)
    : IViewPanel(parent)
    , m_viewModel(viewModel)
    , m_renderer(std::move(renderer))
    , m_plane(plane)
    , m_viewName(nameForPlane(plane))
{
  // 单平面视口。面板持自己的 vtkGenericOpenGLRenderWindow（KTD8：
  // 一 widget 对应一 window，勿复用）；三面板共享 renderer，平面归属
  // 由 activate 的 setRenderTarget(plane) 表达。
  auto* panes = new QWidget(this);
  auto* row = new QHBoxLayout(panes);
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(2);
  m_planeWidget = new QVTKOpenGLNativeWidget(panes);
  m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  m_planeWidget->setRenderWindow(m_renderWindow);
  row->addWidget(m_planeWidget);

  // 占位（无体/加载失败态，不崩溃）
  m_placeholder = new QLabel(QStringLiteral("无序列数据"), this);
  m_placeholder->setObjectName(QStringLiteral("mprPlaceholder"));
  m_placeholder->setAlignment(Qt::AlignCenter);
  m_placeholder->setStyleSheet(QStringLiteral(
      "background-color: #2b2b2b; color: #888888; " "font-size: 14px;"));

  m_stack = new QStackedLayout(this);
  m_stack->setObjectName(QStringLiteral("mprStack"));
  m_stack->addWidget(panes);  // index 0: 平面视口
  m_stack->addWidget(m_placeholder);  // index 1: 占位
  m_stack->setCurrentIndex(1);  // 初始占位态（KTD7：有体后切换）

  // seriesLoaded → 惰性投递（默认 2D、菜单手动切出）：未激活只标脏，
  // 激活后经取体入口按 seriesKey 取体（KTD7：view 不直读
  // IFrameCache），可用体（z 向 ≥2 层）推给 renderer 并切实视口；
  // 空体或单层体保持占位。
  connect(m_viewModel,
          &SeriesViewModel::seriesLoaded,
          this,
          [this](const QString&) { onSeriesLoaded(); });

  // 初始/联动窗宽窗位广播（KTD7：seriesLoaded 后 ViewModel 立即广播
  // 一次初始 WW/WC；U6 的菜单/交互修改也走此通道，最后操作为准）。
  // 三面板同接（幂等：renderer 同值早退），各面板本地缓存都须跟上，
  // 供 activate 补推。
  connect(m_viewModel,
          &SeriesViewModel::windowLevelChanged,
          this,
          [this](double windowWidth, double windowCenter)
          {
            // 初始/联动值一律以此广播为准（初始值捕获已上移到
            // ViewModel 的 reset 通道，KTD11）
            m_windowWidth = windowWidth;
            m_windowCenter = windowCenter;
            m_hasWindowLevel = true;
            m_renderer->updateWindowLevel(windowWidth, windowCenter);
          });

  // ---- U6 联动接线 ----
  // 正向（R8）：2D 翻层 → 轴位切面同步（体素索引直映射，仅承诺轴位
  // 采集序列，M3）。只挂轴位面板：切面归属轴位平面，其余面板订阅会
  // 造成每次翻层对同一 SetSlice 的重复推送（每步多轮重绘）。
  if (m_plane == IMprRenderer::kPlaneAxial) {
    connect(m_viewModel,
            &SeriesViewModel::sliceChanged,
            this,
            [this](int index) {
              m_renderer->setSlicePosition(IMprRenderer::kPlaneAxial, index);
            });
  }

  // 反向（R8）：MPR 子视口切面（体素 z）变化 → 2D 跳层。
  // setCurrentIndex 内部夹取 [0, N-1]、同值幂等（防环）。回调是
  // renderer 全局的（只看轴位 pane），各面板登记等价转发（后建覆盖
  // 先建，语义一致，无累积成本）。
  m_renderer->setSliceIndexCallback(
      [viewModel = m_viewModel](double voxelIndex) {
        viewModel->setCurrentIndex(static_cast<int>(std::lround(voxelIndex)));
      });

  // 窗宽窗位回灌（KTD11）：面板只做一行转发，抑制/幂等/广播逻辑
  // 全部在 ViewModel——MPR 拖拽与 2D 拖拽、菜单共用同一广播链。
  m_renderer->setWindowLevelCallback(
      [viewModel = m_viewModel](double windowWidth, double windowCenter)
      { viewModel->notifyWindowLevelInteracted(windowWidth, windowCenter); });
}

MprPanel::~MprPanel() = default;

void MprPanel::onSeriesLoaded()
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

void MprPanel::applyVolume()
{
  auto volume = m_viewModel->getVolume();
  const bool usable = isMprUsable(volume);
  if (usable) {
    m_cachedVolume = volume;
    m_renderer->setVolume(volume);
    m_stack->setCurrentIndex(0);  // 平面视口
  } else {
    m_cachedVolume = nullptr;  // 换序列后旧体不得残留在 activate 补推路径
    // 单层体不进视口（冠/矢面退化为 1 像素条），提示区分两种占位因
    m_placeholder->setText(volume ? QStringLiteral("序列层数不足，无法 MPR")
                                  : QStringLiteral("无序列数据"));
    m_stack->setCurrentIndex(1);  // 占位（取体为空或不足两层）
  }
  // 联动重置（R8 / U6 场景4）：新序列轴位切面同步到 2D 当前层
  // （加载后为第 0 层）——否则 setVolume 的默认居中会停在体中间。
  // 只由轴位面板执行：其余面板的视口与轴位切面无关，且轴位面板未
  // 激活时该写入本就被 renderer 的未绑定守卫跳过，其 activate 补推
  // 覆盖该缺口（无体守卫见 VtkMprRenderer::setSlicePosition）。
  if (m_plane == IMprRenderer::kPlaneAxial) {
    m_renderer->setSlicePosition(IMprRenderer::kPlaneAxial,
                                 m_viewModel->currentIndex());
  }
}

void MprPanel::activate()
{
  // KTD8：refreshLayout 对可见/上下文重建的面板补调 —— 注入本平面
  // 渲染目标（幂等），并补推 volume/WW-WC 状态，保证 R6
  // "先开序列后切布局"与"先切布局后开序列"两顺序结果一致。
  m_activated = true;  // 惰性投递终点：此后 seriesLoaded 走实时推送
  m_renderer->setRenderTarget(m_renderWindow, m_plane);
  if (m_dirty) {
    // 惰性补取：未激活期间到达过 seriesLoaded，渲染目标已注入，
    // 此处取体并应用（可用→推送+切实视口；不可用→占位）
    m_dirty = false;
    applyVolume();
  } else if (hasVolume()) {
    m_renderer->setVolume(m_cachedVolume);
    // 上下文重建/补推切面（KTD8）：渲染目标刚注入，把当前层定位
    // 推给 renderer（seriesLoaded 早于激活时被 setSlicePosition 的
    // 未绑定守卫跳过，此处补上，保证 R6 两种顺序一致）
    if (m_plane == IMprRenderer::kPlaneAxial) {
      m_renderer->setSlicePosition(IMprRenderer::kPlaneAxial,
                                   m_viewModel->currentIndex());
    }
  }
  if (m_hasWindowLevel) {
    m_renderer->updateWindowLevel(m_windowWidth, m_windowCenter);
  }
  m_renderer->render();
}

void MprPanel::fitToWindow()
{
  if (hasVolume()) {
    m_renderer->reset();
  }
}

void MprPanel::setWindowLevel(double windowWidth, double windowCenter)
{
  // 菜单路径（focusedPanel → 本面板）：转发 ViewModel 统一广播链
  // （KTD11，最后操作为准）——广播回来时经 windowLevelChanged 槽
  // 更新本地缓存并写 renderer，此处不再直写
  m_viewModel->setWindowLevel(windowWidth, windowCenter);
}

void MprPanel::resetWindowLevel()
{
  // 同上：初始值恢复也走 ViewModel 广播链（初始值由其加载时捕获）
  m_viewModel->resetWindowLevel();
}
