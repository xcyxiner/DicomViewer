
#pragma once
#include <QWidget>
#include <memory>

#include "qtmetamacros.h"
#include "presentation/viewmodels/SeriesViewModel.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include <QVBoxLayout>
namespace Ui {
    class GUICenter;
}
class GUICenter : public QWidget
{
  Q_OBJECT
public:
  explicit GUICenter(SeriesViewModel* viewModel,
                     QWidget* parent = nullptr);
  ~GUICenter();

public:
  void onOpenFile();
  void onOpenFolder();
  void showEvent();
  void fitToWindow();
  void resetWindowLevel();
  void setWindowLevel(double windowWidth, double windowCenter);

protected:
  void resizeEvent(QResizeEvent* event) override;

signals:
  void addFiles(const QStringList& t_paths);

private:
  Ui::GUICenter* ui;
  SeriesViewModel* m_seriesViewModel;
  QVTKOpenGLNativeWidget* m_vtkWidget;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  bool m_hasImage = false;
};
