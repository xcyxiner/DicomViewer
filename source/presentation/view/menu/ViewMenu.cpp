
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
  m_modeGroup->addAction(ui->actionMprOnly);
  m_modeGroup->addAction(ui->actionTwoDOnly);
  m_modeGroup->addAction(ui->actionSplit);
  ui->actionTwoDOnly->setChecked(true);  // 默认：仅 2D

  createConnections();
}

ViewMenu::~ViewMenu()
{
  delete ui;
}

void ViewMenu::createConnections()
{
  connect(ui->actionMprOnly,
          &QAction::triggered,
          this,
          [this]() { emit viewModeChanged(ViewMode::MprOnly); });
  connect(ui->actionTwoDOnly,
          &QAction::triggered,
          this,
          [this]() { emit viewModeChanged(ViewMode::TwoDOnly); });
  connect(ui->actionSplit,
          &QAction::triggered,
          this,
          [this]() { emit viewModeChanged(ViewMode::Split); });
}

void ViewMenu::setViewMode(ViewMode mode)
{
  switch (mode) {
    case ViewMode::MprOnly:
      ui->actionMprOnly->setChecked(true);
      break;
    case ViewMode::TwoDOnly:
      ui->actionTwoDOnly->setChecked(true);
      break;
    case ViewMode::Split:
      ui->actionSplit->setChecked(true);
      break;
  }
}
