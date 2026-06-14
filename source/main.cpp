#include <QApplication>

#include "GUICenter.h"
#include "GUIWindow.h"
#include "qapplication.h"

auto main(int argc, char* argv[]) -> int
{
  QApplication application(argc, argv);
  GUIWindow window;
  GUICenter center;
  window.setContent(&center);
  window.show();
  return application.exec();
}
