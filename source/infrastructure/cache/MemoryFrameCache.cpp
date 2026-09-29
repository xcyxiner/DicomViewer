
#include "MemoryFrameCache.h"

MemoryFrameCache::MemoryFrameCache()
{
  cache = std::unordered_map<std::string,
                             std::unordered_map<int, IFrameCache::FramePtr>>();
}

void MemoryFrameCache::put(const std::string& uid,
                           int index,
                           const IFrameCache::FramePtr& frame)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (cache.find(uid) == cache.end()) {
    cache[uid] = std::unordered_map<int, IFrameCache::FramePtr>();
  }
  cache[uid][index] = frame;
}

IFrameCache::FramePtr MemoryFrameCache::get(const std::string& uid, int index)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (cache.find(uid) != cache.end()) {
    const auto& frameMap = cache[uid];
    if (frameMap.find(index) != frameMap.end()) {
      // 锁内返回值拷贝（引用计数递增），调用方持有期间不受并发
      // put/get 影响
      return frameMap.at(index);
    }
  }
  return IFrameCache::FramePtr();
}

void MemoryFrameCache::putVolume(const std::string& seriesKey,
                                 vtkSmartPointer<vtkImageData> volume)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  // 单槽语义：新体写入前释放旧体，避免内存无限增长（换序列峰值
  // 约 2 体积），旧体不可再取回
  m_volumes.clear();
  if (volume) {
    m_volumes[seriesKey] = std::move(volume);
  }
}

vtkSmartPointer<vtkImageData> MemoryFrameCache::getVolume(
    const std::string& seriesKey)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_volumes.find(seriesKey);
  if (it == m_volumes.end()) {
    return nullptr;
  }
  return it->second;
}
