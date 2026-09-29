
#pragma once
#include <QActionGroup>
#include <QMenu>
#include <QMenuBar>
#include <QVector>

#include "presentation/view/LayoutMode.h"
#include "qtmetamacros.h"

class IViewPanel;

namespace Ui
{
class LayoutMenu;
}

class LayoutMenu : public QMenu
{
  Q_OBJECT
public:
  explicit LayoutMenu(QMenuBar* menuBar, QWidget* parent = nullptr);
  ~LayoutMenu();

  // 更新格子子菜单中的视图列表
  void updateSlotMenus(const QVector<IViewPanel*>& panels, int slotCount);

  // 更新布局模式选中状态
  void setLayoutMode(LayoutMode mode);

  // 「分配视图」子菜单启禁用 —— 非分屏视图模式下槽位由 ViewMode
  // 决定，逐格分配无意义（GUIWindow 侧同步忽略越权分配）
  void setSlotMenusEnabled(bool enabled);

signals:
  void layoutModeChanged(LayoutMode mode);
  void slotViewChanged(int slot, int viewIndex);

private:
  void createConnections();
  void rebuildSlotSubMenus(int slotCount, const QVector<IViewPanel*>& panels);

private:
  Ui::LayoutMenu* ui;
  QActionGroup* m_modeGroup;
  QMenu* m_slotMenu;  // "分配视图" 子菜单
  QVector<QMenu*> m_slotSubMenus;
};
