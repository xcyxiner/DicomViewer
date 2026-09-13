
#include <QResizeEvent>
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

  ui->layout->addWidget(m_vtkWidget);
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

GUICenter::~GUICenter() {}

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
