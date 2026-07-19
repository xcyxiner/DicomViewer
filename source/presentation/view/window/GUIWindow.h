
#pragma once
#include <QMainWindow>

#include "presentation/view/menu/FileMenu.h"
#include "qtmetamacros.h"
#include "qwidget.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
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
  void setContent(QWidget* t_widget);
  void openFile();
  void initvtkTest();

  void showEvent(QShowEvent *event) override;
private:
  void createMenu();

private:
  QWidget* m_childWidget = {};

  FileMenu* m_fileMenu;

  std::unique_ptr<Ui::GUIWindow> ui;

   vtkSmartPointer<vtkRenderer> renderer;
   QVTKOpenGLNativeWidget* m_vtkWidget;
};
