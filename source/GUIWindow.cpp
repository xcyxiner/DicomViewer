
#include <QMenuBar>

#include "GUIWindow.h"

#include "FileMenu.h"
#include "GUICenter.h"

GUIWindow::GUIWindow(QWidget* parent)
    : QMainWindow(parent)
{
  resize(1316, 805);
  createMenu();
}

void GUIWindow::setContent(QWidget* t_widget)
{
  this->m_childWidget = t_widget;
  this->setCentralWidget(this->m_childWidget);
}

void GUIWindow::createMenu()
{
  this->m_fileMenu = new FileMenu(this->menuBar(), this);
}

void GUIWindow::openFile()
{
  dynamic_cast<GUICenter*>(m_childWidget)->onOpenFile();
}
