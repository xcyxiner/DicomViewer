
#pragma once
#include <QWidget>
#include <memory>


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
};
