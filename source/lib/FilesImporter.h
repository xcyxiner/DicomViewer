
#pragma once
#include <QThread>

#include "qobject.h"

class FilesImporter : public QThread
{
  Q_OBJECT
public:
  explicit FilesImporter(QObject* parent = nullptr);
};
