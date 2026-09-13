
#pragma once
#include <QGridLayout>
#include <QMainWindow>
#include <QVector>

#include "presentation/view/IViewPanel.h"
#include "presentation/view/LayoutMode.h"
#include "presentation/view/menu/FileMenu.h"
#include "presentation/view/menu/WindowLevelMenu.h"
#include "qtmetamacros.h"

class LayoutMenu;

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

  LayoutMode layoutMode() const { return m_layoutMode; }

  int slotCount() const;

  int focusedSlot() const { return m_focusedSlot; }

  IViewPanel* focusedPanel() const;
  IViewPanel* panelInSlot(int slot) const;

  const QVector<IViewPanel*>& panels() const { return m_panels; }

signals:
  void layoutModeChanged(LayoutMode mode);
  void viewRegistered(const QString& name);
  void focusedSlotChanged(int slot);

private:
  void createMenu();
  void refreshLayout();
  QWidget* createPlaceholder();
  IViewPanel* findPanelForSlot(int slot) const;

private:
  QVector<IViewPanel*> m_panels;
  QVector<int> m_slotAssignments;  // 格子→视图索引映射，-1 表示空
  int m_focusedSlot = 0;
  LayoutMode m_layoutMode = LayoutMode::Single;

  QGridLayout* m_gridLayout = nullptr;

  FileMenu* m_fileMenu;
  WindowLevelMenu* m_windowLevelMenu;
  LayoutMenu* m_layoutMenu;

  std::unique_ptr<Ui::GUIWindow> ui;
};
