
#pragma once
#include <QWidget>
#include <memory>


#include "qglobal.h"
#include "qtmetamacros.h"
#include "presentation/viewmodels/SeriesViewModel.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
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
  void showEvent();

signals:
  void addFiles(const QStringList& t_paths);

private:
  Ui::GUICenter* ui;
  SeriesViewModel* m_seriesViewModel;
  QVTKOpenGLNativeWidget* m_vtkWidget;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;

     vtkSmartPointer<vtkRenderer> renderer;
   void initvtkTest();
};
