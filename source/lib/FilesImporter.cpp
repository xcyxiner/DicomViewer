
#include <cstdlib>
#include <mutex>

#include "FilesImporter.h"

#include "qglobal.h"
#include "qobject.h"

FilesImporter::FilesImporter(QObject* parent)
    : QThread(Q_NULLPTR)
{
}

void FilesImporter::startImporter()
{
  m_isWorking = true;
  start();
}

void FilesImporter::stopImporter()
{
  if (!m_isWorking) {
    return;
  }
  m_isWorking = false;
  quit();
  wait();
  m_filesPaths.clear();
}

void FilesImporter::run()
{
  while (m_isWorking) {
    importFiles();
  }
}

void FilesImporter::addFiles(const QStringList& t_paths)
{
  std::lock_guard<QMutex> lock(m_filesMutex);
  for (const auto& path : t_paths) {
    m_filesPaths.push_back(path);
  }
}

void FilesImporter::importFiles() {}
