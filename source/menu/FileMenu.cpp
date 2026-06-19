
#include <QAction>

#include "FileMenu.h"

#include "qaction.h"
#include "qkeysequence.h"
#include "qmenu.h"
#include "window/GUIWindow.h"

FileMenu::FileMenu(QMenuBar* menuBar, QWidget* parent)
    : QMenu(parent)
{
  initView(menuBar);
  createConnections(parent);
}

void FileMenu::initView(QMenuBar* menuBar)
{
  auto* menu = new QMenu("File", this);

  this->m_fileOpenAction = new QAction("Open", this);

  this->m_fileOpenAction->setShortcut(QKeySequence::Open);

  menu->addAction(m_fileOpenAction);
  menuBar->addMenu(menu);
}

void FileMenu::createConnections(QWidget* parent) const
{
  auto* receiver = dynamic_cast<GUIWindow*>(parent);
  connect(
      m_fileOpenAction, &QAction::triggered, receiver, &GUIWindow::openFile);
}
