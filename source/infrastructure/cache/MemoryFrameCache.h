
#pragma once
#include "IFrameCache.h"
#include <unordered_map>
class MemoryFrameCache : public IFrameCache
{
public:
    explicit MemoryFrameCache();
    ~MemoryFrameCache()=default;
public:
     void put(const std::string& uid, int index, const IFrameCache::FramePtr& frame) override;
     IFrameCache::FramePtr get(const std::string& uid, int index) override;
private:
    std::unordered_map<std::string, std::unordered_map<int, IFrameCache::FramePtr>> cache;
};
