
#include <QFileDialog>
#include <QLabel>
#include <QVBoxLayout>

#include "GUIWindow.h"

#include "presentation/view/menu/FileMenu.h"
#include "presentation/view/menu/LayoutMenu.h"
#include "presentation/view/menu/WindowLevelMenu.h"
#include "ui_GUIWindow.h"

GUIWindow::GUIWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::GUIWindow>())
{
  ui->setupUi(this);

  // 创建 QGridLayout 添加到 ui->layout 中
  m_gridLayout = new QGridLayout();
  m_gridLayout->setContentsMargins(0, 0, 0, 0);
  m_gridLayout->setSpacing(2);
  ui->layout->addLayout(m_gridLayout);

  // 初始化格子分配（最多4个格子）
  m_slotAssignments = {-1, -1, -1, -1};

  createMenu();
}

GUIWindow::~GUIWindow() = default;

void GUIWindow::createMenu()
{
  this->m_fileMenu = new FileMenu(this->ui->menubar, this);
  connect(m_fileMenu, &FileMenu::openFile, this, &GUIWindow::openFile);
  connect(m_fileMenu, &FileMenu::openFolder, this, &GUIWindow::openFolder);
  connect(m_fileMenu, &FileMenu::closeApp, this, &GUIWindow::close);

  this->m_windowLevelMenu = new WindowLevelMenu(this->ui->menubar, this);
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelDefault,
          this,
          [this]()
          {
            auto* panel = focusedPanel();
            if (panel) {
              panel->resetWindowLevel();
            }
          });
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
            auto* panel = focusedPanel();
            if (panel) {
              panel->setWindowLevel(80, 40);
            }
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelLung,
          this,
          [this]()
          {
            auto* panel = focusedPanel();
            if (panel) {
              panel->setWindowLevel(1500, -600);
            }
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::windowLevelBone,
          this,
          [this]()
          {
            auto* panel = focusedPanel();
            if (panel) {
              panel->setWindowLevel(2000, 300);
            }
          });
  connect(m_windowLevelMenu,
          &WindowLevelMenu::fitToWindow,
          this,
          [this]()
          {
            auto* panel = focusedPanel();
            if (panel) {
              panel->fitToWindow();
            }
          });

  // LayoutMenu
  this->m_layoutMenu = new LayoutMenu(this->ui->menubar, this);
  connect(m_layoutMenu,
          &LayoutMenu::layoutModeChanged,
          this,
          &GUIWindow::setGridLayout);
  connect(m_layoutMenu,
          &LayoutMenu::slotViewChanged,
          this,
          &GUIWindow::assignViewToSlot);
  connect(this,
          &GUIWindow::layoutModeChanged,
          this,
          [this](LayoutMode mode)
          {
            m_layoutMenu->setLayoutMode(mode);
            m_layoutMenu->updateSlotMenus(m_panels, slotCount());
          });
  connect(this,
          &GUIWindow::viewRegistered,
          this,
          [this](const QString&)
          { m_layoutMenu->updateSlotMenus(m_panels, slotCount()); });
}

// --- 视图注册与管理 ---

void GUIWindow::registerView(IViewPanel* panel)
{
  m_panels.append(panel);

  // 找到第一个空格子分配
  for (int i = 0; i < m_slotAssignments.size(); ++i) {
    if (m_slotAssignments[i] == -1) {
      m_slotAssignments[i] = m_panels.size() - 1;
      break;
    }
  }

  refreshLayout();

  // 第一个注册的视图自动激活
  if (m_panels.size() == 1) {
    m_panels[0]->activate();
    m_focusedSlot = 0;
  }

  emit viewRegistered(panel->viewName());
}

void GUIWindow::setGridLayout(LayoutMode mode)
{
  m_layoutMode = mode;
  refreshLayout();
  emit layoutModeChanged(mode);
}

void GUIWindow::assignViewToSlot(int slot, int viewIndex)
{
  if (slot >= 0 && slot < m_slotAssignments.size()) {
    m_slotAssignments[slot] = viewIndex;
    refreshLayout();
  }
}

void GUIWindow::setFocusedSlot(int slot)
{
  if (slot >= 0 && slot < slotCount()) {
    m_focusedSlot = slot;
    emit focusedSlotChanged(slot);
  }
}

int GUIWindow::slotCount() const
{
  switch (m_layoutMode) {
    case LayoutMode::Single:
      return 1;
    case LayoutMode::Horizontal:
      return 2;
    case LayoutMode::Grid:
      return 4;
  }
  return 1;
}

IViewPanel* GUIWindow::focusedPanel() const
{
  return panelInSlot(m_focusedSlot);
}

IViewPanel* GUIWindow::panelInSlot(int slot) const
{
  return findPanelForSlot(slot);
}

IViewPanel* GUIWindow::findPanelForSlot(int slot) const
{
  if (slot < 0 || slot >= m_slotAssignments.size()) {
    return nullptr;
  }
  int panelIndex = m_slotAssignments[slot];
  if (panelIndex < 0 || panelIndex >= m_panels.size()) {
    return nullptr;
  }
  return m_panels[panelIndex];
}

void GUIWindow::refreshLayout()
{
  // 清空网格中的所有控件（不删除）
  while (m_gridLayout->count() > 0) {
    QLayoutItem* item = m_gridLayout->takeAt(0);
    if (item->widget()) {
      item->widget()->setParent(nullptr);
    }
    delete item;
  }

  int cols, rows;
  switch (m_layoutMode) {
    case LayoutMode::Single:
      rows = 1;
      cols = 1;
      break;
    case LayoutMode::Horizontal:
      rows = 1;
      cols = 2;
      break;
    case LayoutMode::Grid:
      rows = 2;
      cols = 2;
      break;
  }

  for (int slot = 0; slot < rows * cols; ++slot) {
    int row = slot / cols;
    int col = slot % cols;

    IViewPanel* panel = findPanelForSlot(slot);
    if (panel) {
      m_gridLayout->addWidget(panel, row, col);
      panel->show();
      panel->fitToWindow();
    } else {
      m_gridLayout->addWidget(createPlaceholder(), row, col);
    }
  }
}

QWidget* GUIWindow::createPlaceholder()
{
  auto* placeholder = new QWidget();
  placeholder->setStyleSheet("background-color: #2b2b2b;");
  auto* label = new QLabel("无视图", placeholder);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet("color: #888888; font-size: 14px;");
  auto* layout = new QVBoxLayout(placeholder);
  layout->addWidget(label);
  return placeholder;
}

// --- 文件操作 ---

void GUIWindow::openFile()
{
  QFileDialog fileDialog(this, "选择文件");
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted) {
    QStringList paths = fileDialog.selectedFiles();
    // 传递给所有活动视图
    for (int i = 0; i < slotCount(); ++i) {
      IViewPanel* panel = findPanelForSlot(i);
      if (panel) {
        panel->loadFiles(paths);
      }
    }
  }
}

void GUIWindow::openFolder()
{
  QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹");
  if (!dir.isEmpty()) {
    QStringList paths = {dir};
    for (int i = 0; i < slotCount(); ++i) {
      IViewPanel* panel = findPanelForSlot(i);
      if (panel) {
        panel->loadFiles(paths);
      }
    }
  }
}
