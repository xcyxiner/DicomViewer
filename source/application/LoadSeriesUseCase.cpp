

#include <cmath>
#include <exception>
#include <limits>
#include <vector>

#include "LoadSeriesUseCase.h"

LoadSeriesUseCase::LoadSeriesUseCase(IDicomReader& dicomReader,
                                     ITaskQueue& taskQueue)
    : m_dicomReader(dicomReader)
    , m_taskQueue(taskQueue)
{
}

std::future<LoadSeriesResult> LoadSeriesUseCase::loadSeriesAsync(
    const std::string& path)
{
  // KTD4：一次全读入。后台只产出体与元数据；体入槽位由 ViewModel
  // 在回主线程的同一替换点完成（KTD9），此处不碰缓存。
  return m_taskQueue.submitWithResult(
      [this, path]() -> LoadSeriesResult
      {
        LoadSeriesResult result;
        try {
          m_dicomReader.open(path);

          const int frameCount = m_dicomReader.getFrameCount();
          if (frameCount <= 0) {
            result.error = "序列无有效帧: " + path;
            return result;
          }

          // 逐层元数据：收集全部 SOP Instance UID（R3）与第 0 帧
          // WW/WC（初始显示设置）
          std::vector<std::string> frameUids;
          frameUids.reserve(frameCount);
          double firstWidth = 0.0;
          double firstCenter = 0.0;
          for (int i = 0; i < frameCount; ++i) {
            auto info = m_dicomReader.readFrameInfo(i);
            if (!info || info->getSopInstanceUid().empty()) {
              result.error =
                  "第 " + std::to_string(i) + " 层元数据缺失: " + path;
              return result;
            }
            frameUids.push_back(info->getSopInstanceUid());
            if (i == 0) {
              firstWidth = info->getWindowWidth();
              firstCenter = info->getWindowCenter();
            }
          }

          auto volume = m_dicomReader.readVolume();
          if (!volume) {
            result.error = "体数据读取失败: " + path;
            return result;
          }

          auto displaySet = std::make_shared<StackDisplaySet>();
          displaySet->setFrameUids(std::move(frameUids));
          displaySet->setCurrentIndex(0);
          displaySet->setSeriesKey(m_dicomReader.getSeriesInstanceUid());
          // 第 0 帧无 WW/WC 时保留 DisplaySettings 默认值（R3）
          if (firstWidth > 0.0
              && std::abs(firstCenter) > std::numeric_limits<double>::epsilon())
          {
            DisplaySettings settings;
            settings.setWindowWidth(firstWidth);
            settings.setWindowCenter(firstCenter);
            displaySet->setDisplaySettings(settings);
          }

          result.displaySet = std::move(displaySet);
          result.volume = std::move(volume);
        } catch (const std::exception& e) {
          // 异常在此捕获转为失败结果，禁止越过线程边界（否则
          // std::terminate）；失败不携带任何半成品状态
          result.displaySet.reset();
          result.volume = nullptr;
          result.error = e.what();
          if (result.error.empty()) {
            result.error = "加载失败: " + path;
          }
        } catch (...) {
          result.displaySet.reset();
          result.volume = nullptr;
          result.error = "加载失败（未知错误）: " + path;
        }
        return result;
      });
}
