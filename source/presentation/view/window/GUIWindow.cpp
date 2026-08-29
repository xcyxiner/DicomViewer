
#include <QMenuBar>
#include <QVBoxLayout>

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

void GUIWindow::showEvent(QShowEvent* event)
{
  static bool firstShow = true;
  if (firstShow) {
    firstShow = false;
    dynamic_cast<GUICenter*>(m_childWidget)->showEvent();
  }
}

GUIWindow::~GUIWindow() = default;

void GUIWindow::setContent(QWidget* t_widget)
{
  ui->layout->addWidget(t_widget);
  m_childWidget = t_widget;
}

void GUIWindow::createMenu()
{
  this->m_fileMenu = new FileMenu(this->ui->menubar, this);
}

void GUIWindow::openFile()
{
  dynamic_cast<GUICenter*>(m_childWidget)->onOpenFile();
}
