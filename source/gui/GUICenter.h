
#pragma once
#include <QWidget>
#include <memory>

#include "lib/FilesImporter.h"
#include "qglobal.h"
#include "qtmetamacros.h"

class GUICenter : public QWidget
{
  Q_OBJECT
public:
  explicit GUICenter(QWidget* parent = nullptr);
  ~GUICenter();

public:
  void onOpenFile();

signals:
  void addFiles(const QStringList& t_paths);

private:
  std::unique_ptr<FilesImporter> m_fileImporter;
};
