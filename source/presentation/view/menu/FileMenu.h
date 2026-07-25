
#pragma once
#include <QMenu>
#include <QMenuBar>

#include "qaction.h"
#include "qmenubar.h"
#include "qtmetamacros.h"
#include "qwidget.h"
namespace Ui {
    class FileMenu;
} 
class FileMenu : public QMenu
{
  Q_OBJECT
public:
  explicit FileMenu(QMenuBar* menuBar, QWidget* parent = nullptr);

signals:
  void openFile();

private:
  void createConnections(QWidget* parent) const;
  Ui::FileMenu* ui;
};
