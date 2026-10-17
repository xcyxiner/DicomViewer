
#include <QAction>

#include "ViewMenu.h"

#include "qaction.h"
#include "qmenu.h"
#include "ui_ViewMenu.h"

ViewMenu::ViewMenu(QMenuBar* menuBar, QWidget* parent)
    : QMenu(parent)
    , ui(new Ui::ViewMenu)
{
  ui->setupUi(menuBar);

  // 视图模式单选组
  m_modeGroup = new QActionGroup(this);
  m_modeGroup->setExclusive(true);
  m_modeGroup->addAction(ui->actionTwoDOnly);
  m_modeGroup->addAction(ui->actionMpr3d);
  ui->actionTwoDOnly->setChecked(true);  // 默认：仅 2D

  createConnections();
}

ViewMenu::~ViewMenu()
{
  delete ui;
}

void ViewMenu::createConnections()
{
  connect(ui->actionTwoDOnly,
          &QAction::triggered,
          this,
          [this]() { emit viewModeChanged(ViewMode::TwoDOnly); });
  connect(ui->actionMpr3d,
          &QAction::triggered,
          this,
          [this]() { emit viewModeChanged(ViewMode::Mpr3d); });
}

void ViewMenu::setViewMode(ViewMode mode)
{
  switch (mode) {
    case ViewMode::TwoDOnly:
      ui->actionTwoDOnly->setChecked(true);
      break;
    case ViewMode::Mpr3d:
      ui->actionMpr3d->setChecked(true);
      break;
  }
}
