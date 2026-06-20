
#include <QFileDialog>
#include <memory>

#include "GUICenter.h"

#include "lib/FilesImporter.h"
#include "qdialog.h"
#include "qfiledialog.h"
#include "qtmetamacros.h"

GUICenter::GUICenter(QWidget* parent)
    : QWidget(parent)
{
  m_fileImporter = std::make_unique<FilesImporter>(this);
  m_fileImporter->startImporter();
  connect(this,
          &GUICenter::addFiles,
          m_fileImporter.get(),
          &FilesImporter::addFiles);
}

GUICenter::~GUICenter()
{
  m_fileImporter->stopImporter();
}

void GUICenter::onOpenFile()
{
  QFileDialog fileDialog(this, "选择文件");
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted) {
    emit this->addFiles(fileDialog.selectedFiles());
  }
}
