
#include <QFileDialog>
#include <QLabel>
#include <QVBoxLayout>

#include "GUIWindow.h"

#include "presentation/view/menu/FileMenu.h"
#include "presentation/view/menu/LayoutMenu.h"
#include "presentation/view/menu/ViewMenu.h"
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

  // ViewMenu（默认仅 2D：启动只显示 2D，MPR 经菜单切出）
  this->m_viewMenu = new ViewMenu(this->ui->menubar, this);
  connect(
      m_viewMenu, &ViewMenu::viewModeChanged, this, &GUIWindow::setViewMode);
  connect(this,
          &GUIWindow::viewModeChanged,
          this,
          [this](ViewMode mode)
          {
            m_viewMenu->setViewMode(mode);
            // 非分屏模式下逐格分配无意义，禁用「分配视图」
            m_layoutMenu->setSlotMenusEnabled(mode == ViewMode::Split);
          });

  // LayoutMenu
  this->m_layoutMenu = new LayoutMenu(this->ui->menubar, this);
  m_layoutMenu->setSlotMenusEnabled(false);  // 默认仅 2D
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

  // 按当前视图模式解析槽位：默认仅 2D 下 2D 先注册落槽 0，
  // MPR 随后注册不占槽 —— 启动即只显示 2D
  m_slotAssignments = resolveSlots(m_slotAssignments, m_viewMode);

  // refreshLayout 会对所有可见面板补调 activate（KTD8），首个面板
  // 的激活语义由其承担；此处只初始化焦点槽。
  refreshLayout();

  if (m_panels.size() == 1) {
    m_focusedSlot = 0;
  }

  emit viewRegistered(panel->viewName());
}

void GUIWindow::setViewMode(ViewMode mode)
{
  if (mode == m_viewMode) {
    return;
  }
  if (m_viewMode == ViewMode::Split) {
    // 记住分屏指派，回到分屏时恢复
    m_splitAssignments = m_slotAssignments;
  }
  m_viewMode = mode;
  const QVector<int> base = (mode == ViewMode::Split)
      ? m_splitAssignments
      : QVector<int> {-1, -1, -1, -1};
  m_slotAssignments = resolveSlots(base, mode);
  if (!findPanelForSlot(m_focusedSlot)) {
    m_focusedSlot = 0;
  }
  refreshLayout();
  emit viewModeChanged(mode);
}

bool GUIWindow::isRoleActive(ViewRole role, ViewMode mode) const
{
  switch (mode) {
    case ViewMode::MprOnly:
      return role == ViewRole::Mpr;
    case ViewMode::TwoDOnly:
      return role == ViewRole::TwoD;
    case ViewMode::Split:
      return true;
  }
  return true;
}

QVector<int> GUIWindow::resolveSlots(const QVector<int>& base,
                                     ViewMode mode) const
{
  // 注意：局部变量不可叫 slots —— Qt 关键字宏会把它展开为空
  QVector<int> resolved = base;

  // 模式内活跃但尚未占槽的面板 → 按注册序补进空槽
  for (int panelIndex = 0; panelIndex < m_panels.size(); ++panelIndex) {
    if (!isRoleActive(m_panels[panelIndex]->viewRole(), mode)
        || resolved.contains(panelIndex))
    {
      continue;
    }
    int freeSlot = resolved.indexOf(-1);
    if (freeSlot < 0) {
      break;
    }
    resolved[freeSlot] = panelIndex;
  }

  // 非活跃面板让出槽位（单视图模式下另一视图不显示）
  for (int slot = 0; slot < resolved.size(); ++slot) {
    int panelIndex = resolved[slot];
    if (panelIndex >= 0 && panelIndex < m_panels.size()
        && !isRoleActive(m_panels[panelIndex]->viewRole(), mode))
    {
      resolved[slot] = -1;
    }
  }
  return resolved;
}

void GUIWindow::setGridLayout(LayoutMode mode)
{
  m_layoutMode = mode;
  refreshLayout();
  emit layoutModeChanged(mode);
}

void GUIWindow::assignViewToSlot(int slot, int viewIndex)
{
  // 单视图模式下槽位由 ViewMode 决定（分配菜单同步禁用），忽略越权分配
  if (m_viewMode != ViewMode::Split) {
    return;
  }
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
  // 摘除上一轮全部格子控件但不改父子关系：setParent(nullptr) 会把
  // QVTKOpenGLNativeWidget 摘成顶层窗口，GL 上下文随原生窗口销毁，
  // 下一轮 addWidget 重挂时上下文重建——VTK 清理旧 FBO 报
  // "glDeleteFramebuffers Invalid operation"（每格一组 16 条）。
  // takeAt 后面板仍是 centralwidget 的子控件、仅脱离布局管理；占位
  // 格是每轮新建的临时对象，摘下即删（原先遗弃到无父状态会泄漏）。
  while (m_gridLayout->count() > 0) {
    QLayoutItem* item = m_gridLayout->takeAt(0);
    QWidget* widget = item->widget();
    delete item;
    if (widget && !qobject_cast<IViewPanel*>(widget)) {
      delete widget;
    }
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

  // 本轮不占槽的面板显式隐藏——原实现靠 setParent(nullptr) 的附带
  // 隐藏表达"离槽"，保住父子关系后须手动表达；含已指派但超出本轮
  // 格数的槽位（如 Split 指派 4 槽 + Single 布局只渲染槽 0）。
  for (IViewPanel* panel : m_panels) {
    bool placed = false;
    for (int slot = 0; slot < rows * cols && !placed; ++slot) {
      placed = (findPanelForSlot(slot) == panel);
    }
    if (!placed) {
      panel->hide();
    }
  }

  for (int slot = 0; slot < rows * cols; ++slot) {
    int row = slot / cols;
    int col = slot % cols;

    IViewPanel* panel = findPanelForSlot(slot);
    if (panel) {
      m_gridLayout->addWidget(panel, row, col);
      panel->show();
      // KTD8：布局切换会重挂 widget 触发 GL 上下文重建，对可见面板
      // 补调 activate（幂等：注入渲染目标并补推缓存状态），消除
      // "注册后未激活"的面板生命周期缺口。
      panel->activate();
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
    broadcastLoadFiles(fileDialog.selectedFiles());
  }
}

void GUIWindow::openFolder()
{
  QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹");
  if (!dir.isEmpty()) {
    broadcastLoadFiles({dir});
  }
}

void GUIWindow::broadcastLoadFiles(const QStringList& paths)
{
  for (IViewPanel* panel : m_panels) {
    if (panel) {
      panel->loadFiles(paths);
    }
  }
}
