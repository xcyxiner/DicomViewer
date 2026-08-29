
#include <QFileDialog>
#include <memory>

#include "GUICenter.h"

#include "ui_GUICenter.h"

GUICenter::GUICenter(SeriesViewModel* viewModel, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::GUICenter)
    , m_seriesViewModel(viewModel)
{
  ui->setupUi(this);
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
}
