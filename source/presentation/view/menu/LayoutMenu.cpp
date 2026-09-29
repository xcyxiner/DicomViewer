
#include <QAction>

#include "LayoutMenu.h"

#include "presentation/view/IViewPanel.h"
#include "qaction.h"
#include "qmenu.h"
#include "ui_LayoutMenu.h"

LayoutMenu::LayoutMenu(QMenuBar* menuBar, QWidget* parent)
    : QMenu(parent)
    , ui(new Ui::LayoutMenu)
{
  ui->setupUi(menuBar);

  // 布局模式单选组
  m_modeGroup = new QActionGroup(this);
  m_modeGroup->setExclusive(true);
  m_modeGroup->addAction(ui->actionSingle);
  m_modeGroup->addAction(ui->actionHorizontal);
  m_modeGroup->addAction(ui->actionGrid);
  ui->actionSingle->setChecked(true);

  // 创建 "分配视图" 子菜单
  m_slotMenu = ui->menuLayout->addMenu("分配视图");

  createConnections();
}

LayoutMenu::~LayoutMenu()
{
  delete ui;
}

void LayoutMenu::createConnections()
{
  connect(ui->actionSingle,
          &QAction::triggered,
          this,
          [this]() { emit layoutModeChanged(LayoutMode::Single); });
  connect(ui->actionHorizontal,
          &QAction::triggered,
          this,
          [this]() { emit layoutModeChanged(LayoutMode::Horizontal); });
  connect(ui->actionGrid,
          &QAction::triggered,
          this,
          [this]() { emit layoutModeChanged(LayoutMode::Grid); });
}

void LayoutMenu::setLayoutMode(LayoutMode mode)
{
  switch (mode) {
    case LayoutMode::Single:
      ui->actionSingle->setChecked(true);
      break;
    case LayoutMode::Horizontal:
      ui->actionHorizontal->setChecked(true);
      break;
    case LayoutMode::Grid:
      ui->actionGrid->setChecked(true);
      break;
  }
}

void LayoutMenu::setSlotMenusEnabled(bool enabled)
{
  m_slotMenu->setEnabled(enabled);
}

void LayoutMenu::updateSlotMenus(const QVector<IViewPanel*>& panels,
                                 int slotCount)
{
  rebuildSlotSubMenus(slotCount, panels);
}

void LayoutMenu::rebuildSlotSubMenus(int slotCount,
                                     const QVector<IViewPanel*>& panels)
{
  // 清除旧的格子子菜单
  m_slotMenu->clear();
  m_slotSubMenus.clear();

  for (int slot = 0; slot < slotCount; ++slot) {
    QString slotName = QString("格子 %1").arg(slot + 1);
    QMenu* subMenu = m_slotMenu->addMenu(slotName);
    m_slotSubMenus.append(subMenu);

    // 添加 "空" 选项
    QAction* emptyAction = subMenu->addAction("空");
    emptyAction->setCheckable(true);
    connect(emptyAction,
            &QAction::triggered,
            this,
            [this, slot]() { emit slotViewChanged(slot, -1); });

    // 添加每个已注册视图
    for (int i = 0; i < panels.size(); ++i) {
      QAction* action = subMenu->addAction(panels[i]->viewName());
      action->setCheckable(true);
      connect(action,
              &QAction::triggered,
              this,
              [this, slot, i]() { emit slotViewChanged(slot, i); });
    }
  }
}
