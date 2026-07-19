
#include <QMenuBar>

#include "GUIWindow.h"

#include "presentation/view/gui/GUICenter.h"
#include "presentation/view/menu/FileMenu.h"
#include "ui_GUIWindow.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <QVBoxLayout>

GUIWindow::GUIWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::GUIWindow>())
{
  ui->setupUi(this);
  createMenu();
  initvtkTest();
}
void GUIWindow::initvtkTest(){
  // 1. 创建Qt控件
 m_vtkWidget = new QVTKOpenGLNativeWidget(this);

// 2. 创建渲染窗口和渲染器
auto m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
 renderer = vtkSmartPointer<vtkRenderer>::New();
m_renderWindow->AddRenderer(renderer);
 // ✅ 关键：将渲染窗口设置到 QVTKOpenGLNativeWidget
  m_vtkWidget->setRenderWindow(m_renderWindow);
// 4. 添加到布局（resize(400,300)会被布局覆盖，建议省略或设置最小尺寸）
ui->layout->addWidget(m_vtkWidget);
}

void GUIWindow::showEvent(QShowEvent* event) {
  static bool firstShow = true;
  if (firstShow) {
    firstShow = false;
  // 5. 可以添加一些测试数据看看效果（比如立方体）
  auto source = vtkSmartPointer<vtkConeSource>::New();
  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(source->GetOutputPort());
  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->ResetCamera();
 renderer->GetRenderWindow()->Render();
  }
}

GUIWindow::~GUIWindow() = default;

void GUIWindow::setContent(QWidget* t_widget)
{
  // this->m_childWidget = t_widget;
  // this->ui->centralwidget = t_widget;
}

void GUIWindow::createMenu()
{
  this->m_fileMenu = new FileMenu(this->ui->menubar, this);
}

void GUIWindow::openFile()
{
  dynamic_cast<GUICenter*>(m_childWidget)->onOpenFile();
}
