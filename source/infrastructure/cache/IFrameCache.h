
#pragma once
#include <memory>
#include "domain/model/Frame.h"

#include <variant>
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
class IFrameCache 
{
public:
    explicit IFrameCache();
    virtual ~IFrameCache()=default;

public:
    using FramePtr = std::variant<std::shared_ptr<Frame>, vtkSmartPointer<vtkImageData>,nullptr_t>;

    // 按 (SOP Instance UID, 层号) 写入单帧。
    // 注意：体为唯一像素源后，逐帧像素不再入缓存（用例侧已移除逐帧 put）。
    virtual void put(const std::string& uid, int index, const IFrameCache::FramePtr& frame) = 0;

    // 按 (SOP Instance UID, 层号) 读取单帧。
    // 实现须在锁内返回拷贝（shared_ptr 值语义），禁止返回内部引用。
    virtual IFrameCache::FramePtr get(const std::string& uid, int index) = 0;

    // ---- 体槽位：以 SeriesInstanceUID 为键存取整幅 3D 体 ----
    // 单槽语义：新序列写入替换旧体，避免内存无限增长（换序列峰值约 2 体积）。
    // putVolume 时机：加载完成回主线程后的同一替换点调用，
    // 与 StackDisplaySet 整体替换一起对 GUI 原子可见（防止加载中途
    // 按旧 seriesKey 拉体 miss 出空白）。
    virtual void putVolume(const std::string& seriesKey,
                           vtkSmartPointer<vtkImageData> volume) = 0;

    // 未命中返回空指针，由调用方（MPR 面板）降级显示占位。
    virtual vtkSmartPointer<vtkImageData> getVolume(
        const std::string& seriesKey) = 0;
};
