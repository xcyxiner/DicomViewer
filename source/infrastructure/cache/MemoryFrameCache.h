
#pragma once
#include "IFrameCache.h"
#include <mutex>
#include <unordered_map>
class MemoryFrameCache : public IFrameCache
{
public:
    explicit MemoryFrameCache();
    ~MemoryFrameCache()=default;
public:
    // 写入 (uid, index) 帧；全程持锁（后台加载 put、GUI 线程 get）。
    void put(const std::string& uid, int index, const IFrameCache::FramePtr& frame) override;

    // 持锁查找并返回值拷贝（shared_ptr/vtkSmartPointer 引用计数在锁内
    // 递增，禁止返回内部引用）。
    IFrameCache::FramePtr get(const std::string& uid, int index) override;

    // 体槽位写入（单槽语义：先清空旧体再写入，新序列必然替换旧体；
    // 持锁）。seriesKey 为 SeriesInstanceUID。
    void putVolume(const std::string& seriesKey,
                   vtkSmartPointer<vtkImageData> volume) override;

    // 持锁查找；未命中返回空指针，由调用方（MPR 面板）降级显示占位。
    vtkSmartPointer<vtkImageData> getVolume(
        const std::string& seriesKey) override;

private:
    // 互斥锁覆盖帧与体的全部读写路径（KTD9：消除后台 put 与 GUI
    // get 之间的数据竞争）。
    std::mutex m_mutex;
    std::unordered_map<std::string, std::unordered_map<int, IFrameCache::FramePtr>> cache;
    // 体槽位：seriesKey → 整幅体（单槽语义，新体替换旧体）
    std::unordered_map<std::string, vtkSmartPointer<vtkImageData>> m_volumes;
};
