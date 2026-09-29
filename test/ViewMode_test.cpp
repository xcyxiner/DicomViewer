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

}  // namespace

// 默认视图模式为仅 2D：2D 注册落槽 0，MPR 注册不占槽
TEST(ViewModeDefault, TwoDOnlyShowsTwoDInSlot0)
{
  GUIWindow window;
  EXPECT_EQ(window.viewMode(), ViewMode::TwoDOnly) << "默认应为仅 2D";

  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  EXPECT_EQ(window.panelInSlot(0), &twoD) << "槽 0 应为 2D";
  EXPECT_EQ(window.panelInSlot(1), nullptr) << "仅 2D 下其余槽应为空";
}

// 切仅 MPR：MPR 换入槽 0，2D 让出槽位
TEST(ViewModeSwitch, MprOnlyShowsMprInSlot0)
{
  GUIWindow window;
  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  window.setViewMode(ViewMode::MprOnly);

  EXPECT_EQ(window.viewMode(), ViewMode::MprOnly);
  EXPECT_EQ(window.panelInSlot(0), &mpr);
  EXPECT_EQ(window.panelInSlot(1), nullptr);
}

// 切分屏：2D 槽 0、MPR 槽 1（按注册序补位）
TEST(ViewModeSwitch, SplitShowsBothPanels)
{
  GUIWindow window;
  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  window.setViewMode(ViewMode::Split);

  EXPECT_EQ(window.panelInSlot(0), &twoD);
  EXPECT_EQ(window.panelInSlot(1), &mpr);
}

// 模式往返：分屏下的自定义指派在切走再切回后恢复
TEST(ViewModeRoundTrip, RemembersSplitAssignments)
{
  GUIWindow window;
  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  window.setViewMode(ViewMode::Split);
  // 分屏下对调：MPR → 槽 0，2D → 槽 1
  window.assignViewToSlot(0, 1);
  window.assignViewToSlot(1, 0);

  window.setViewMode(ViewMode::MprOnly);
  EXPECT_EQ(window.panelInSlot(0), &mpr);
  EXPECT_EQ(window.panelInSlot(1), nullptr);

  window.setViewMode(ViewMode::Split);
  EXPECT_EQ(window.panelInSlot(0), &mpr) << "分屏指派应被记住";
  EXPECT_EQ(window.panelInSlot(1), &twoD);
}

// 单视图模式下逐格分配被忽略（分配菜单同步禁用）
TEST(ViewModeAssign, AssignIgnoredOutsideSplit)
{
  GUIWindow window;
  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  window.assignViewToSlot(1, 0);  // 仅 2D 模式：越权分配

  EXPECT_EQ(window.panelInSlot(1), nullptr) << "越权分配不应生效";
  EXPECT_EQ(window.panelInSlot(0), &twoD) << "槽 0 不应被越权改动";
}

// 注册时机：先切仅 2D 再注册，MPR 不占槽；回仅 MPR 后换位
TEST(ViewModeRegister, RegistrationFollowsCurrentMode)
{
  GUIWindow window;
  window.setViewMode(ViewMode::TwoDOnly);

  FakePanel twoD(QStringLiteral("2D Viewer"), ViewRole::TwoD);
  FakePanel mpr(QStringLiteral("MPR"), ViewRole::Mpr);
  window.registerView(&twoD);
  window.registerView(&mpr);

  EXPECT_EQ(window.panelInSlot(0), &twoD);
  EXPECT_EQ(window.panelInSlot(1), nullptr) << "仅 2D 下 MPR 不应占槽";

  window.setViewMode(ViewMode::MprOnly);
  EXPECT_EQ(window.panelInSlot(0), &mpr) << "仅 MPR 下 MPR 应换入槽 0";
  EXPECT_EQ(window.panelInSlot(1), nullptr);
}

// viewModeChanged 信号：模式变化恰发出一次，同值设置不重发
TEST(ViewModeSignal, EmitsOnlyOnChange)
{
  GUIWindow window;
  int emitCount = 0;
  ViewMode lastMode = ViewMode::MprOnly;
  QObject::connect(&window,
                   &GUIWindow::viewModeChanged,
                   &window,
                   [&emitCount, &lastMode](ViewMode mode)
                   {
                     ++emitCount;
                     lastMode = mode;
                   });

  window.setViewMode(ViewMode::Split);
  EXPECT_EQ(emitCount, 1);
  EXPECT_EQ(lastMode, ViewMode::Split);

  window.setViewMode(ViewMode::Split);  // 同值：不重发
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
