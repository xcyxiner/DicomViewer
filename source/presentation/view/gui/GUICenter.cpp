
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QWheelEvent>
#include <memory>

#include "GUICenter.h"

#include "ui_GUICenter.h"

GUICenter::GUICenter(SeriesViewModel* viewModel, QWidget* parent)
    : IViewPanel(parent)
    , ui(new Ui::GUICenter)
    , m_seriesViewModel(viewModel)
{
  ui->setupUi(this);
  m_vtkWidget = new QVTKOpenGLNativeWidget(this);

  m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  m_vtkWidget->setRenderWindow(m_renderWindow);

  // 视图区 + 右侧层滑条一行，底部 n/N 层号（R4 的保底导航控件）
  m_sliceSlider = new QSlider(Qt::Vertical, this);
  m_sliceSlider->setMinimumHeight(120);
  m_sliceSlider->setEnabled(false);
  m_sliceLabel = new QLabel(QStringLiteral("–/–"), this);
  m_sliceLabel->setAlignment(Qt::AlignCenter);
  m_sliceLabel->setStyleSheet(QStringLiteral("color: #888888;"));

  auto* viewerRow = new QHBoxLayout();
  viewerRow->setContentsMargins(0, 0, 0, 0);
  viewerRow->setSpacing(2);
  viewerRow->addWidget(m_vtkWidget, 1);
  viewerRow->addWidget(m_sliceSlider, 0);
  ui->layout->addLayout(viewerRow, 1);
  ui->layout->addWidget(m_sliceLabel, 0);

  // 滚轮抢先于 VTK interactor 处理（KTD10：默认滚轮是相机推进、
  // 不翻层），阻断交给 VTK
  m_vtkWidget->installEventFilter(this);

  // 加载失败提示（R9）：应用不崩溃、已显示图像保留，仅向用户呈现
  // 失败原因。连接挂在本面板上，面板销毁自动断开。
  connect(m_seriesViewModel,
          &SeriesViewModel::loadFailed,
          this,
          [this](const QString& path, const QString& reason)
          {
            QMessageBox::warning(this,
                                 QStringLiteral("加载失败"),
                                 QStringLiteral("%1\n%2").arg(path, reason));
          });

  // 序列就绪：按帧数设滑条 range 与层号
  connect(m_seriesViewModel,
          &SeriesViewModel::seriesLoaded,
          this,
          [this](const QString&) { syncSliceControls(); });
  // 翻层：同步滑条位置与层号（值比较 + QSignalBlocker 防回环）
  connect(m_seriesViewModel,
          &SeriesViewModel::sliceChanged,
          this,
          [this](int) { syncSliceControls(); });
  // 滑条拖动 → setCurrentIndex（夹取与 sliceChanged 由 ViewModel 统一）
  connect(m_sliceSlider,
          &QSlider::valueChanged,
          this,
          [this](int value) { m_seriesViewModel->setCurrentIndex(value); });

  connect(m_seriesViewModel,
          &SeriesViewModel::imageChanged,
          this,
          [this]()
          {
            m_hasImage = true;
            m_seriesViewModel->render();
            fitToWindow();
          });
}

GUICenter::~GUICenter()
{
  delete ui;
}

void GUICenter::activate()
{
  m_seriesViewModel->setRenderWindow(m_renderWindow);
}

void GUICenter::deactivate()
{
  // 当前无操作
}

void GUICenter::loadFiles(const QStringList& paths)
{
  for (const auto& path : paths) {
    m_seriesViewModel->loadSeries(path.toStdString());
  }
}

void GUICenter::fitToWindow()
{
  if (m_hasImage && m_vtkWidget) {
    m_seriesViewModel->fitToWindow();
  }
}

void GUICenter::resetWindowLevel()
{
  if (m_hasImage) {
    m_seriesViewModel->resetWindowLevel();
  }
}

void GUICenter::setWindowLevel(double windowWidth, double windowCenter)
{
  if (m_hasImage) {
    m_seriesViewModel->setWindowLevel(windowWidth, windowCenter);
  }
}

void GUICenter::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  fitToWindow();
}

bool GUICenter::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == m_vtkWidget && event->type() == QEvent::Wheel) {
    const auto* wheel = static_cast<QWheelEvent*>(event);
    const int delta = wheel->angleDelta().y();
    if (delta == 0) {
      return false;
    }
    // 向上滚 = 上一层（索引 -1），向下滚 = 下一层；
    // N=0 / N=1 / 越界由 ViewModel 夹取为安全空操作
    if (delta > 0) {
      m_seriesViewModel->previousFrame();
    } else {
      m_seriesViewModel->nextFrame();
    }
    return true;  // 阻断 VTK interactor 的滚轮相机推进（KTD10）
  }
  return QWidget::eventFilter(watched, event);
}

void GUICenter::syncSliceControls()
{
  const int count = m_seriesViewModel->frameCount();
  const int index = m_seriesViewModel->currentIndex();
  const QSignalBlocker blocker(m_sliceSlider);
  if (count <= 0) {
    m_sliceSlider->setRange(0, 0);
    m_sliceSlider->setEnabled(false);
    m_sliceLabel->setText(QStringLiteral("–/–"));
    return;
  }
  m_sliceSlider->setRange(0, count - 1);
  m_sliceSlider->setValue(index);
  m_sliceSlider->setEnabled(count > 1);  // N=1：无层可翻，禁用滑条
  m_sliceLabel->setText(QStringLiteral("%1/%2").arg(index + 1).arg(count));
}
