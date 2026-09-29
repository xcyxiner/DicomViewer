
#pragma once
#include <QActionGroup>
#include <QMenu>
#include <QMenuBar>

#include "presentation/view/ViewMode.h"
#include "qtmetamacros.h"

namespace Ui
{
class ViewMenu;
}

// 「View」菜单：视图模式排他三选一（仅 MPR / 仅 2D / 分屏）。
// 默认选中「仅 2D」——启动只显示 2D，MPR 需经菜单切出；
// GUIWindow::viewModeChanged 回投 setViewMode 同步勾选状态。
class ViewMenu : public QMenu
{
  Q_OBJECT
public:
  explicit ViewMenu(QMenuBar* menuBar, QWidget* parent = nullptr);
  ~ViewMenu();

  // 同步选中状态（GUIWindow 回投）
  void setViewMode(ViewMode mode);

signals:
  void viewModeChanged(ViewMode mode);

private:
  void createConnections();

private:
  Ui::ViewMenu* ui;
  QActionGroup* m_modeGroup;
};
