
#include <QAction>

#include "FileMenu.h"

#include "qaction.h"
#include "qkeysequence.h"
#include "qmenu.h"
#include "presentation/view/window/GUIWindow.h"
#include "ui_FileMenu.h"
FileMenu::FileMenu(QMenuBar* menuBar, QWidget* parent)
    : QMenu(parent),ui(new Ui::FileMenu)
{
  ui->setupUi(menuBar);
  createConnections(parent);
}



void FileMenu::createConnections(QWidget* parent) const
{
  auto* receiver = dynamic_cast<GUIWindow*>(parent);
  connect(
      ui->actionOpen, &QAction::triggered, receiver, &GUIWindow::openFile);
}
