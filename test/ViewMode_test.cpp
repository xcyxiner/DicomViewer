#include <QApplication>
#include <QByteArray>
#include <QString>

#include "presentation/view/ViewMode.h"

#include <gtest/gtest.h>

#include "presentation/view/IViewPanel.h"
#include "presentation/view/window/GUIWindow.h"

namespace
{

// 逻辑级假面板：不建渲染/GL，只提供名字与角色，专测 GUIWindow
// 的视图模式槽位分配。
class FakePanel : public IViewPanel
{
public:
  FakePanel(const QString& name, ViewRole role)
      : m_name(name)
      , m_role(role)
  {
  }

  QString viewName() const override { return m_name; }

  ViewRole viewRole() const override { return m_role; }

private:
  QString m_name;
  ViewRole m_role;
};

// 注册全套面板（与 main.cpp 注册序一致）：2D → 轴 → 冠 → 矢 → 3D
struct FullPanels
{
  FakePanel twoD {QStringLiteral("2D Viewer"), ViewRole::TwoD};
  FakePanel axial {QStringLiteral("MPR 轴位"), ViewRole::Mpr};
  FakePanel coronal {QStringLiteral("MPR 冠状"), ViewRole::Mpr};
  FakePanel sagittal {QStringLiteral("MPR 矢状"), ViewRole::Mpr};
  FakePanel volume {QStringLiteral("3D 体绘制"), ViewRole::Volume};

  void registerAll(GUIWindow& window)
  {
    window.registerView(&twoD);
    window.registerView(&axial);
    window.registerView(&coronal);
    window.registerView(&sagittal);
    window.registerView(&volume);
  }
};

}  // namespace

// 默认视图模式为仅 2D：2D 注册落槽 0，MPR/3D 注册不占槽，布局 Single
TEST(ViewModeDefault, TwoDOnlyShowsTwoDInSlot0)
{
  GUIWindow window;
  EXPECT_EQ(window.viewMode(), ViewMode::TwoDOnly) << "默认应为仅 2D";
  EXPECT_EQ(window.layoutMode(), LayoutMode::Single) << "默认应为 1x1";

  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  EXPECT_EQ(window.panelInSlot(0), &twoD) << "槽 0 应为 2D";
  EXPECT_EQ(window.panelInSlot(1), nullptr) << "仅 2D 下其余槽应为空";
}

// 切 MPR + 3D：四格 2x2，槽 0-3 按注册序 = 轴/冠/矢/3D，2D 让位
TEST(ViewModeSwitch, Mpr3dShowsFourPanelsInGrid)
{
  GUIWindow window;
  FullPanels panels;
  panels.registerAll(window);

  window.setViewMode(ViewMode::Mpr3d);

  EXPECT_EQ(window.viewMode(), ViewMode::Mpr3d);
  EXPECT_EQ(window.layoutMode(), LayoutMode::Grid) << "应强制 2x2";
  EXPECT_EQ(window.panelInSlot(0), &panels.axial);
  EXPECT_EQ(window.panelInSlot(1), &panels.coronal);
  EXPECT_EQ(window.panelInSlot(2), &panels.sagittal);
  EXPECT_EQ(window.panelInSlot(3), &panels.volume);
}

// 模式往返强制布局：进 MPR+3D → Grid，回仅 2D → Single 且槽 0=2D
TEST(ViewModeRoundTrip, SwitchForcesLayout)
{
  GUIWindow window;
  FullPanels panels;
  panels.registerAll(window);

  window.setViewMode(ViewMode::Mpr3d);
  EXPECT_EQ(window.layoutMode(), LayoutMode::Grid);

  window.setViewMode(ViewMode::TwoDOnly);
  EXPECT_EQ(window.layoutMode(), LayoutMode::Single)
      << "回仅 2D 应强制 1x1（否则其余格是「无视图」占位）";
  EXPECT_EQ(window.panelInSlot(0), &panels.twoD);
  EXPECT_EQ(window.panelInSlot(1), nullptr);
}

// 逐格分配在两种模式下均被忽略（分配菜单同步恒禁用）
TEST(ViewModeAssign, AssignIgnoredInAllModes)
{
  GUIWindow window;
  FullPanels panels;
  panels.registerAll(window);

  // 仅 2D：越权把轴位塞进槽 1
  window.assignViewToSlot(1, 1);
  EXPECT_EQ(window.panelInSlot(1), nullptr) << "越权分配不应生效";
  EXPECT_EQ(window.panelInSlot(0), &panels.twoD) << "槽 0 不应被越权改动";

  // MPR + 3D：越权对调槽 0/1
  window.setViewMode(ViewMode::Mpr3d);
  window.assignViewToSlot(0, 2);
  window.assignViewToSlot(1, 1);
  EXPECT_EQ(window.panelInSlot(0), &panels.axial) << "槽位应保持注册序";
  EXPECT_EQ(window.panelInSlot(1), &panels.coronal);
}

// 注册时机：仅 2D 下注册时 MPR/3D 不占槽；切 MPR+3D 后按注册序补槽
TEST(ViewModeRegister, RegistrationFollowsCurrentMode)
{
  GUIWindow window;
  window.setViewMode(ViewMode::TwoDOnly);

  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  FakePanel volume(QStringLiteral("3D 体绘制"), ViewRole::Volume);
  window.registerView(&twoD);
  window.registerView(&mpr);
  window.registerView(&volume);

  EXPECT_EQ(window.panelInSlot(0), &twoD);
  EXPECT_EQ(window.panelInSlot(1), nullptr) << "仅 2D 下 MPR 不应占槽";

  window.setViewMode(ViewMode::Mpr3d);
  EXPECT_EQ(window.panelInSlot(0), &mpr) << "MPR+3D 下 MPR 应换入槽 0";
  EXPECT_EQ(window.panelInSlot(1), &volume) << "3D 按注册序补槽 1";
}

// viewModeChanged 信号：模式变化恰发出一次，同值设置不重发
TEST(ViewModeSignal, EmitsOnlyOnChange)
{
  GUIWindow window;
  int emitCount = 0;
  ViewMode lastMode = ViewMode::TwoDOnly;
  QObject::connect(&window,
                   &GUIWindow::viewModeChanged,
                   &window,
                   [&emitCount, &lastMode](ViewMode mode)
                   {
                     ++emitCount;
                     lastMode = mode;
                   });

  window.setViewMode(ViewMode::Mpr3d);
  EXPECT_EQ(emitCount, 1);
  EXPECT_EQ(lastMode, ViewMode::Mpr3d);

  window.setViewMode(ViewMode::Mpr3d);  // 同值：不重发
  EXPECT_EQ(emitCount, 1);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // 控件测试无显示服务器：强制 offscreen 平台（须在 QApplication 构造前）
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication application(argc, argv);
  return RUN_ALL_TESTS();
}
