
#include <QFileDialog>
#include <memory>

#include "GUICenter.h"

#include <QVTKOpenGLNativeWidget.h>

#include "qdialog.h"
#include "qfiledialog.h"
#include "qtmetamacros.h"
#include "ui_GUICenter.h"

GUICenter::GUICenter(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::GUICenter)
{
  ui->setupUi(this);
  // initvtkTest();
  m_seriesViewModel = new SeriesViewModel(this);
  m_vtkWidget = new QVTKOpenGLNativeWidget(this);

  m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  m_vtkWidget->setRenderWindow(m_renderWindow);

  ui->layout->addWidget(m_vtkWidget);
  connect(this,
          &GUICenter::addFiles,
          m_seriesViewModel,
          [this](const QStringList& paths)
          {
            for (const auto& path : paths) {
              m_seriesViewModel->loadSeries(path.toStdString());
            }
          });
  connect(m_seriesViewModel,
          &SeriesViewModel::imageChanged,
          this,
          [this]() { m_seriesViewModel->render(); });
}

void GUICenter::initvtkTest()
{
  // 1. 创建Qt控件
  m_vtkWidget = new QVTKOpenGLNativeWidget(this);

  // 2. 创建渲染窗口和渲染器
  auto m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  renderer = vtkSmartPointer<vtkRenderer>::New();
  m_renderWindow->AddRenderer(renderer);
  m_vtkWidget->setRenderWindow(m_renderWindow);
  ui->layout->addWidget(m_vtkWidget);
}

GUICenter::~GUICenter() {}

void GUICenter::onOpenFile()
{
  QFileDialog fileDialog(this, "选择文件");
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted) {
    emit this->addFiles(fileDialog.selectedFiles());
  }
}

void GUICenter::showEvent()
{
  m_seriesViewModel->setRenderWindow(m_renderWindow);

  // auto source = vtkSmartPointer<vtkConeSource>::New();
  // auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  // mapper->SetInputConnection(source->GetOutputPort());
  // auto actor = vtkSmartPointer<vtkActor>::New();
  // actor->SetMapper(mapper);
  // renderer->AddActor(actor);
  // renderer->ResetCamera();
  // renderer->GetRenderWindow()->Render();
}
