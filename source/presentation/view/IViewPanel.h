
#pragma once
#include <QString>
#include <QStringList>
#include <QWidget>

// 面板角色 —— ViewMode 据此决定面板是否参与槽位分配：
// TwoDOnly 只给 TwoD 分配槽位；Mpr3d 给 Mpr + Volume 分配（2D 让位）。
enum class ViewRole
{
  TwoD,
  Mpr,
  Volume  // 3D 体绘制面板
};

class IViewPanel : public QWidget
{
  Q_OBJECT
public:
  explicit IViewPanel(QWidget* parent = nullptr)
      : QWidget(parent)
  {
  }
  virtual ~IViewPanel() = default;

  // 显示名称（纯虚，每个视图必须实现）
  virtual QString viewName() const = 0;

  // 面板角色（默认 2D；MPR 面板覆写）
  virtual ViewRole viewRole() const { return ViewRole::TwoD; }

  // 生命周期回调
  virtual void activate() {}
  virtual void deactivate() {}

  // 缩放适应窗口
  virtual void fitToWindow() {}

  // 窗宽窗位
  virtual void setWindowLevel(double windowWidth, double windowCenter)
  {
    Q_UNUSED(windowWidth)
    Q_UNUSED(windowCenter)
  }
  virtual void resetWindowLevel() {}

  // 文件加载
  virtual void loadFiles(const QStringList& paths) { Q_UNUSED(paths) }
};
