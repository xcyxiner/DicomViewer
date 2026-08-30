
#include <QMenuBar>
#include <QVBoxLayout>

#include "GUIWindow.h"

#include "presentation/view/gui/GUICenter.h"
#include "presentation/view/menu/FileMenu.h"
#include "presentation/view/menu/WindowLevelMenu.h"
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
  connect(m_fileMenu, &FileMenu::openFolder, this, &GUIWindow::openFolder);
  connect(m_fileMenu, &FileMenu::closeApp, this, &GUIWindow::close);

  this->m_windowLevelMenu = new WindowLevelMenu(this->ui->menubar, this);
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelDefault,
          this,
          [this]()
          { dynamic_cast<GUICenter*>(m_childWidget)->resetWindowLevel(); });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelAll,
          this,
          [this]()
          {
            // TODO: implement all window level
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelBrain,
          this,
          [this]()
          {
            // Brain: WW=80, WC=40
            dynamic_cast<GUICenter*>(m_childWidget)->setWindowLevel(80, 40);
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelLung,
          this,
          [this]()
          {
            // Lung: WW=1500, WC=-600
            dynamic_cast<GUICenter*>(m_childWidget)->setWindowLevel(1500, -600);
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelBone,
          this,
          [this]()
          {
            // Bone: WW=2000, WC=300
            dynamic_cast<GUICenter*>(m_childWidget)->setWindowLevel(2000, 300);
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::fitToWindow,
          this,
          [this]() { dynamic_cast<GUICenter*>(m_childWidget)->fitToWindow(); });
}

void GUIWindow::openFile()
{
  dynamic_cast<GUICenter*>(m_childWidget)->onOpenFile();
}

void GUIWindow::openFolder()
{
  dynamic_cast<GUICenter*>(m_childWidget)->onOpenFolder();
}
