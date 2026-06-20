
#pragma once
#include <QMutex>
#include <QThread>
#include <deque>
#include <memory>

#include "core/controller/CoreController.h"
#include "qglobal.h"
#include "qmutex.h"
#include "qobject.h"

class FilesImporter : public QThread
{
  Q_OBJECT
public:
  explicit FilesImporter(QObject* parent = nullptr);

public:
  void startImporter();
  void stopImporter();
  void addFiles(const QStringList& t_paths);
  void importFiles();

protected:
  void run() override;

private:
  QMutex m_filesMutex;
  bool m_isWorking = false;
  std::deque<QString> m_filesPaths;
  std::unique_ptr<CoreController> m_coreController;
};
