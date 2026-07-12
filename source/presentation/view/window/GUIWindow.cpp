
#include <QMenuBar>

#include "GUIWindow.h"

#include "presentation/view/gui/GUICenter.h"
#include "presentation/view/menu/FileMenu.h"
#include "ui_GUIWindow.h"

GUIWindow::GUIWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::GUIWindow>())
{
  ui->setupUi(this);
  createMenu();
}

GUIWindow::~GUIWindow() = default;

void GUIWindow::setContent(QWidget* t_widget)
{
  this->m_childWidget = t_widget;
  this->ui->centralwidget = t_widget;
}

void GUIWindow::createMenu()
{
  this->m_fileMenu = new FileMenu(this->ui->menubar, this);
}

void GUIWindow::openFile()
{
  dynamic_cast<GUICenter*>(m_childWidget)->onOpenFile();
}
