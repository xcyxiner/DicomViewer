
#include <cstdlib>
#include <memory>
#include <mutex>

#include "FilesImporter.h"

#include "core/controller/CoreController.h"
#include "qglobal.h"
#include "qobject.h"

FilesImporter::FilesImporter(QObject* parent)
    : QThread(Q_NULLPTR)
{
  m_coreController = std::make_unique<CoreController>();
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

void FilesImporter::importFiles()
{
  while (!m_filesPaths.empty() && m_isWorking) {
    std::lock_guard<QMutex> lock(m_filesMutex);
    m_coreController->readData(m_filesPaths.front().toStdString());
    m_filesPaths.pop_front();
  }
}
