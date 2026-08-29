#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/container/ServiceContainer.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/VtkAdaptRenderer.h"
#include "infrastructure/task/QtTaskQueue.h"
#include "presentation/view/gui/GUICenter.h"
#include "presentation/view/window/GUIWindow.h"
#include "presentation/viewmodels/SeriesViewModel.h"

auto main(int argc, char* argv[]) -> int
{
  QApplication application(argc, argv);

  // 设置 DCMTK 字典路径（打包后使用相对路径）
#ifdef __linux__
  {
    QString appDir = QCoreApplication::applicationDirPath();
    QString dictPath =
        QDir(appDir).absoluteFilePath("../share/dcmtk/dicom.dic");
    if (QFile::exists(dictPath)) {
      qputenv("DCMDICTPATH", dictPath.toLocal8Bit());
    }
  }
#endif

  // Composition Root — 注册所有服务
  ServiceContainer container;
  container.registerInstance<IImageRenderer>(
      std::make_shared<VtkAdaptRenderer>());
  container.registerInstance<IDicomReader>(
      std::make_shared<VTKDicomAdaptReader>());
  container.registerInstance<ITaskQueue>(std::make_shared<QtTaskQueue>());
  container.registerInstance<IFrameCache>(std::make_shared<MemoryFrameCache>());

  // 从容器解析依赖，注入到 ViewModel
  SeriesViewModel viewModel(container.resolve<IImageRenderer>(),
                            container.resolve<IDicomReader>(),
                            container.resolve<ITaskQueue>(),
                            container.resolve<IFrameCache>());

  GUIWindow window;
  GUICenter center(&viewModel);
  window.setContent(&center);
  window.show();

  return application.exec();
}
