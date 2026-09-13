
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

private:
  Ui::GUICenter* ui;
  SeriesViewModel* m_seriesViewModel;
  QVTKOpenGLNativeWidget* m_vtkWidget;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  bool m_hasImage = false;
};
