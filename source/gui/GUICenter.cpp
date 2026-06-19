
#include <QFileDialog>

#include "GUICenter.h"

#include "qdialog.h"
#include "qfiledialog.h"

GUICenter::GUICenter(QWidget* parent)
    : QWidget(parent)
{
}

void GUICenter::onOpenFile()
{
  QFileDialog fileDialog(this, "选择文件");
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted) {}
}
