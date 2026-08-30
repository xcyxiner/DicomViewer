
#pragma once
#include <QMenu>
#include <QMenuBar>

#include "qaction.h"
#include "qmenubar.h"
#include "qtmetamacros.h"
#include "qwidget.h"
namespace Ui {
    class WindowLevelMenu;
}
class WindowLevelMenu : public QMenu
{
  Q_OBJECT
public:
  explicit WindowLevelMenu(QMenuBar* menuBar, QWidget* parent = nullptr);

signals:
  void windowLevelDefault();
  void windowLevelAll();
  void windowLevelBrain();
  void windowLevelLung();
  void windowLevelBone();
  void fitToWindow();

private:
  void createConnections() const;
  Ui::WindowLevelMenu* ui;
};
