
#pragma once
#include <QGridLayout>
#include <QMainWindow>
#include <QVector>

#include "presentation/view/IViewPanel.h"
#include "presentation/view/LayoutMode.h"
#include "presentation/view/ViewMode.h"
#include "presentation/view/menu/FileMenu.h"
#include "presentation/view/menu/WindowLevelMenu.h"
#include "qtmetamacros.h"

class LayoutMenu;
class ViewMenu;

namespace Ui
{
class GUIWindow;
}

class GUIWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit GUIWindow(QWidget* parent = nullptr);
  ~GUIWindow();

public:
  void openFile();
  void openFolder();

  // 视图注册与管理
  void registerView(IViewPanel* panel);
  void setGridLayout(LayoutMode mode);
  void assignViewToSlot(int slot, int viewIndex);
  void setFocusedSlot(int slot);

  // 视图模式（View 菜单）：决定哪些面板参与槽位分配，默认仅 2D
  void setViewMode(ViewMode mode);

  LayoutMode layoutMode() const { return m_layoutMode; }

  ViewMode viewMode() const { return m_viewMode; }

  int slotCount() const;

  int focusedSlot() const { return m_focusedSlot; }

  IViewPanel* focusedPanel() const;
  IViewPanel* panelInSlot(int slot) const;

  const QVector<IViewPanel*>& panels() const { return m_panels; }

signals:
  void layoutModeChanged(LayoutMode mode);
  void viewModeChanged(ViewMode mode);
  void viewRegistered(const QString& name);
  void focusedSlotChanged(int slot);

private:
  void createMenu();
  void refreshLayout();
  QWidget* createPlaceholder();
  IViewPanel* findPanelForSlot(int slot) const;
  // 该角色在该视图模式下是否参与槽位分配
  bool isRoleActive(ViewRole role, ViewMode mode) const;
  // 以 base 为起点解析槽位：补进模式内活跃但未占槽的面板，剔除
  // 非活跃面板（单视图模式下 base 传空表 → 活跃面板落槽 0）
  QVector<int> resolveSlots(const QVector<int>& base, ViewMode mode) const;
  // 打开广播：遍历全部已注册面板（KTD8：消除 Single 布局下按槽
  // 广播的盲区；MPR 的 loadFiles 为 no-op，此举安全）
  void broadcastLoadFiles(const QStringList& paths);

private:
  QVector<IViewPanel*> m_panels;
  QVector<int> m_slotAssignments;  // 格子→视图索引映射，-1 表示空
  int m_focusedSlot = 0;
  LayoutMode m_layoutMode = LayoutMode::Single;
  ViewMode m_viewMode = ViewMode::TwoDOnly;  // 默认：仅 2D
  // 离开分屏模式时记住的槽位指派，回到分屏时恢复
  QVector<int> m_splitAssignments = {-1, -1, -1, -1};

  QGridLayout* m_gridLayout = nullptr;

  FileMenu* m_fileMenu;
  WindowLevelMenu* m_windowLevelMenu;
  ViewMenu* m_viewMenu;
  LayoutMenu* m_layoutMenu;

  std::unique_ptr<Ui::GUIWindow> ui;
};
