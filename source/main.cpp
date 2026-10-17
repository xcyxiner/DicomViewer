#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/container/ServiceContainer.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/IMprRenderer.h"
#include "infrastructure/rendering/IVolumeRenderer.h"
#include "infrastructure/rendering/VtkAdaptRenderer.h"
#include "infrastructure/rendering/VtkMprRenderer.h"
#include "infrastructure/rendering/VtkVolumeRenderer.h"
#include "infrastructure/task/QtTaskQueue.h"
#include "presentation/view/gui/GUICenter.h"
#include "presentation/view/gui/MprPanel.h"
#include "presentation/view/gui/VolumePanel.h"
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
  // MPR 渲染器（KTD3/KTD8：组合根构造，注入 MprPanel，面板不自建实现）
  container.registerInstance<IMprRenderer>(std::make_shared<VtkMprRenderer>());
  // 3D 体绘制渲染器（同契约：组合根构造注入 VolumePanel）
  container.registerInstance<IVolumeRenderer>(
      std::make_shared<VtkVolumeRenderer>());

  // 从容器解析依赖，注入到 ViewModel
  SeriesViewModel viewModel(container.resolve<IImageRenderer>(),
                            container.resolve<IDicomReader>(),
                            container.resolve<ITaskQueue>(),
                            container.resolve<IFrameCache>());

  GUIWindow window;
  GUICenter center(&viewModel);
  // MPR 三平面 = 三个独立面板共享同一 renderer：各自占一个网格槽位，
  // 2x2 下三格分占；注册序即默认槽序（2D → 轴 → 冠 → 矢）
  auto mprRenderer = container.resolve<IMprRenderer>();
  MprPanel mprAxial(&viewModel, mprRenderer, IMprRenderer::kPlaneAxial);
  MprPanel mprCoronal(&viewModel, mprRenderer, IMprRenderer::kPlaneCoronal);
  MprPanel mprSagittal(&viewModel, mprRenderer, IMprRenderer::kPlaneSagittal);
  // 3D 体绘制面板（Mpr3d 模式默认槽 3）
  VolumePanel volume3d(&viewModel, container.resolve<IVolumeRenderer>());
  window.registerView(&center);
  // LayoutMenu 经 viewRegistered 自动列出（三个互异平面名 + 3D）
  // 注册序即默认槽序：TwoDOnly → 槽0=2D；Mpr3d → 槽0-3 = 轴/冠/矢/3D
  window.registerView(&mprAxial);
  window.registerView(&mprCoronal);
  window.registerView(&mprSagittal);
  window.registerView(&volume3d);
  window.show();

  return application.exec();
}
