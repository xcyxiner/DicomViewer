
#pragma once
#include <QString>
#include <QStringList>
#include <QWidget>

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
