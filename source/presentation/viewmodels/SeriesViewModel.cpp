
#include "SeriesViewModel.h"

SeriesViewModel::SeriesViewModel(std::shared_ptr<IImageRenderer> renderer,
                                 std::shared_ptr<IDicomReader> reader,
                                 std::shared_ptr<ITaskQueue> taskQueue,
                                 std::shared_ptr<IFrameCache> frameCache,
                                 QObject* parent)
    : QObject(parent)
    , m_imageRenderer(std::move(renderer))
    , m_stackDisplaySet(std::make_shared<StackDisplaySet>())
    , m_dicomReader(std::move(reader))
    , m_taskQueue(std::move(taskQueue))
    , m_frameCache(std::move(frameCache))
{
  m_loadSeriesUseCase = std::make_unique<LoadSeriesUseCase>(
      *m_dicomReader, *m_taskQueue, *m_frameCache);
}

SeriesViewModel::~SeriesViewModel()
{
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }
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
  // std::visit 已在 renderer 内部处理 nullptr_t 分支
  m_imageRenderer->render(frame, m_stackDisplaySet->getDisplaySettings());
}

void SeriesViewModel::fitToWindow()
{
  m_imageRenderer->fitToWindow();
}

void SeriesViewModel::loadSeries(const std::string& path)
{
  // 等待上一次加载完成
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }

  auto future = m_loadSeriesUseCase->loadSeriesAsync(path);

  m_workerThread = std::thread(
      [this, future = std::move(future)]() mutable
      {
        auto result = std::move(future).get();
        // 回到主线程更新数据并发信号
        QMetaObject::invokeMethod(
            this,
            [this, result = std::move(result)]() mutable
            {
              m_stackDisplaySet = std::move(result);
              emit imageChanged();
            },
            Qt::QueuedConnection);
      });
}
