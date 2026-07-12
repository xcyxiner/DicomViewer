
#include <QFileDialog>
#include <memory>

#include "GUICenter.h"


#include "qdialog.h"
#include "qfiledialog.h"
#include "qtmetamacros.h"

GUICenter::GUICenter(QWidget* parent)
    : QWidget(parent)
{

}

GUICenter::~GUICenter()
{
 
}

void GUICenter::onOpenFile()
{
  QFileDialog fileDialog(this, "选择文件");
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted) {
    emit this->addFiles(fileDialog.selectedFiles());
  }
}
