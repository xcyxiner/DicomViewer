
#include <algorithm>

#include "SeriesViewModel.h"

#include "infrastructure/utils/FloatCompare.h"
#include "infrastructure/utils/ImageSliceExtractor.h"

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
  m_loadSeriesUseCase =
      std::make_unique<LoadSeriesUseCase>(*m_dicomReader, *m_taskQueue);

  // KTD11：2D renderer 的窗宽窗位交互回灌统一绑到本对象（回调由
  // ViewModel 持有，面板不直接挂 renderer 回调）；MPR 侧经
  // MprPanel 一行转发到 notifyWindowLevelInteracted。
  m_imageRenderer->setWindowLevelCallback(
      [this](double windowWidth, double windowCenter)
      { notifyWindowLevelInteracted(windowWidth, windowCenter); });
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
  const int count = frameCount();
  const int index = currentIndex();
  if (count <= 0 || index < 0 || index >= count) {
    // 未加载 / 索引越界：安全空操作
    return;
  }

  // 体为唯一像素源（KTD1）：按 seriesKey 取体，经唯一抽层实现取当前层。
  // 不在此调用 reader —— reader 单例被后台加载占用，跨线程调用违反
  // KTD9 的加载期隔离；抽层工具函数本身对体快照操作，线程安全前提是
  // 体已稳定提交（KTD9 同一替换点）。
  auto volume = getVolume();
  if (!volume) {
    // 单槽替换的已知局限：新序列体替换槽位后，残留旧 seriesKey 取体
    // miss（换序列中途或旧显示集未同步）——跳过本次渲染，不崩溃。
    return;
  }
  auto slice = ImageSliceExtractor::extractSlice(volume.Get(), index);
  if (!slice) {
    return;
  }
  // std::visit 的 vtkImageData 分支由 renderer 内部处理
  m_imageRenderer->render(slice, m_stackDisplaySet->getDisplaySettings());
}

void SeriesViewModel::fitToWindow()
{
  m_imageRenderer->fitToWindow();
}

void SeriesViewModel::loadSeries(const std::string& path)
{
  // 等待上一次加载完成 —— join 串行化兼作 reader 单例免于线程池
  // 并发的唯一屏障（不可直接移除）
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }

  auto future = m_loadSeriesUseCase->loadSeriesAsync(path);

  m_workerThread = std::thread(
      [this, future = std::move(future), path]() mutable
      {
        try {
          auto result = std::move(future).get();
          // 回到主线程：体入槽位与显示集替换在同一点原子提交（KTD9），
          // 随后按 KTD7 顺序发信号
          QMetaObject::invokeMethod(
              this,
              [this, result = std::move(result), path]() mutable
              {
                if (!result.error.empty() || !result.displaySet) {
                  // 失败：不触碰旧 StackDisplaySet 与体槽位（R9）
                  postLoadFailed(
                      path, result.error.empty() ? "加载失败" : result.error);
                  return;
                }
                // 同一替换点：先体入槽位，再整体替换显示集
                m_frameCache->putVolume(result.displaySet->getSeriesKey(),
                                        result.volume);
                m_stackDisplaySet = std::move(result.displaySet);
                emit seriesLoaded(
                    QString::fromStdString(m_stackDisplaySet->getSeriesKey()));
                // KTD7：紧随 seriesLoaded 广播一次初始窗宽窗位，
                // 作为 MPR 面板的初始通道
                const auto& settings = m_stackDisplaySet->getDisplaySettings();
                m_initialWindowWidth = settings.getWindowWidth();
                m_initialWindowCenter = settings.getWindowCenter();
                emit windowLevelChanged(settings.getWindowWidth(),
                                        settings.getWindowCenter());
                emit imageChanged();
              },
              Qt::QueuedConnection);
        } catch (const std::exception& e) {
          // 用例已捕获业务异常；此处为防御层，保证任何失败都以
          // loadFailed 呈现而非越过线程边界
          postLoadFailed(path, e.what());
        } catch (...) {
          postLoadFailed(path, "加载失败（未知错误）");
        }
      });
}

void SeriesViewModel::postLoadFailed(const std::string& path,
                                     const std::string& reason)
{
  // 失败通道统一出口：队列投递到主线程后发 loadFailed（跨线程信号
  // 参数用 QString，KTD7）。成功/异常三条失败路径共用，恰发一次。
  QMetaObject::invokeMethod(
      this,
      [this, path, reason]()
      {
        emit loadFailed(QString::fromStdString(path),
                        QString::fromStdString(reason));
      },
      Qt::QueuedConnection);
}

void SeriesViewModel::resetWindowLevel()
{
  // 恢复到加载时捕获的初始值，经同一广播链（菜单 reset → 焦点面板
  // → 此处）——DisplaySettings 已被后续修改覆盖，故单独保存初始值
  applyWindowLevel(m_initialWindowWidth, m_initialWindowCenter);
}

void SeriesViewModel::setWindowLevel(double windowWidth, double windowCenter)
{
  applyWindowLevel(windowWidth, windowCenter);
}

void SeriesViewModel::notifyWindowLevelInteracted(double windowWidth,
                                                  double windowCenter)
{
  // 交互回灌与菜单/程序化设置走同一链路（最后操作为准，KTD11）
  applyWindowLevel(windowWidth, windowCenter);
}

void SeriesViewModel::applyWindowLevel(double windowWidth, double windowCenter)
{
  if (!m_stackDisplaySet || m_stackDisplaySet->getFrameUids().empty()) {
    return;
  }

  const auto& current = m_stackDisplaySet->getDisplaySettings();
  if (FloatCompare::nearlyEqual(current.getWindowWidth(), windowWidth)
      && FloatCompare::nearlyEqual(current.getWindowCenter(), windowCenter))
  {
    // 值比较幂等（KTD11 第二道闸）：程序化回写被回灌的同值不再
    // 广播，切断广播-回灌环
    return;
  }

  // 持久化到显示设置：翻层重绘（render 传 DisplaySettings）沿用
  // 最后一次操作的窗宽窗位，而不是回退到加载初始值
  DisplaySettings settings = current;
  settings.setWindowWidth(windowWidth);
  settings.setWindowCenter(windowCenter);
  m_stackDisplaySet->setDisplaySettings(settings);

  // 写 2D renderer（其内部在程序化回写期间抑制交互回调）
  m_imageRenderer->updateWindowLevel(windowWidth, windowCenter);
  // 恰发一次广播 → 所有面板（GUICenter 经 ViewModel 已写；MprPanel
  // 订阅后写 MPR renderer），最后操作为准
  emit windowLevelChanged(windowWidth, windowCenter);
}

void SeriesViewModel::nextFrame()
{
  setCurrentIndex(currentIndex() + 1);
}

void SeriesViewModel::previousFrame()
{
  setCurrentIndex(currentIndex() - 1);
}

void SeriesViewModel::setCurrentIndex(int index)
{
  const int count = frameCount();
  if (count <= 0) {
    // 未加载序列：安全空操作
    return;
  }
  const int clamped = std::clamp(index, 0, count - 1);
  if (clamped == m_stackDisplaySet->getCurrentIndex()) {
    // 同值幂等：不重发 sliceChanged（防滑条-信号回环），也不重复渲染
    return;
  }
  m_stackDisplaySet->setCurrentIndex(clamped);
  emit sliceChanged(clamped);
  // 只刷新渲染，不触发 imageChanged —— 相机与窗宽窗位保持（R4）
  render();
}

int SeriesViewModel::frameCount() const
{
  if (!m_stackDisplaySet) {
    return 0;
  }
  return static_cast<int>(m_stackDisplaySet->getFrameUids().size());
}

int SeriesViewModel::currentIndex() const
{
  if (!m_stackDisplaySet) {
    return 0;
  }
  return m_stackDisplaySet->getCurrentIndex();
}

vtkSmartPointer<vtkImageData> SeriesViewModel::getVolume() const
{
  if (!m_stackDisplaySet) {
    return nullptr;
  }
  const std::string& seriesKey = m_stackDisplaySet->getSeriesKey();
  if (seriesKey.empty()) {
    return nullptr;
  }
  return m_frameCache->getVolume(seriesKey);
}
