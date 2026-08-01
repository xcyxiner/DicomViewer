
#include "SeriesViewModel.h"

#include "infrastructure/cache/MemoryFrameCache.h"
#include "infrastructure/dicom_io/HybridReader.h"
#include "infrastructure/dicom_io/VTKDicomAdaptReader.h"
#include "infrastructure/rendering/VtkAdaptRenderer.h"
#include "infrastructure/task/QtTaskQueue.h"

SeriesViewModel::SeriesViewModel(QObject* parent)
    : QObject(parent)
{
  m_imageRenderer = std::make_shared<VtkAdaptRenderer>();
  m_stackDisplaySet = std::make_shared<StackDisplaySet>();
  m_dicomReader = std::make_unique<VTKDicomAdaptReader>();
  m_taskQueue = std::make_unique<QtTaskQueue>();
  m_frameCache = std::make_unique<MemoryFrameCache>();
  m_loadSeriesUseCase = std::make_unique<LoadSeriesUseCase>(
      *m_dicomReader, *m_taskQueue, *m_frameCache);
}

void SeriesViewModel::setRenderWindow(vtkSmartPointer<vtkRenderWindow> window)
{
  m_imageRenderer->setRenderTarget(window);
}

void SeriesViewModel::render()
{
  std::string sopUid = m_stackDisplaySet->getFrameUids().at(
      m_stackDisplaySet->getCurrentIndex());
  int frameIndex = m_stackDisplaySet->getCurrentIndex();
  auto frame = m_frameCache->get(sopUid, frameIndex);
  using FrameInfoType = std::decay_t<decltype(frame)>;
  if constexpr (std::is_same_v<FrameInfoType, nullptr_t>) {
    return;
  }
  m_imageRenderer->render(frame, m_stackDisplaySet->getDisplaySettings());
}

void SeriesViewModel::loadSeries(const std::string& path)
{
  auto future = m_loadSeriesUseCase->loadSeriesAsync(path);
  auto watcher =
      std::make_shared<std::future<std::shared_ptr<StackDisplaySet>>>(
          std::move(future));
  std::thread(
      [this, watcher]()
      {
        m_stackDisplaySet = watcher->get();
        emit imageChanged();
      })
      .detach();
}
