#include <QApplication>

#include "presentation/view/gui/GUICenter.h"
#include "qapplication.h"
#include "presentation/view/window/GUIWindow.h"

auto main(int argc, char* argv[]) -> int
{
  QApplication application(argc, argv);
  GUIWindow window;
  GUICenter center;
  window.setContent(&center);
  window.show();
  return application.exec();
}
