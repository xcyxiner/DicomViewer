
#pragma once
#include <QMenu>
#include <QMenuBar>

#include "qaction.h"
#include "qmenubar.h"
#include "qtmetamacros.h"
#include "qwidget.h"

class FileMenu : public QMenu
{
  Q_OBJECT
public:
  explicit FileMenu(QMenuBar* menuBar, QWidget* parent = nullptr);

signals:
  void openFile();

private:
  void initView(QMenuBar* menuBar);
  void createConnections(QWidget* parent) const;
  QAction* m_fileOpenAction;
};
