
#include <QAction>

#include "WindowLevelMenu.h"

#include "qaction.h"
#include "qmenu.h"
#include "ui_WindowLevelMenu.h"

WindowLevelMenu::WindowLevelMenu(QMenuBar* menuBar, QWidget* parent)
    : QMenu(parent)
    , ui(new Ui::WindowLevelMenu)
{
  ui->setupUi(menuBar);
  createConnections();
}

void WindowLevelMenu::createConnections() const
{
  connect(ui->actionDefault,
          &QAction::triggered,
          this,
          &WindowLevelMenu::windowLevelDefault);
  connect(ui->actionAll,
          &QAction::triggered,
          this,
          &WindowLevelMenu::windowLevelAll);
  connect(ui->actionBrain,
          &QAction::triggered,
          this,
          &WindowLevelMenu::windowLevelBrain);
  connect(ui->actionLung,
          &QAction::triggered,
          this,
          &WindowLevelMenu::windowLevelLung);
  connect(ui->actionBone,
          &QAction::triggered,
          this,
          &WindowLevelMenu::windowLevelBone);
  connect(ui->actionFitToWindow,
          &QAction::triggered,
          this,
          &WindowLevelMenu::fitToWindow);
}
