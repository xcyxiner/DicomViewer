
#pragma once
#include <memory>

#include "presentation/view/IViewPanel.h"
#include "presentation/viewmodels/SeriesViewModel.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>

namespace Ui {
    class GUICenter;
}
class QSlider;
class QLabel;
class GUICenter : public IViewPanel
{
public:
  explicit GUICenter(SeriesViewModel* viewModel,
                     QWidget* parent = nullptr);
  ~GUICenter() override;

  // IViewPanel 接口
  QString viewName() const override { return "2D Viewer"; }
  void activate() override;
  void deactivate() override;
  void fitToWindow() override;
  void setWindowLevel(double windowWidth, double windowCenter) override;
  void resetWindowLevel() override;
  void loadFiles(const QStringList& paths) override;

protected:
  void resizeEvent(QResizeEvent* event) override;

  // 滚轮翻层（KTD10）：装在 VTK widget 上抢先于 interactor 处理，
  // 阻断默认的滚轮相机推进。
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  // 按 ViewModel 当前状态刷新侧边滑条 range/位置与 n/N 层号；
  // N<=1 或未加载时禁用滑条。
  void syncSliceControls();

  Ui::GUICenter* ui;
  SeriesViewModel* m_seriesViewModel;
  QVTKOpenGLNativeWidget* m_vtkWidget;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  bool m_hasImage = false;
  // 右侧层滑条 + 底部层号（R4 保底导航；滚轮为主导航）
  QSlider* m_sliceSlider = nullptr;
  QLabel* m_sliceLabel = nullptr;
};
